/*
 * Copyright (C) 2010-2016 Oracle Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, in which case the provisions of the CDDL are applicable
 * instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#ifndef __VBOXGUEST_CORE_H__
#define __VBOXGUEST_CORE_H__

#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/vbox_vmmdev.h>
#include <linux/vboxguest.h>

struct vbg_session;

/** VBox guest memory balloon. */
struct vbg_mem_balloon {
	/** Work handling VMMDEV_EVENT_BALLOON_CHANGE_REQUEST events */
	struct work_struct work;
	/** Pre-allocated VMMDevGetMemBalloonChangeRequest for query */
	VMMDevGetMemBalloonChangeRequest *get_req;
	/** Pre-allocated VMMDevChangeMemBalloon req for inflate / deflate */
	VMMDevChangeMemBalloon *change_req;
	/** The current number of chunks in the balloon. */
	u32 chunks;
	/** The maximum number of chunks in the balloon. */
	u32 max_chunks;
	/**
	 * Array of pointers to page arrays. A page * array is allocated for
	 * each chunk when inflating, and freed when the deflating.
	 */
	struct page ***pages;
};

/**
 * Per bit usage tracker for a u32 mask.
 *
 * Used for optimal handling of guest properties and event filter.
 */
struct vbg_bit_usage_tracker {
	/** Per bit usage counters. */
	u32 per_bit_usage[32];
	/** The current mask according to per_bit_usage. */
	u32 mask;
};

/** VBox guest device (data) extension. */
struct vbg_dev {
	struct device *dev;
	/** The base of the adapter I/O ports. */
	u16 io_port;
	/** Pointer to the mapping of the VMMDev adapter memory. */
	VMMDevMemory *mmio;
	/** Host version */
	char host_version[64];
	/** Host features */
	unsigned int host_features;
	/**
	 * Dummy page and vmap address for reserved kernel virtual-address
	 * space for the guest mappings, only used on hosts lacking vtx.
	 */
	struct page *guest_mappings_dummy_page;
	void *guest_mappings;
	/** Spinlock protecting pending_events. */
	spinlock_t event_spinlock;
	/** Preallocated VMMDevEvents for the IRQ handler. */
	VMMDevEvents *ack_events_req;
	/** Wait-for-event list for threads waiting for multiple events. */
	wait_queue_head_t event_wq;
	/** Mask of pending events. */
	u32 pending_events;
	/** Wait-for-event list for threads waiting on HGCM async completion. */
	wait_queue_head_t hgcm_wq;
	/** Pre-allocated hgcm cancel2 req. for cancellation on timeout */
	VMMDevHGCMCancel2 *cancel_req;
	/** Mutex protecting cancel_req accesses */
	struct mutex cancel_req_mutex;
	/** Pre-allocated mouse-status request for the input-device handling. */
	VMMDevReqMouseStatus *mouse_status_req;
	/** Input device for reporting abs mouse coordinates to the guest. */
	struct input_dev *input;

	/** Spinlock various items in vbg_session. */
	spinlock_t session_spinlock;
	/** List of guest sessions, protected by session_spinlock. */
	struct list_head session_list;
	/** Memory balloon information. */
	struct vbg_mem_balloon mem_balloon;

	/**
	 * @name Host Event Filtering
	 * @{
	 */

	/** Events we won't permit anyone to filter out. */
	u32 fixed_events;
	/** Usage counters for the host events (excludes fixed events). */
	struct vbg_bit_usage_tracker event_filter_tracker;
	/** The event filter last reported to the host (or UINT32_MAX). */
	u32 event_filter_host;
	/** @} */

	/**
	 * @name Guest Capabilities
	 * @{
	 */

	/**
	 * Usage counters for guest capabilities. Indexed by capability bit
	 * number, one count per session using a capability.
	 */
	struct vbg_bit_usage_tracker guest_caps_tracker;
	/** The guest capabilities last reported to the host (or UINT32_MAX). */
	u32 guest_caps_host;
	/** @} */

	/**
	 * Heartbeat timer which fires with interval
	 * cNsHearbeatInterval and its handler sends
	 * VMMDevReq_GuestHeartbeat to VMMDev.
	 */
	struct timer_list heartbeat_timer;
	/** Heartbeat timer interval in ms. */
	int heartbeat_interval_ms;
	/** Preallocated VMMDevReq_GuestHeartbeat request. */
	VMMDevRequestHeader *guest_heartbeat_req;

	/** "vboxguest" char-device */
	struct miscdevice misc_device;
	/** "vboxuser" char-device */
	struct miscdevice misc_device_user;
};

/** The VBoxGuest per session data. */
struct vbg_session {
	/** The list node. */
	struct list_head list_node;
	/** Pointer to the device extension. */
	struct vbg_dev *gdev;

	/**
	 * Array containing HGCM client IDs associated with this session.
	 * These will be automatically disconnected when the session is closed.
	 */
	u32 hgcm_client_ids[64];
	/**
	 * Host events requested by the session.
	 * An event type requested in any guest session will be added to the
	 * host filter. Protected by struct vbg_dev.SessionSpinlock.
	 */
	u32 event_filter;
	/**
	 * Guest capabilities for this session.
	 * A capability claimed by any guest session will be reported to the
	 * host. Protected by struct vbg_dev.SessionSpinlock.
	 */
	u32 guest_caps;
	/** Does this session belong to a root process or a user one? */
	bool user_session;
	/** Set on CANCEL_ALL_WAITEVENTS, protected by the event_spinlock. */
	bool cancel_waiters;
};

int  vbg_core_init(struct vbg_dev *gdev, u32 fixed_events);
void vbg_core_exit(struct vbg_dev *gdev);
int  vbg_core_open_session(struct vbg_dev *gdev,
			   struct vbg_session **session_ret, bool user_session);
void vbg_core_close_session(struct vbg_session *session);
int  vbg_core_ioctl(struct vbg_session *session, unsigned int req, void *data,
		    size_t data_size, size_t *data_size_ret);
int  vbg_core_set_mouse_status(struct vbg_dev *gdev, u32 features);

irqreturn_t vbg_core_isr(int irq, void *dev_id);

void vbg_linux_mouse_event(struct vbg_dev *gdev);

#endif
