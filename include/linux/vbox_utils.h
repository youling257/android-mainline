/*
 * Copyright (C) 2006-2016 Oracle Corporation
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

#ifndef __VBOX_UTILS_H__
#define __VBOX_UTILS_H__

#include <linux/printk.h>
#include <linux/vbox_vmmdev.h>
#include <linux/vboxguest.h>

struct vbg_dev;

/**
 * vboxguest logging functions, these log both to the backdoor and call
 * the equivalent kernel pr_foo function.
 */
__printf(1, 2) void vbg_info(const char *fmt, ...);
__printf(1, 2) void vbg_warn(const char *fmt, ...);
__printf(1, 2) void vbg_err(const char *fmt, ...);

/* Only use backdoor logging for non-dynamic debug builds */
#if defined(DEBUG) && !defined(CONFIG_DYNAMIC_DEBUG)
__printf(1, 2) void vbg_debug(const char *fmt, ...);
#else
#define vbg_debug pr_debug
#endif

/** @name Generic request functions.
 * @{
 */

/**
 * Allocate memory for generic request and initialize the request header.
 *
 * @returns the allocated memory
 * @param   len		Size of memory block required for the request.
 * @param   req_type	The generic request type.
 */
void *vbg_req_alloc(size_t len, VMMDevRequestType req_type);

/**
 * Perform a generic request.
 *
 * @returns VBox status code
 * @param   gdev	The Guest extension device.
 * @param   req		Pointer the request structure.
 */
int vbg_req_perform(struct vbg_dev *gdev, void *req);

/**
 * Verify the generic request header.
 *
 * @returns VBox status code
 * @param   req		Pointer the request header structure.
 * @param   buffer_size Size of the request memory block. It should be equal to
 *			the request size for fixed size requests. It can be
 *			greater for variable size requests.
 */
int vbg_req_verify(const VMMDevRequestHeader *req, size_t buffer_size);
/** @} */

int vbg_hgcm_connect(struct vbg_dev *gdev, HGCMServiceLocation *loc,
		     u32 *client_id);

int vbg_hgcm_disconnect(struct vbg_dev *gdev, u32 client_id);

int vbg_hgcm_call(struct vbg_dev *gdev,
		  VBoxGuestHGCMCallInfo *pCallInfo, u32 cbCallInfo,
		  u32 timeout_ms, bool is_user);

int vbg_hgcm_call32(struct vbg_dev *gdev,
		    VBoxGuestHGCMCallInfo *pCallInfo, u32 cbCallInfo,
		    u32 timeout_ms, bool is_user);

/**
 * Convert a VirtualBox status code to a standard Linux kernel return value.
 * @returns 0 or negative errno value.
 * @param   rc		VirtualBox status code to convert.
 */
int vbg_status_code_to_errno(int rc);

/**
 * Helper for the vboxsf driver to get a reference to the guest device.
 * @returns a pointer to the gdev; or a ERR_PTR value on error.
 */
struct vbg_dev *vbg_get_gdev(void);

/**
 * Helper for the vboxsf driver to put a guest device reference.
 * @param   gdev	Reference returned by vbg_get_gdev to put.
 */
void vbg_put_gdev(struct vbg_dev *gdev);

#endif
