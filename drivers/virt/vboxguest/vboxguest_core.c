/*
 * vboxguest core guest-device handling code, VBoxGuest.cpp in upstream svn.
 *
 * Copyright (C) 2007-2016 Oracle Corporation
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

#include <linux/device.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include <linux/vbox_err.h>
#include <linux/vbox_utils.h>
#include <linux/vmalloc.h>
#include "vboxguest_core.h"
#include "vboxguest_version.h"

#define GUEST_MAPPINGS_TRIES	5

/**
 * Reserves memory in which the VMM can relocate any guest mappings
 * that are floating around.
 *
 * This operation is a little bit tricky since the VMM might not accept
 * just any address because of address clashes between the three contexts
 * it operates in, so we try several times.
 *
 * Failure to reserve the guest mappings is ignored.
 *
 * @param   gdev	The Guest extension device.
 */
static void vbg_guest_mappings_init(struct vbg_dev *gdev)
{
	VMMDevReqHypervisorInfo *req;
	void *guest_mappings[GUEST_MAPPINGS_TRIES];
	struct page **pages = NULL;
	u32 size, hypervisor_size;
	int i, rc;

	/* Query the required space. */
	req = vbg_req_alloc(sizeof(*req), VMMDevReq_GetHypervisorInfo);
	if (!req)
		return;

	req->hypervisorStart = 0;
	req->hypervisorSize = 0;
	rc = vbg_req_perform(gdev, req);
	if (rc < 0)
		goto out;

	/*
	 * The VMM will report back if there is nothing it wants to map, like
	 * for instance in VT-x and AMD-V mode.
	 */
	if (req->hypervisorSize == 0)
		goto out;

	hypervisor_size = req->hypervisorSize;
	/* Add 4M so that we can align the vmap to 4MiB as the host requires. */
	size = PAGE_ALIGN(req->hypervisorSize) + SZ_4M;

	pages = kmalloc(sizeof(*pages) * (size >> PAGE_SHIFT), GFP_KERNEL);
	if (!pages)
		goto out;

	gdev->guest_mappings_dummy_page = alloc_page(GFP_HIGHUSER);
	if (!gdev->guest_mappings_dummy_page)
		goto out;

	for (i = 0; i < (size >> PAGE_SHIFT); i++)
		pages[i] = gdev->guest_mappings_dummy_page;

	/* Try several times, the host can be picky about certain addresses. */
	for (i = 0; i < GUEST_MAPPINGS_TRIES; i++) {
		guest_mappings[i] = vmap(pages, (size >> PAGE_SHIFT),
					 VM_MAP, PAGE_KERNEL_RO);
		if (!guest_mappings[i])
			break;

		req->header.requestType = VMMDevReq_SetHypervisorInfo;
		req->header.rc = VERR_INTERNAL_ERROR;
		req->hypervisorSize = hypervisor_size;
		req->hypervisorStart =
			(unsigned long)PTR_ALIGN(guest_mappings[i], SZ_4M);

		rc = vbg_req_perform(gdev, req);
		if (rc >= 0) {
			gdev->guest_mappings = guest_mappings[i];
			break;
		}
	}

	/* Free vmap's from failed attempts. */
	while (--i >= 0)
		vunmap(guest_mappings[i]);

	/* On failure free the dummy-page backing the vmap */
	if (!gdev->guest_mappings) {
		__free_page(gdev->guest_mappings_dummy_page);
		gdev->guest_mappings_dummy_page = NULL;
	}

out:
	kfree(req);
	kfree(pages);
}

/**
 * Undo what vbg_guest_mappings_init did.
 *
 * @param   gdev	The Guest extension device.
 */
static void vbg_guest_mappings_exit(struct vbg_dev *gdev)
{
	VMMDevReqHypervisorInfo *req;
	int rc;

	if (!gdev->guest_mappings)
		return;

	/*
	 * Tell the host that we're going to free the memory we reserved for
	 * it, the free it up. (Leak the memory if anything goes wrong here.)
	 */
	req = vbg_req_alloc(sizeof(*req), VMMDevReq_SetHypervisorInfo);
	if (!req)
		return;

	req->hypervisorStart = 0;
	req->hypervisorSize = 0;

	rc = vbg_req_perform(gdev, req);

	kfree(req);

	if (rc < 0) {
		vbg_err("%s error: %d\n", __func__, rc);
		return;
	}

	vunmap(gdev->guest_mappings);
	gdev->guest_mappings = NULL;

	__free_page(gdev->guest_mappings_dummy_page);
	gdev->guest_mappings_dummy_page = NULL;
}

/**
 * Report the guest information to the host.
 *
 * @returns 0 or negative errno value.
 * @param   gdev	The Guest extension device.
 */
static int vbg_report_guest_info(struct vbg_dev *gdev)
{
	/*
	 * Allocate and fill in the two guest info reports.
	 */
	VMMDevReportGuestInfo *req1 = NULL;
	VMMDevReportGuestInfo2 *req2 = NULL;
	int rc, ret = -ENOMEM;

	req1 = vbg_req_alloc(sizeof(*req1), VMMDevReq_ReportGuestInfo);
	req2 = vbg_req_alloc(sizeof(*req2), VMMDevReq_ReportGuestInfo2);
	if (!req1 || !req2)
		goto out_free;

	req1->guestInfo.interfaceVersion = VMMDEV_VERSION;
#ifdef CONFIG_X86_64
	req1->guestInfo.osType = VBOXOSTYPE_Linux26_x64;
#else
	req1->guestInfo.osType = VBOXOSTYPE_Linux26;
#endif

	req2->guestInfo.additionsMajor = VBOX_VERSION_MAJOR;
	req2->guestInfo.additionsMinor = VBOX_VERSION_MINOR;
	req2->guestInfo.additionsBuild = VBOX_VERSION_BUILD;
	req2->guestInfo.additionsRevision = VBOX_SVN_REV;
	/* (no features defined yet) */
	req2->guestInfo.additionsFeatures = 0;
	strlcpy(req2->guestInfo.szName, VBOX_VERSION_STRING,
		sizeof(req2->guestInfo.szName));

	/*
	 * There are two protocols here:
	 *      1. Info2 + Info1. Supported by >=3.2.51.
	 *      2. Info1 and optionally Info2. The old protocol.
	 *
	 * We try protocol 2 first.  It will fail with VERR_NOT_SUPPORTED
	 * if not supported by the VMMDev (message ordering requirement).
	 */
	rc = vbg_req_perform(gdev, req2);
	if (rc >= 0) {
		rc = vbg_req_perform(gdev, req1);
	} else if (rc == VERR_NOT_SUPPORTED || rc == VERR_NOT_IMPLEMENTED) {
		rc = vbg_req_perform(gdev, req1);
		if (rc >= 0) {
			rc = vbg_req_perform(gdev, req2);
			if (rc == VERR_NOT_IMPLEMENTED)
				rc = VINF_SUCCESS;
		}
	}
	ret = vbg_status_code_to_errno(rc);

out_free:
	kfree(req2);
	kfree(req1);
	return ret;
}

/**
 * Report the guest driver status to the host.
 *
 * @returns 0 or negative errno value.
 * @param   gdev	The Guest extension device.
 * @param   active	Flag whether the driver is now active or not.
 */
static int vbg_report_driver_status(struct vbg_dev *gdev, bool active)
{
	VMMDevReportGuestStatus *req;
	int rc;

	req = vbg_req_alloc(sizeof(*req), VMMDevReq_ReportGuestStatus);
	if (!req)
		return -ENOMEM;

	req->guestStatus.facility = VBoxGuestFacilityType_VBoxGuestDriver;
	req->guestStatus.status = active ? VBoxGuestFacilityStatus_Active :
					   VBoxGuestFacilityStatus_Inactive;
	req->guestStatus.flags = 0;

	rc = vbg_req_perform(gdev, req);
	if (rc == VERR_NOT_IMPLEMENTED)	/* Compatibility with older hosts. */
		rc = VINF_SUCCESS;

	kfree(req);

	return vbg_status_code_to_errno(rc);
}

/** @name Memory Ballooning
 * @{
 */

/**
 * Inflate the balloon by one chunk.
 *
 * The caller owns the balloon mutex.
 *
 * @returns VBox status code
 * @param   gdev	The Guest extension device.
 * @param   chunk_idx	Index of the chunk.
 */
static int vbg_balloon_inflate(struct vbg_dev *gdev, u32 chunk_idx)
{
	VMMDevChangeMemBalloon *req = gdev->mem_balloon.change_req;
	struct page **pages;
	int i, rc;

	pages = kmalloc(sizeof(*pages) * VMMDEV_MEMORY_BALLOON_CHUNK_PAGES,
			GFP_KERNEL | __GFP_NOWARN);
	if (!pages)
		return VERR_NO_MEMORY;

	req->header.size = sizeof(*req);
	req->inflate = true;
	req->pages = VMMDEV_MEMORY_BALLOON_CHUNK_PAGES;

	for (i = 0; i < VMMDEV_MEMORY_BALLOON_CHUNK_PAGES; i++) {
		pages[i] = alloc_page(GFP_KERNEL | __GFP_NOWARN);
		if (!pages[i]) {
			rc = VERR_NO_MEMORY;
			goto out_error;
		}

		req->phys_page[i] = page_to_phys(pages[i]);
	}

	rc = vbg_req_perform(gdev, req);
	if (rc < 0) {
		vbg_err("%s error: %d\n", __func__, rc);
		goto out_error;
	}

	gdev->mem_balloon.pages[chunk_idx] = pages;

	return VINF_SUCCESS;

out_error:
	while (--i >= 0)
		__free_page(pages[i]);
	kfree(pages);

	return rc;
}

/**
 * Deflate the balloon by one chunk.
 *
 * The caller owns the balloon mutex.
 *
 * @returns VBox status code
 * @param   gdev	The Guest extension device.
 * @param   chunk_idx	Index of the chunk.
 */
static int vbg_balloon_deflate(struct vbg_dev *gdev, u32 chunk_idx)
{
	VMMDevChangeMemBalloon *req = gdev->mem_balloon.change_req;
	struct page **pages = gdev->mem_balloon.pages[chunk_idx];
	int i, rc;

	req->header.size = sizeof(*req);
	req->inflate = false;
	req->pages = VMMDEV_MEMORY_BALLOON_CHUNK_PAGES;

	for (i = 0; i < VMMDEV_MEMORY_BALLOON_CHUNK_PAGES; i++)
		req->phys_page[i] = page_to_phys(pages[i]);

	rc = vbg_req_perform(gdev, req);
	if (rc < 0) {
		vbg_err("%s error: %d\n", __func__, rc);
		return rc;
	}

	for (i = 0; i < VMMDEV_MEMORY_BALLOON_CHUNK_PAGES; i++)
		__free_page(pages[i]);
	kfree(pages);
	gdev->mem_balloon.pages[chunk_idx] = NULL;

	return VINF_SUCCESS;
}

/**
 * Respond to VMMDEV_EVENT_BALLOON_CHANGE_REQUEST events, query the size
 * the host wants the balloon to be and adjust accordingly.
 */
static void vbg_balloon_work(struct work_struct *work)
{
	struct vbg_dev *gdev =
		container_of(work, struct vbg_dev, mem_balloon.work);
	VMMDevGetMemBalloonChangeRequest *req = gdev->mem_balloon.get_req;
	u32 i, chunks;
	int rc;

	/*
	 * Setting this bit means that we request the value from the host and
	 * change the guest memory balloon according to the returned value.
	 */
	req->eventAck = VMMDEV_EVENT_BALLOON_CHANGE_REQUEST;
	rc = vbg_req_perform(gdev, req);
	if (rc < 0) {
		vbg_err("%s error: %d)\n", __func__, rc);
		return;
	}

	/*
	 * The host always returns the same maximum amount of chunks, so
	 * we do this once.
	 */
	if (!gdev->mem_balloon.max_chunks) {
		gdev->mem_balloon.pages =
			devm_kcalloc(gdev->dev, req->cPhysMemChunks,
				     sizeof(struct page **), GFP_KERNEL);
		if (!gdev->mem_balloon.pages)
			return;

		gdev->mem_balloon.max_chunks = req->cPhysMemChunks;
	}

	chunks = req->cBalloonChunks;
	if (chunks > gdev->mem_balloon.max_chunks) {
		vbg_err("%s: illegal balloon size %u (max=%u)\n",
			__func__, chunks, gdev->mem_balloon.max_chunks);
		return;
	}

	if (req->cBalloonChunks > gdev->mem_balloon.chunks) {
		/* inflate */
		for (i = gdev->mem_balloon.chunks; i < chunks; i++) {
			rc = vbg_balloon_inflate(gdev, i);
			if (rc < 0)
				return;

			gdev->mem_balloon.chunks++;
		}
	} else {
		/* deflate */
		for (i = gdev->mem_balloon.chunks; i-- > chunks;) {
			rc = vbg_balloon_deflate(gdev, i);
			if (rc < 0)
				return;

			gdev->mem_balloon.chunks--;
		}
	}
}

/** @} */

/** @name Heartbeat
 * @{
 */

/**
 * Callback for heartbeat timer.
 */
static void vbg_heartbeat_timer(unsigned long data)
{
	struct vbg_dev *gdev = (struct vbg_dev *)data;

	vbg_req_perform(gdev, gdev->guest_heartbeat_req);
	mod_timer(&gdev->heartbeat_timer,
		  msecs_to_jiffies(gdev->heartbeat_interval_ms));
}

/**
 * Configure the host to check guest's heartbeat
 * and get heartbeat interval from the host.
 *
 * @returns 0 or negative errno value.
 * @param   gdev	The Guest extension device.
 * @param   enabled	Set true to enable guest heartbeat checks on host.
 */
static int vbg_heartbeat_host_config(struct vbg_dev *gdev, bool enabled)
{
	VMMDevReqHeartbeat *req;
	int rc;

	req = vbg_req_alloc(sizeof(*req), VMMDevReq_HeartbeatConfigure);
	if (!req)
		return -ENOMEM;

	req->fEnabled = enabled;
	req->cNsInterval = 0;
	rc = vbg_req_perform(gdev, req);
	do_div(req->cNsInterval, 1000000); /* ns -> ms */
	gdev->heartbeat_interval_ms = req->cNsInterval;
	kfree(req);

	return vbg_status_code_to_errno(rc);
}

/**
 * Initializes the heartbeat timer.
 *
 * This feature may be disabled by the host.
 *
 * @returns 0 or negative errno value (ignored).
 * @param   gdev	The Guest extension device.
 */
static int vbg_heartbeat_init(struct vbg_dev *gdev)
{
	int ret;

	/* Make sure that heartbeat checking is disabled if we fail. */
	ret = vbg_heartbeat_host_config(gdev, false);
	if (ret < 0)
		return ret;

	ret = vbg_heartbeat_host_config(gdev, true);
	if (ret < 0)
		return ret;

	/*
	 * Preallocate the request to use it from the timer callback because:
	 *    1) on Windows vbg_req_alloc must be called at IRQL <= APC_LEVEL
	 *       and the timer callback runs at DISPATCH_LEVEL;
	 *    2) avoid repeated allocations.
	 */
	gdev->guest_heartbeat_req = vbg_req_alloc(
					sizeof(*gdev->guest_heartbeat_req),
					VMMDevReq_GuestHeartbeat);
	if (!gdev->guest_heartbeat_req)
		return -ENOMEM;

	vbg_info("%s: Setting up heartbeat to trigger every %d milliseconds\n",
		 __func__, gdev->heartbeat_interval_ms);
	mod_timer(&gdev->heartbeat_timer, 0);

	return 0;
}

/**
 * Cleanup hearbeat code, stop HB timer and disable host heartbeat checking.
 * @param   gdev	The Guest extension device.
 */
static void vbg_heartbeat_exit(struct vbg_dev *gdev)
{
	del_timer_sync(&gdev->heartbeat_timer);
	vbg_heartbeat_host_config(gdev, false);
	kfree(gdev->guest_heartbeat_req);

}

/** @} */

/** @name Guest Capabilities and Event Filter
 * @{
 */

/**
 * Applies a change to the bit usage tracker.
 *
 * @returns true if the mask changed, false if not.
 * @param   tracker	The bit usage tracker.
 * @param   changed	The bits to change.
 * @param   previous	The previous value of the bits.
 */
static bool vbg_track_bit_usage(struct vbg_bit_usage_tracker *tracker,
				u32 changed, u32 previous)
{
	bool global_change = false;

	while (changed) {
		u32 bit = ffs(changed) - 1;
		u32 bitmask = BIT(bit);

		if (bitmask & previous) {
			tracker->per_bit_usage[bit] -= 1;
			if (tracker->per_bit_usage[bit] == 0) {
				global_change = true;
				tracker->mask &= ~bitmask;
			}
		} else {
			tracker->per_bit_usage[bit] += 1;
			if (tracker->per_bit_usage[bit] == 1) {
				global_change = true;
				tracker->mask |= bitmask;
			}
		}

		changed &= ~bitmask;
	}

	return global_change;
}

/**
 * Init and termination worker for resetting the (host) event filter on the host
 *
 * @returns 0 or negative errno value.
 * @param   gdev            The Guest extension device.
 * @param   fixed_events    Fixed events (init time).
 */
static int vbg_reset_host_event_filter(struct vbg_dev *gdev,
				       u32 fixed_events)
{
	VMMDevCtlGuestFilterMask *req;
	int rc;

	req = vbg_req_alloc(sizeof(*req), VMMDevReq_CtlGuestFilterMask);
	if (!req)
		return -ENOMEM;

	req->u32NotMask = U32_MAX & ~fixed_events;
	req->u32OrMask = fixed_events;
	rc = vbg_req_perform(gdev, req);
	if (rc < 0)
		vbg_err("%s error: %d\n", __func__, rc);

	kfree(req);
	return vbg_status_code_to_errno(rc);
}

/**
 * Changes the event filter mask for the given session.
 *
 * This is called in response to VBOXGUEST_IOCTL_CTL_FILTER_MASK as well as to
 * do session cleanup.
 *
 * @returns VBox status code
 * @param   gdev                The Guest extension device.
 * @param   session             The session.
 * @param   or_mask             The events to add.
 * @param   not_mask            The events to remove.
 * @param   session_termination Set if we're called by the session cleanup code.
 *                              This tweaks the error handling so we perform
 *                              proper session cleanup even if the host
 *                              misbehaves.
 *
 * @remarks Takes the session spinlock.
 */
static int vbg_set_session_event_filter(struct vbg_dev *gdev,
					struct vbg_session *session,
					u32 or_mask, u32 not_mask,
					bool session_termination)
{
	VMMDevCtlGuestFilterMask *req;
	u32 changed, previous;
	unsigned long flags;
	int rc = VINF_SUCCESS;

	/* Allocate a request buffer before taking the spinlock */
	req = vbg_req_alloc(sizeof(*req), VMMDevReq_CtlGuestFilterMask);
	if (!req) {
		if (!session_termination)
			return VERR_NO_MEMORY;
		/* Ignore failure, we must do session cleanup. */
	}

	spin_lock_irqsave(&gdev->session_spinlock, flags);

	/* Apply the changes to the session mask. */
	previous = session->event_filter;
	session->event_filter |= or_mask;
	session->event_filter &= ~not_mask;

	/* If anything actually changed, update the global usage counters. */
	changed = previous ^ session->event_filter;
	if (!changed)
		goto out;

	vbg_track_bit_usage(&gdev->event_filter_tracker, changed, previous);
	req->u32OrMask = gdev->fixed_events | gdev->event_filter_tracker.mask;

	if (gdev->event_filter_host == req->u32OrMask || !req)
		goto out;

	gdev->event_filter_host = req->u32OrMask;
	req->u32NotMask = ~req->u32OrMask;
	rc = vbg_req_perform(gdev, req);
	if (rc < 0) {
		/* Failed, roll back (unless it's session termination time). */
		gdev->event_filter_host = U32_MAX;
		if (session_termination)
			goto out;

		vbg_track_bit_usage(&gdev->event_filter_tracker, changed,
				    session->event_filter);
		session->event_filter = previous;
	}

out:
	spin_unlock_irqrestore(&gdev->session_spinlock, flags);
	kfree(req);

	return rc;
}

/**
 * Init and termination worker for set guest capabilities to zero on the host.
 *
 * @returns 0 or negative errno value.
 * @param   gdev	The Guest extension device.
 */
static int vbg_reset_host_capabilities(struct vbg_dev *gdev)
{
	VMMDevReqGuestCapabilities2 *req;
	int rc;

	req = vbg_req_alloc(sizeof(*req), VMMDevReq_SetGuestCapabilities);
	if (!req)
		return -ENOMEM;

	req->u32NotMask = U32_MAX;
	req->u32OrMask = 0;
	rc = vbg_req_perform(gdev, req);
	if (rc < 0)
		vbg_err("%s error: %d\n", __func__, rc);

	kfree(req);
	return vbg_status_code_to_errno(rc);
}

/**
 * Sets the guest capabilities for a session.
 *
 * @returns VBox status code
 * @param   gdev                The Guest extension device.
 * @param   session             The session.
 * @param   or_mask             The capabilities to add.
 * @param   not_mask            The capabilities to remove.
 * @param   session_termination Set if we're called by the session cleanup code.
 *                              This tweaks the error handling so we perform
 *                              proper session cleanup even if the host
 *                              misbehaves.
 *
 * @remarks Takes the session spinlock.
 */
static int vbg_set_session_capabilities(struct vbg_dev *gdev,
					struct vbg_session *session,
					u32 or_mask, u32 not_mask,
					bool session_termination)
{
	VMMDevReqGuestCapabilities2 *req;
	unsigned long flags;
	int rc = VINF_SUCCESS;
	u32 changed, previous;

	/* Allocate a request buffer before taking the spinlock */
	req = vbg_req_alloc(sizeof(*req), VMMDevReq_SetGuestCapabilities);
	if (!req) {
		if (!session_termination)
			return VERR_NO_MEMORY;
		/* Ignore failure, we must do session cleanup. */
	}

	spin_lock_irqsave(&gdev->session_spinlock, flags);

	/* Apply the changes to the session mask. */
	previous = session->guest_caps;
	session->guest_caps |= or_mask;
	session->guest_caps &= ~not_mask;

	/* If anything actually changed, update the global usage counters. */
	changed = previous ^ session->guest_caps;
	if (!changed)
		goto out;

	vbg_track_bit_usage(&gdev->guest_caps_tracker, changed, previous);
	req->u32OrMask = gdev->guest_caps_tracker.mask;

	if (gdev->guest_caps_host == req->u32OrMask || !req)
		goto out;

	gdev->guest_caps_host = req->u32OrMask;
	req->u32NotMask = ~req->u32OrMask;
	rc = vbg_req_perform(gdev, req);
	if (rc < 0) {
		/* Failed, roll back (unless it's session termination time). */
		gdev->guest_caps_host = U32_MAX;
		if (session_termination)
			goto out;

		vbg_track_bit_usage(&gdev->guest_caps_tracker, changed,
				    session->guest_caps);
		session->guest_caps = previous;
	}

out:
	spin_unlock_irqrestore(&gdev->session_spinlock, flags);
	kfree(req);

	return rc;
}

/** @} */

/**
 * vbg_query_host_version try get the host feature mask and version information
 * (vbg_host_version).
 *
 * @returns 0 or negative errno value (ignored).
 * @param   gdev	The Guest extension device.
 */
static int vbg_query_host_version(struct vbg_dev *gdev)
{
	VMMDevReqHostVersion *req;
	int rc, ret;

	req = vbg_req_alloc(sizeof(*req), VMMDevReq_GetHostVersion);
	if (!req)
		return -ENOMEM;

	rc = vbg_req_perform(gdev, req);
	ret = vbg_status_code_to_errno(rc);
	if (ret)
		goto out;

	snprintf(gdev->host_version, sizeof(gdev->host_version), "%u.%u.%ur%u",
		 req->major, req->minor, req->build, req->revision);
	gdev->host_features = req->features;

	vbg_info("vboxguest: host-version: %s %#x\n", gdev->host_version,
		 gdev->host_features);

	if (!(req->features & VMMDEV_HVF_HGCM_PHYS_PAGE_LIST)) {
		vbg_err("vboxguest: Error host too old (does not support page-lists)\n");
		ret = -ENODEV;
	}

out:
	kfree(req);
	return ret;
}

/**
 * Initializes the VBoxGuest device extension when the
 * device driver is loaded.
 *
 * The native code locates the VMMDev on the PCI bus and retrieve
 * the MMIO and I/O port ranges, this function will take care of
 * mapping the MMIO memory (if present). Upon successful return
 * the native code should set up the interrupt handler.
 *
 * @returns 0 or negative errno value.
 *
 * @param   gdev           The Guest extension device.
 * @param   fixed_events   Events that will be enabled upon init and no client
 *                         will ever be allowed to mask.
 */
int vbg_core_init(struct vbg_dev *gdev, u32 fixed_events)
{
	int ret = -ENOMEM;

	gdev->fixed_events = fixed_events | VMMDEV_EVENT_HGCM;
	gdev->event_filter_host = U32_MAX;	/* forces a report */
	gdev->guest_caps_host = U32_MAX;	/* forces a report */

	init_waitqueue_head(&gdev->event_wq);
	init_waitqueue_head(&gdev->hgcm_wq);
	INIT_LIST_HEAD(&gdev->session_list);
	spin_lock_init(&gdev->event_spinlock);
	spin_lock_init(&gdev->session_spinlock);
	mutex_init(&gdev->cancel_req_mutex);
	setup_timer(&gdev->heartbeat_timer, vbg_heartbeat_timer,
		    (unsigned long)gdev);
	INIT_WORK(&gdev->mem_balloon.work, vbg_balloon_work);

	gdev->mem_balloon.get_req =
		vbg_req_alloc(sizeof(*gdev->mem_balloon.get_req),
			      VMMDevReq_GetMemBalloonChangeRequest);
	gdev->mem_balloon.change_req =
		vbg_req_alloc(sizeof(*gdev->mem_balloon.change_req),
			      VMMDevReq_ChangeMemBalloon);
	gdev->cancel_req =
		vbg_req_alloc(sizeof(*(gdev->cancel_req)),
			      VMMDevReq_HGCMCancel2);
	gdev->ack_events_req =
		vbg_req_alloc(sizeof(*gdev->ack_events_req),
			      VMMDevReq_AcknowledgeEvents);
	gdev->mouse_status_req =
		vbg_req_alloc(sizeof(*gdev->mouse_status_req),
			      VMMDevReq_GetMouseStatus);

	if (!gdev->mem_balloon.get_req || !gdev->mem_balloon.change_req ||
	    !gdev->cancel_req || !gdev->ack_events_req ||
	    !gdev->mouse_status_req)
		goto err_free_reqs;

	ret = vbg_query_host_version(gdev);
	if (ret)
		goto err_free_reqs;

	ret = vbg_report_guest_info(gdev);
	if (ret) {
		vbg_err("vboxguest: VBoxReportGuestInfo error: %d\n", ret);
		goto err_free_reqs;
	}

	ret = vbg_reset_host_event_filter(gdev, gdev->fixed_events);
	if (ret) {
		vbg_err("vboxguest: Error setting fixed event filter: %d\n",
			ret);
		goto err_free_reqs;
	}

	ret = vbg_reset_host_capabilities(gdev);
	if (ret) {
		vbg_err("vboxguest: Error clearing guest capabilities: %d\n",
			ret);
		goto err_free_reqs;
	}

	ret = vbg_core_set_mouse_status(gdev, 0);
	if (ret) {
		vbg_err("vboxguest: Error clearing mouse status: %d\n", ret);
		goto err_free_reqs;
	}

	/* These may fail without requiring the driver init to fail. */
	vbg_guest_mappings_init(gdev);
	vbg_heartbeat_init(gdev);

	/* All Done! */
	ret = vbg_report_driver_status(gdev, true);
	if (ret < 0)
		vbg_err("vboxguest: VBoxReportGuestDriverStatus error: %d\n",
			ret);

	return 0;

err_free_reqs:
	kfree(gdev->mouse_status_req);
	kfree(gdev->ack_events_req);
	kfree(gdev->cancel_req);
	kfree(gdev->mem_balloon.change_req);
	kfree(gdev->mem_balloon.get_req);
	return ret;
}

/**
 * Call this on exit to clean-up vboxguest-core managed resources.
 *
 * The native code should call this before the driver is loaded,
 * but don't call this on shutdown.
 *
 * @param   gdev	The Guest extension device.
 */
void vbg_core_exit(struct vbg_dev *gdev)
{
	vbg_heartbeat_exit(gdev);
	vbg_guest_mappings_exit(gdev);

	/* Clear the host flags (mouse status etc). */
	vbg_reset_host_event_filter(gdev, 0);
	vbg_reset_host_capabilities(gdev);
	vbg_core_set_mouse_status(gdev, 0);

	kfree(gdev->mouse_status_req);
	kfree(gdev->ack_events_req);
	kfree(gdev->cancel_req);
	kfree(gdev->mem_balloon.change_req);
	kfree(gdev->mem_balloon.get_req);
}

/**
 * Creates a VBoxGuest user session.
 *
 * vboxguest_linux.c calls this when userspace opens the char-device.
 *
 * @returns 0 or negative errno value.
 * @param   gdev          The Guest extension device.
 * @param   session_ret   Where to store the session on success.
 * @param   user_session  Set if this is a session for the vboxuser device.
 */
int vbg_core_open_session(struct vbg_dev *gdev,
			  struct vbg_session **session_ret, bool user_session)
{
	struct vbg_session *session;
	unsigned long flags;

	session = kzalloc(sizeof(*session), GFP_KERNEL);
	if (!session)
		return -ENOMEM;

	session->gdev = gdev;
	session->user_session = user_session;

	spin_lock_irqsave(&gdev->session_spinlock, flags);
	list_add(&session->list_node, &gdev->session_list);
	spin_unlock_irqrestore(&gdev->session_spinlock, flags);

	*session_ret = session;

	return 0;
}

/**
 * Closes a VBoxGuest session.
 *
 * @param   session	The session to close (and free).
 */
void vbg_core_close_session(struct vbg_session *session)
{
	struct vbg_dev *gdev = session->gdev;
	unsigned long flags;
	int i;

	spin_lock_irqsave(&gdev->session_spinlock, flags);
	list_del(&session->list_node);
	spin_unlock_irqrestore(&gdev->session_spinlock, flags);

	vbg_set_session_capabilities(gdev, session, 0, U32_MAX, true);
	vbg_set_session_event_filter(gdev, session, 0, U32_MAX, true);

	for (i = 0; i < ARRAY_SIZE(session->hgcm_client_ids); i++) {
		if (session->hgcm_client_ids[i])
			vbg_hgcm_disconnect(gdev, session->hgcm_client_ids[i]);
	}

	kfree(session);
}

static bool vbg_wait_event_cond(struct vbg_dev *gdev,
				struct vbg_session *session,
				VBoxGuestWaitEventInfo *info)
{
	unsigned long flags;
	bool wakeup;
	u32 events;

	spin_lock_irqsave(&gdev->event_spinlock, flags);

	events = gdev->pending_events & info->u32EventMaskIn;
	wakeup = events || session->cancel_waiters;

	spin_unlock_irqrestore(&gdev->event_spinlock, flags);

	return wakeup;
}

/* Must be called with the event_lock held */
static u32 vbg_consume_events_locked(struct vbg_dev *gdev,
				     struct vbg_session *session,
				     VBoxGuestWaitEventInfo *info)
{
	u32 events = gdev->pending_events & info->u32EventMaskIn;

	gdev->pending_events &= ~events;
	return events;
}

static int vbg_ioctl_wait_event(struct vbg_dev *gdev,
				struct vbg_session *session,
				VBoxGuestWaitEventInfo *info)
{
	unsigned long flags;
	long timeout;
	int rc = VINF_SUCCESS;

	if (info->u32TimeoutIn == U32_MAX)
		timeout = MAX_SCHEDULE_TIMEOUT;
	else
		timeout = msecs_to_jiffies(info->u32TimeoutIn);

	info->u32Result = VBOXGUEST_WAITEVENT_OK;
	info->u32EventFlagsOut = 0;

	do {
		timeout = wait_event_interruptible_timeout(
				gdev->event_wq,
				vbg_wait_event_cond(gdev, session, info),
				timeout);

		spin_lock_irqsave(&gdev->event_spinlock, flags);

		if (timeout < 0 || session->cancel_waiters) {
			info->u32Result = VBOXGUEST_WAITEVENT_INTERRUPTED;
			rc = VERR_INTERRUPTED;
		} else if (timeout == 0) {
			info->u32Result = VBOXGUEST_WAITEVENT_TIMEOUT;
			rc = VERR_TIMEOUT;
		} else {
			info->u32EventFlagsOut =
			    vbg_consume_events_locked(gdev, session, info);
		}

		spin_unlock_irqrestore(&gdev->event_spinlock, flags);

		/*
		 * Someone else may have consumed the event(s) first, in
		 * which case we go back to waiting.
		 */
	} while (rc == VINF_SUCCESS && info->u32EventFlagsOut == 0);

	return rc;
}

static int vbg_ioctl_cancel_all_wait_events(struct vbg_dev *gdev,
					    struct vbg_session *session)
{
	unsigned long flags;

	spin_lock_irqsave(&gdev->event_spinlock, flags);
	session->cancel_waiters = true;
	spin_unlock_irqrestore(&gdev->event_spinlock, flags);

	wake_up(&gdev->event_wq);

	return VINF_SUCCESS;
}

/**
 * Checks if the VMM request is allowed in the context of the given session.
 *
 * @returns VBox status code
 * @param   gdev	The Guest extension device.
 * @param   session	The calling session.
 * @param   req		The request.
 */
static int vbg_req_allowed(struct vbg_dev *gdev, struct vbg_session *session,
			   VMMDevRequestHeader const *req)
{
	const VMMDevReportGuestStatus *guest_status;
	bool trusted_apps_only;

	switch (req->requestType) {
	/* Trusted users apps only. */
	case VMMDevReq_QueryCredentials:
	case VMMDevReq_ReportCredentialsJudgement:
	case VMMDevReq_RegisterSharedModule:
	case VMMDevReq_UnregisterSharedModule:
	case VMMDevReq_WriteCoreDump:
	case VMMDevReq_GetCpuHotPlugRequest:
	case VMMDevReq_SetCpuHotPlugStatus:
	case VMMDevReq_CheckSharedModules:
	case VMMDevReq_GetPageSharingStatus:
	case VMMDevReq_DebugIsPageShared:
	case VMMDevReq_ReportGuestStats:
	case VMMDevReq_ReportGuestUserState:
	case VMMDevReq_GetStatisticsChangeRequest:
	case VMMDevReq_ChangeMemBalloon:
		trusted_apps_only = true;
		break;

	/* Anyone. */
	case VMMDevReq_GetMouseStatus:
	case VMMDevReq_SetMouseStatus:
	case VMMDevReq_SetPointerShape:
	case VMMDevReq_GetHostVersion:
	case VMMDevReq_Idle:
	case VMMDevReq_GetHostTime:
	case VMMDevReq_SetPowerStatus:
	case VMMDevReq_AcknowledgeEvents:
	case VMMDevReq_CtlGuestFilterMask:
	case VMMDevReq_ReportGuestStatus:
	case VMMDevReq_GetDisplayChangeRequest:
	case VMMDevReq_VideoModeSupported:
	case VMMDevReq_GetHeightReduction:
	case VMMDevReq_GetDisplayChangeRequest2:
	case VMMDevReq_VideoModeSupported2:
	case VMMDevReq_VideoAccelEnable:
	case VMMDevReq_VideoAccelFlush:
	case VMMDevReq_VideoSetVisibleRegion:
	case VMMDevReq_GetDisplayChangeRequestEx:
	case VMMDevReq_GetSeamlessChangeRequest:
	case VMMDevReq_GetVRDPChangeRequest:
	case VMMDevReq_LogString:
	case VMMDevReq_GetSessionId:
		trusted_apps_only = false;
		break;

	/**
	 * @todo this have to be changed into an I/O control and the facilities
	 *    tracked in the session so they can automatically be failed when
	 *    the session terminates without reporting the new status.
	 *
	 * The information presented by IGuest is not reliable without this!
	 */
	/* Depends on the request parameters... */
	case VMMDevReq_ReportGuestCapabilities:
		guest_status = (const VMMDevReportGuestStatus *)req;
		switch (guest_status->guestStatus.facility) {
		case VBoxGuestFacilityType_All:
		case VBoxGuestFacilityType_VBoxGuestDriver:
			vbg_err("Denying userspace vmm report guest cap. call facility %#08x\n",
				guest_status->guestStatus.facility);
			return VERR_PERMISSION_DENIED;
		case VBoxGuestFacilityType_VBoxService:
			trusted_apps_only = true;
			break;
		case VBoxGuestFacilityType_VBoxTrayClient:
		case VBoxGuestFacilityType_Seamless:
		case VBoxGuestFacilityType_Graphics:
		default:
			trusted_apps_only = false;
			break;
		}
		break;

	/* Anything else is not allowed. */
	default:
		vbg_err("Denying userspace vmm call type %#08x\n",
			req->requestType);
		return VERR_PERMISSION_DENIED;
	}

	if (trusted_apps_only && session->user_session) {
		vbg_err("Denying userspace vmm call type %#08x through vboxuser device node\n",
			req->requestType);
		return VERR_PERMISSION_DENIED;
	}

	return VINF_SUCCESS;
}

static int vbg_ioctl_vmmrequest(struct vbg_dev *gdev,
				struct vbg_session *session,
				VMMDevRequestHeader *req, size_t req_size,
				size_t *req_size_ret)
{
	int rc;

	rc = vbg_req_verify(req, req_size);
	if (rc < 0)
		return rc;

	rc = vbg_req_allowed(gdev, session, req);
	if (rc < 0)
		return rc;

	rc = vbg_req_perform(gdev, req);
	if (rc >= 0) {
		WARN_ON(rc == VINF_HGCM_ASYNC_EXECUTE);
		*req_size_ret = req->size;
	}

	return rc;
}

static int vbg_ioctl_hgcm_connect(struct vbg_dev *gdev,
				  struct vbg_session *session,
				  VBoxGuestHGCMConnectInfo *info)
{
	unsigned long flags;
	u32 client_id;
	int i, rc;

	/* Find a free place in the sessions clients array and claim it */
	spin_lock_irqsave(&gdev->session_spinlock, flags);
	for (i = 0; i < ARRAY_SIZE(session->hgcm_client_ids); i++) {
		if (!session->hgcm_client_ids[i]) {
			session->hgcm_client_ids[i] = U32_MAX;
			break;
		}
	}
	spin_unlock_irqrestore(&gdev->session_spinlock, flags);

	if (i >= ARRAY_SIZE(session->hgcm_client_ids))
		return VERR_TOO_MANY_OPEN_FILES;

	rc = vbg_hgcm_connect(gdev, &info->Loc, &client_id);

	spin_lock_irqsave(&gdev->session_spinlock, flags);
	if (rc >= 0) {
		info->result = VINF_SUCCESS;
		info->u32ClientID = client_id;
		session->hgcm_client_ids[i] = client_id;
	} else {
		session->hgcm_client_ids[i] = 0;
	}
	spin_unlock_irqrestore(&gdev->session_spinlock, flags);

	return rc;
}

static int vbg_ioctl_hgcm_disconnect(struct vbg_dev *gdev,
				     struct vbg_session *session,
				     VBoxGuestHGCMDisconnectInfo *info)
{
	unsigned long flags;
	u32 client_id;
	int i, rc;

	client_id = info->u32ClientID;
	if (client_id == 0 || client_id == U32_MAX)
		return VERR_INVALID_HANDLE;

	spin_lock_irqsave(&gdev->session_spinlock, flags);
	for (i = 0; i < ARRAY_SIZE(session->hgcm_client_ids); i++) {
		if (session->hgcm_client_ids[i] == client_id) {
			session->hgcm_client_ids[i] = U32_MAX;
			break;
		}
	}
	spin_unlock_irqrestore(&gdev->session_spinlock, flags);

	if (i >= ARRAY_SIZE(session->hgcm_client_ids))
		return VERR_INVALID_HANDLE;

	rc = vbg_hgcm_disconnect(gdev, client_id);

	spin_lock_irqsave(&gdev->session_spinlock, flags);
	if (rc >= 0) {
		info->result = VINF_SUCCESS;
		session->hgcm_client_ids[i] = 0;
	} else {
		session->hgcm_client_ids[i] = client_id;
	}
	spin_unlock_irqrestore(&gdev->session_spinlock, flags);

	return rc;
}

static int vbg_ioctl_hgcm_call(struct vbg_dev *gdev,
			       struct vbg_session *session,
			       u32 timeout_ms, bool f32bit,
			       VBoxGuestHGCMCallInfo *info, size_t info_size,
			       size_t *info_size_ret)
{
	u32 client_id = info->u32ClientID;
	unsigned long flags;
	size_t actual_size;
	int i, rc;

	if (info_size < sizeof(VBoxGuestHGCMCallInfo))
		return VERR_BUFFER_OVERFLOW;

	if (info->cParms > VBOX_HGCM_MAX_PARMS)
		return VERR_INVALID_PARAMETER;

	if (client_id == 0 || client_id == U32_MAX)
		return VERR_INVALID_HANDLE;

	actual_size = sizeof(*info);
	if (f32bit)
		actual_size += info->cParms * sizeof(HGCMFunctionParameter32);
	else
		actual_size += info->cParms * sizeof(HGCMFunctionParameter);
	if (info_size < actual_size) {
		vbg_debug("VBOXGUEST_IOCTL_HGCM_CALL: info_size=%#zx (%zu) required size is %#zx (%zu)\n",
			  info_size, info_size, actual_size, actual_size);
		return VERR_INVALID_PARAMETER;
	}

	/*
	 * Validate the client id.
	 */
	spin_lock_irqsave(&gdev->session_spinlock, flags);
	for (i = 0; i < ARRAY_SIZE(session->hgcm_client_ids); i++)
		if (session->hgcm_client_ids[i] == client_id)
			break;
	spin_unlock_irqrestore(&gdev->session_spinlock, flags);
	if (i >= ARRAY_SIZE(session->hgcm_client_ids)) {
		vbg_debug("VBOXGUEST_IOCTL_HGCM_CALL: Invalid handle. u32Client=%#08x\n",
			  client_id);
		return VERR_INVALID_HANDLE;
	}

	if (f32bit)
		rc = vbg_hgcm_call32(gdev, info, info_size, timeout_ms, true);
	else
		rc = vbg_hgcm_call(gdev, info, info_size, timeout_ms, true);

	if (rc >= 0) {
		*info_size_ret = actual_size;
	} else if (rc == VERR_INTERRUPTED || rc == VERR_TIMEOUT ||
		   rc == VERR_OUT_OF_RANGE) {
		vbg_debug("VBOXGUEST_IOCTL_HGCM_CALL%s error: %d\n",
			  f32bit ? "32" : "64", rc);
	} else {
		vbg_err("VBOXGUEST_IOCTL_HGCM_CALL%s error: %d\n",
			f32bit ? "32" : "64", rc);
	}
	return rc;
}

/**
 * Handle a request for writing a core dump of the guest on the host.
 *
 * @returns VBox status code
 *
 * @param   gdev	The Guest extension device.
 * @param   info	The output buffer.
 */
static int vbg_ioctl_write_core_dump(struct vbg_dev *gdev,
				     VBoxGuestWriteCoreDump *info)
{
	VMMDevReqWriteCoreDump *req;
	int rc;

	req = vbg_req_alloc(sizeof(*req), VMMDevReq_WriteCoreDump);
	if (!req)
		return VERR_NO_MEMORY;

	req->fFlags = info->fFlags;
	rc = vbg_req_perform(gdev, req);

	kfree(req);
	return rc;
}

/**
 * Handle VBOXGUEST_IOCTL_CTL_FILTER_MASK.
 *
 * @returns VBox status code
 * @param   gdev	The Guest extension device.
 * @param   session	The session.
 * @param   info	The request.
 */
static int vbg_ioctl_ctl_filter_mask(struct vbg_dev *gdev,
				     struct vbg_session *session,
				     VBoxGuestFilterMaskInfo *info)
{
	if ((info->u32OrMask | info->u32NotMask) &
						~VMMDEV_EVENT_VALID_EVENT_MASK)
		return VERR_INVALID_PARAMETER;

	return vbg_set_session_event_filter(gdev, session, info->u32OrMask,
					    info->u32NotMask, false);
}

/**
 * Report guest supported mouse-features to the host.
 *
 * @returns 0 or negative errno value.
 * @returns VBox status code
 * @param   gdev	The Guest extension device.
 * @param   features	The set of features to report to the host.
 */
int vbg_core_set_mouse_status(struct vbg_dev *gdev, u32 features)
{
	VMMDevReqMouseStatus *req;
	int rc;

	req = vbg_req_alloc(sizeof(*req), VMMDevReq_SetMouseStatus);
	if (!req)
		return -ENOMEM;

	req->mouseFeatures = features;
	req->pointerXPos = 0;
	req->pointerYPos = 0;

	rc = vbg_req_perform(gdev, req);
	if (rc < 0)
		vbg_err("%s error: %d\n", __func__, rc);

	kfree(req);
	return vbg_status_code_to_errno(rc);
}

/**
 * Handle VBOXGUEST_IOCTL_SET_GUEST_CAPABILITIES.
 *
 * @returns VBox status code
 * @param   gdev	The Guest extension device.
 * @param   session	The session.
 * @param   info	The request.
 */
static int vbg_ioctl_set_capabilities(struct vbg_dev *gdev,
				      struct vbg_session *session,
				      VBoxGuestSetCapabilitiesInfo *info)
{
	if ((info->u32OrMask | info->u32NotMask) &
					~VMMDEV_GUEST_CAPABILITIES_MASK)
		return VERR_INVALID_PARAMETER;

	return vbg_set_session_capabilities(gdev, session, info->u32OrMask,
					    info->u32NotMask, false);
}

/**
 * Common IOCtl for user to kernel communication.
 *
 * This function only does the basic validation and then invokes
 * worker functions that takes care of each specific function.
 *
 * @returns VBox status code
 * @param   session        The client session.
 * @param   req            The requested function.
 * @param   data           The input/output data buffer.
 * @param   data_size      The max size of the data buffer.
 * @param   data_size_ret  Where to store the amount of returned data.
 */
int vbg_core_ioctl(struct vbg_session *session, unsigned int req, void *data,
		   size_t data_size, size_t *data_size_ret)
{
	VBoxGuestHGCMCallInfoTimed *timed = data;
	struct vbg_dev *gdev = session->gdev;
	bool f32bit = false;
	size_t offset;
	int rc;

	*data_size_ret = 0;

	/* All our ioctls are type 'V' dir RW */
	if (_IOC_TYPE(req) != 'V' || _IOC_DIR(req) != (_IOC_READ|_IOC_WRITE))
		return VERR_NOT_SUPPORTED;

	/* First check for variable sized ioctls by ioc number. */
	switch (_IOC_NR(req)) {
	case _IOC_NR(VBOXGUEST_IOCTL_VMMREQUEST(0)):
		return vbg_ioctl_vmmrequest(gdev, session,
					    data, data_size, data_size_ret);
#ifdef CONFIG_X86_64
	case _IOC_NR(VBOXGUEST_IOCTL_HGCM_CALL_32(0)):
		f32bit = true;
		/* Fall through */
#endif
	case _IOC_NR(VBOXGUEST_IOCTL_HGCM_CALL(0)):
		return vbg_ioctl_hgcm_call(gdev, session, U32_MAX, f32bit,
					   data, data_size, data_size_ret);
#ifdef CONFIG_X86_64
	case _IOC_NR(VBOXGUEST_IOCTL_HGCM_CALL_TIMED_32(0)):
		f32bit = true;
		/* Fall through */
#endif
	case _IOC_NR(VBOXGUEST_IOCTL_HGCM_CALL_TIMED(0)):
		offset = offsetof(VBoxGuestHGCMCallInfoTimed, info);
		rc = vbg_ioctl_hgcm_call(gdev, session,
					 timed->u32Timeout, f32bit,
					 data + offset, data_size - offset,
					 data_size_ret);
		*data_size_ret += offset;
		return rc;
	case _IOC_NR(VBOXGUEST_IOCTL_LOG(0)):
		vbg_info("%.*s", (int)data_size, (char *)data);
		return VINF_SUCCESS;
	}

	/* Handle fixed size requests. */
	*data_size_ret = data_size;

	switch (req) {
	case VBOXGUEST_IOCTL_WAITEVENT:
		return vbg_ioctl_wait_event(gdev, session, data);
	case VBOXGUEST_IOCTL_CANCEL_ALL_WAITEVENTS:
		return vbg_ioctl_cancel_all_wait_events(gdev, session);
	case VBOXGUEST_IOCTL_CTL_FILTER_MASK:
		return vbg_ioctl_ctl_filter_mask(gdev, session, data);
	case VBOXGUEST_IOCTL_HGCM_CONNECT:
#ifdef CONFIG_X86_64 /* Needed because these are identical on 32 bit builds */
	case VBOXGUEST_IOCTL_HGCM_CONNECT_32:
#endif
		return vbg_ioctl_hgcm_connect(gdev, session, data);
	case VBOXGUEST_IOCTL_HGCM_DISCONNECT:
#ifdef CONFIG_X86_64
	case VBOXGUEST_IOCTL_HGCM_DISCONNECT_32:
#endif
		return vbg_ioctl_hgcm_disconnect(gdev, session, data);
	case VBOXGUEST_IOCTL_CHECK_BALLOON:
		/*
		 * Under Linux we handle VMMDEV_EVENT_BALLOON_CHANGE_REQUEST
		 * events entirely in the kernel, see vbg_core_isr().
		 */
		return VINF_SUCCESS;
	case VBOXGUEST_IOCTL_CHANGE_BALLOON:
		/* Under Linux we always handle the balloon in R0. */
		return VERR_PERMISSION_DENIED;
	case VBOXGUEST_IOCTL_WRITE_CORE_DUMP:
		return vbg_ioctl_write_core_dump(gdev, data);
	case VBOXGUEST_IOCTL_SET_MOUSE_STATUS:
		vbg_err("VGDrvCommonIoCtl: VBOXGUEST_IOCTL_SET_MOUSE_STATUS should not be used under Linux\n");
		return VERR_NOT_SUPPORTED;
	case VBOXGUEST_IOCTL_GUEST_CAPS_ACQUIRE:
		vbg_err("VGDrvCommonIoCtl: VBOXGUEST_IOCTL_GUEST_CAPS_ACQUIRE should not be used under Linux\n");
		return VERR_NOT_SUPPORTED;
	case VBOXGUEST_IOCTL_SET_GUEST_CAPABILITIES:
		return vbg_ioctl_set_capabilities(gdev, session, data);
	}

	vbg_debug("VGDrvCommonIoCtl: Unknown req %#08x\n", req);
	return VERR_NOT_SUPPORTED;
}

/** Core interrupt service routine. */
irqreturn_t vbg_core_isr(int irq, void *dev_id)
{
	struct vbg_dev *gdev = dev_id;
	VMMDevEvents *req = gdev->ack_events_req;
	bool mouse_position_changed = false;
	unsigned long flags;
	u32 events = 0;
	int rc;

	if (!gdev->mmio->V.V1_04.fHaveEvents)
		return IRQ_NONE;

	/* Get and acknowlegde events. */
	req->header.rc = VERR_INTERNAL_ERROR;
	req->events = 0;
	rc = vbg_req_perform(gdev, req);
	if (rc < 0) {
		vbg_err("Error performing VMMDevEvents req: %d\n", rc);
		return IRQ_NONE;
	}

	events = req->events;

	if (events & VMMDEV_EVENT_MOUSE_POSITION_CHANGED) {
		mouse_position_changed = true;
		events &= ~VMMDEV_EVENT_MOUSE_POSITION_CHANGED;
	}

	if (events & VMMDEV_EVENT_HGCM) {
		wake_up(&gdev->hgcm_wq);
		events &= ~VMMDEV_EVENT_HGCM;
	}

	if (events & VMMDEV_EVENT_BALLOON_CHANGE_REQUEST) {
		schedule_work(&gdev->mem_balloon.work);
		events &= ~VMMDEV_EVENT_BALLOON_CHANGE_REQUEST;
	}

	if (events) {
		spin_lock_irqsave(&gdev->event_spinlock, flags);
		gdev->pending_events |= events;
		spin_unlock_irqrestore(&gdev->event_spinlock, flags);

		wake_up(&gdev->event_wq);
	}

	if (mouse_position_changed)
		vbg_linux_mouse_event(gdev);

	return IRQ_HANDLED;
}
