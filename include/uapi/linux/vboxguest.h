/*
 * VBoxGuest - VirtualBox Guest Additions Driver Interface. (ADD,DEV)
 *
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

#ifndef __UAPI_VBOXGUEST_H__
#define __UAPI_VBOXGUEST_H__

#include <asm/bitsperlong.h>
#include <linux/ioctl.h>
#include <linux/vbox_vmmdev.h> /* For HGCMServiceLocation */

/**
 * @defgroup grp_vboxguest  VirtualBox Guest Additions Device Driver
 *
 * Also know as VBoxGuest.
 *
 * @{
 */

/**
 * @defgroup grp_vboxguest_ioc  VirtualBox Guest Additions Driver Interface
 * @{
 */

/**
 * @name VBoxGuest IOCTL codes and structures.
 *
 * The range 0..15 is for basic driver communication.
 * The range 16..31 is for HGCM communication.
 * The range 32..47 is reserved for future use.
 * The range 48..63 is for OS specific communication.
 * The 7th bit is reserved for future hacks.
 * The 8th bit is reserved for distinguishing between 32-bit and 64-bit
 * processes in future 64-bit guest additions.
 * @{
 */
#if __BITS_PER_LONG == 64
#define VBOXGUEST_IOCTL_FLAG		128
#else
#define VBOXGUEST_IOCTL_FLAG		0
#endif
/** @} */

#define VBOXGUEST_IOCTL_CODE_(function, size) \
	_IOC(_IOC_READ|_IOC_WRITE, 'V', (function), (size))

#define VBOXGUEST_IOCTL_CODE(function, size) \
	VBOXGUEST_IOCTL_CODE_((function) | VBOXGUEST_IOCTL_FLAG, (size))
/* Define 32 bit codes to support 32 bit applications in 64 bit guest driver. */
#define VBOXGUEST_IOCTL_CODE_32(function, size) \
	VBOXGUEST_IOCTL_CODE_((function), (size))

/** IOCTL to VBoxGuest to wait for a VMMDev host notification */
#define VBOXGUEST_IOCTL_WAITEVENT \
	VBOXGUEST_IOCTL_CODE_(2, sizeof(VBoxGuestWaitEventInfo))

/**
 * @name Result codes for VBoxGuestWaitEventInfo::u32Result
 * @{
 */
/** Successful completion, an event occurred. */
#define VBOXGUEST_WAITEVENT_OK          (0)
/** Successful completion, timed out. */
#define VBOXGUEST_WAITEVENT_TIMEOUT     (1)
/** Wait was interrupted. */
#define VBOXGUEST_WAITEVENT_INTERRUPTED (2)
/** An error occurred while processing the request. */
#define VBOXGUEST_WAITEVENT_ERROR       (3)
/** @} */

/** Input and output buffers layout of the IOCTL_VBOXGUEST_WAITEVENT */
typedef struct VBoxGuestWaitEventInfo {
	/** timeout in milliseconds */
	__u32 u32TimeoutIn;
	/** events to wait for */
	__u32 u32EventMaskIn;
	/** result code */
	__u32 u32Result;
	/** events occurred */
	__u32 u32EventFlagsOut;
} VBoxGuestWaitEventInfo;
VMMDEV_ASSERT_SIZE(VBoxGuestWaitEventInfo, 16);


/**
 * IOCTL to VBoxGuest to perform a VMM request
 * @remark  The data buffer for this IOCtl has an variable size, keep this in
 *          mind on systems where this matters.
 */
#define VBOXGUEST_IOCTL_VMMREQUEST(size) \
	VBOXGUEST_IOCTL_CODE_(3, (size))


/** IOCTL to VBoxGuest to control event filter mask. */
#define VBOXGUEST_IOCTL_CTL_FILTER_MASK \
	VBOXGUEST_IOCTL_CODE_(4, sizeof(VBoxGuestFilterMaskInfo))

/** Input and output buffer layout of the IOCTL_VBOXGUEST_CTL_FILTER_MASK. */
typedef struct VBoxGuestFilterMaskInfo {
	__u32 u32OrMask;
	__u32 u32NotMask;
} VBoxGuestFilterMaskInfo;
VMMDEV_ASSERT_SIZE(VBoxGuestFilterMaskInfo, 8);

/**
 * IOCTL to VBoxGuest to interrupt (cancel) any pending WAITEVENTs and return.
 * Handled inside the guest additions and not seen by the host at all.
 * After calling this, VBOXGUEST_IOCTL_WAITEVENT should no longer be called in
 * the same session. Any VBOXGUEST_IOCTL_WAITEVENT calls in the same session
 * done after calling this will directly exit with VERR_INTERRUPTED.
 * @see VBOXGUEST_IOCTL_WAITEVENT
 */
#define VBOXGUEST_IOCTL_CANCEL_ALL_WAITEVENTS \
	VBOXGUEST_IOCTL_CODE_(5, 0)

/**
 * IOCTL to VBoxGuest to perform backdoor logging.
 * The argument is a string buffer of the specified size.
 */
#define VBOXGUEST_IOCTL_LOG(size) \
	VBOXGUEST_IOCTL_CODE_(6, (size))

/**
 * IOCTL to VBoxGuest to check memory ballooning.  The guest kernel module /
 * device driver will ask the host for the current size of the balloon and
 * adjust the size. Or it will set fHandledInR0 = false and R3 is responsible
 * for allocating memory and calling R0 (VBOXGUEST_IOCTL_CHANGE_BALLOON).
 */
#define VBOXGUEST_IOCTL_CHECK_BALLOON \
	VBOXGUEST_IOCTL_CODE_(7, sizeof(VBoxGuestCheckBalloonInfo))

/** Output buffer layout of the VBOXGUEST_IOCTL_CHECK_BALLOON. */
typedef struct VBoxGuestCheckBalloonInfo {
	/** The size of the balloon in chunks of 1MB. */
	__u32 cBalloonChunks;
	/**
	 * false = handled in R0, no further action required.
	 * true = allocate balloon memory in R3.
	 */
	__u32 fHandleInR3;
} VBoxGuestCheckBalloonInfo;
VMMDEV_ASSERT_SIZE(VBoxGuestCheckBalloonInfo, 8);

/**
 * IOCTL to VBoxGuest to supply or revoke one chunk for ballooning.
 * The guest kernel module / device driver will lock down supplied memory or
 * unlock reclaimed memory and then forward the physical addresses of the
 * changed balloon chunk to the host.
 */
#define VBOXGUEST_IOCTL_CHANGE_BALLOON \
	VBOXGUEST_IOCTL_CODE_(8, sizeof(VBoxGuestChangeBalloonInfo))

/**
 * Input buffer layout of the VBOXGUEST_IOCTL_CHANGE_BALLOON request.
 * Information about a memory chunk used to inflate or deflate the balloon.
 */
typedef struct VBoxGuestChangeBalloonInfo {
	/** Address of the chunk. */
	__u64 u64ChunkAddr;
	/** true = inflate, false = deflate. */
	__u32 fInflate;
	/** Alignment padding. */
	__u32 u32Align;
} VBoxGuestChangeBalloonInfo;
VMMDEV_ASSERT_SIZE(VBoxGuestChangeBalloonInfo, 16);

/** IOCTL to VBoxGuest to write guest core. */
#define VBOXGUEST_IOCTL_WRITE_CORE_DUMP \
	VBOXGUEST_IOCTL_CODE(9, sizeof(VBoxGuestWriteCoreDump))

/** Input and output buffer layout of the VBOXGUEST_IOCTL_WRITE_CORE request. */
typedef struct VBoxGuestWriteCoreDump {
	/** Flags (reserved, MBZ). */
	__u32 fFlags;
} VBoxGuestWriteCoreDump;
VMMDEV_ASSERT_SIZE(VBoxGuestWriteCoreDump, 4);

/** IOCTL to VBoxGuest to update the mouse status features. */
#define VBOXGUEST_IOCTL_SET_MOUSE_STATUS \
	VBOXGUEST_IOCTL_CODE_(10, sizeof(__u32))

/** IOCTL to VBoxGuest to connect to a HGCM service. */
#define VBOXGUEST_IOCTL_HGCM_CONNECT \
	VBOXGUEST_IOCTL_CODE(16, sizeof(VBoxGuestHGCMConnectInfo))

/**
 * HGCM connect info structure.
 *
 * This is used by VBOXGUEST_IOCTL_HGCM_CONNECT.
 */
struct VBoxGuestHGCMConnectInfo {
	__s32 result;               /**< OUT */
	HGCMServiceLocation Loc;  /**< IN */
	__u32 u32ClientID;          /**< OUT */
} __packed;
typedef struct VBoxGuestHGCMConnectInfo VBoxGuestHGCMConnectInfo;
VMMDEV_ASSERT_SIZE(VBoxGuestHGCMConnectInfo, 4+4+128+4);

/** IOCTL to VBoxGuest to disconnect from a HGCM service. */
#define VBOXGUEST_IOCTL_HGCM_DISCONNECT \
	VBOXGUEST_IOCTL_CODE(17, sizeof(VBoxGuestHGCMDisconnectInfo))

/**
 * HGCM disconnect info structure.
 *
 * This is used by VBOXGUEST_IOCTL_HGCM_DISCONNECT.
 */
typedef struct VBoxGuestHGCMDisconnectInfo {
	__s32 result;          /**< OUT */
	__u32 u32ClientID;     /**< IN */
} VBoxGuestHGCMDisconnectInfo;
VMMDEV_ASSERT_SIZE(VBoxGuestHGCMDisconnectInfo, 8);

/**
 * IOCTL to VBoxGuest to make a call to a HGCM service.
 * @see VBoxGuestHGCMCallInfo
 */
#define VBOXGUEST_IOCTL_HGCM_CALL(size) \
	VBOXGUEST_IOCTL_CODE(18, (size))

/**
 * HGCM call info structure.
 *
 * This is used by VBOXGUEST_IOCTL_HGCM_CALL.
 */
typedef struct VBoxGuestHGCMCallInfo {
	__s32 result;          /**< OUT Host HGCM return code.*/
	__u32 u32ClientID;     /**< IN  The id of the caller. */
	__u32 u32Function;     /**< IN  Function number. */
	__u32 cParms;          /**< IN  How many parms. */
	/* Parameters follow in form HGCMFunctionParameter aParms[cParms] */
} VBoxGuestHGCMCallInfo;
VMMDEV_ASSERT_SIZE(VBoxGuestHGCMCallInfo, 16);

/** IOCTL to VBoxGuest to make a timed call to a HGCM service. */
#define VBOXGUEST_IOCTL_HGCM_CALL_TIMED(size) \
	VBOXGUEST_IOCTL_CODE(20, (size))

/**
 * HGCM call info structure.
 *
 * This is used by VBOXGUEST_IOCTL_HGCM_CALL_TIMED.
 */
struct VBoxGuestHGCMCallInfoTimed {
	/** IN  How long to wait for completion before cancelling the call. */
	__u32 u32Timeout;
	/** IN  Is this request interruptible? */
	__u32 fInterruptible;
	/**
	 * IN/OUT The rest of the call information.  Placed after the timeout
	 * so that the parameters follow as they would for a normal call.
	 */
	VBoxGuestHGCMCallInfo info;
	/* Parameters follow in form HGCMFunctionParameter aParms[cParms] */
} __packed;
typedef struct VBoxGuestHGCMCallInfoTimed VBoxGuestHGCMCallInfoTimed;
VMMDEV_ASSERT_SIZE(VBoxGuestHGCMCallInfoTimed, 8+16);

/**
 * @name IOCTL numbers that 32-bit clients, like the Windows OpenGL guest
 *        driver, will use when talking to a 64-bit driver.
 * @remarks These are only used by the driver implementation!
 * @{
 */
#define VBOXGUEST_IOCTL_HGCM_CONNECT_32 \
	VBOXGUEST_IOCTL_CODE_32(16, sizeof(VBoxGuestHGCMConnectInfo))
#define VBOXGUEST_IOCTL_HGCM_DISCONNECT_32 \
	VBOXGUEST_IOCTL_CODE_32(17, sizeof(VBoxGuestHGCMDisconnectInfo))
#define VBOXGUEST_IOCTL_HGCM_CALL_32(size) \
	VBOXGUEST_IOCTL_CODE_32(18, (size))
#define VBOXGUEST_IOCTL_HGCM_CALL_TIMED_32(size) \
	VBOXGUEST_IOCTL_CODE_32(20, (size))
/** @} */

/** Get the pointer to the first HGCM parameter.  */
#define VBOXGUEST_HGCM_CALL_PARMS(a) \
    ((HGCMFunctionParameter *)((__u8 *)(a) + sizeof(VBoxGuestHGCMCallInfo)))
/** Get the pointer to the first HGCM parameter in a 32-bit request.  */
#define VBOXGUEST_HGCM_CALL_PARMS32(a) \
    ((HGCMFunctionParameter32 *)((__u8 *)(a) + sizeof(VBoxGuestHGCMCallInfo)))

typedef enum VBOXGUESTCAPSACQUIRE_FLAGS {
	VBOXGUESTCAPSACQUIRE_FLAGS_NONE = 0,
	/*
	 * Configures VBoxGuest to use the specified caps in Acquire mode, w/o
	 * making any caps acquisition/release. so far it is only possible to
	 * set acquire mode for caps, but not clear it, so u32NotMask is
	 * ignored for this request.
	 */
	VBOXGUESTCAPSACQUIRE_FLAGS_CONFIG_ACQUIRE_MODE,
	/* To ensure enum is 32bit */
	VBOXGUESTCAPSACQUIRE_FLAGS_32bit = 0x7fffffff
} VBOXGUESTCAPSACQUIRE_FLAGS;

typedef struct VBoxGuestCapsAquire {
	/*
	 * result status
	 * VINF_SUCCESS - on success
	 * VERR_RESOURCE_BUSY - some caps in the u32OrMask are acquired by some
	 *	other VBoxGuest connection. NOTE: no u32NotMask caps are cleaned
	 *	in this case, No modifications are done on failure.
	 * VER_INVALID_PARAMETER - invalid Caps are specified with either
	 *	u32OrMask or u32NotMask. No modifications are done on failure.
	 */
	__s32 rc;
	/* Acquire command */
	VBOXGUESTCAPSACQUIRE_FLAGS enmFlags;
	/* caps to acquire, OR-ed VMMDEV_GUEST_SUPPORTS_XXX flags */
	__u32 u32OrMask;
	/* caps to release, OR-ed VMMDEV_GUEST_SUPPORTS_XXX flags */
	__u32 u32NotMask;
} VBoxGuestCapsAquire;

/**
 * IOCTL to for Acquiring/Releasing Guest Caps
 * This is used for multiple purposes:
 * 1. By doing Acquire r3 client application (e.g. VBoxTray) claims it will use
 *    the given connection for performing operations like Auto-resize, or
 *    Seamless. If the application terminates, the driver will automatically
 *    cleanup the caps reported to host, so that host knows guest does not
 *    support them anymore.
 * 2. In a multy-user environment this will not allow r3 applications (like
 *    VBoxTray) running in different user sessions simultaneously to interfere
 *    with each other. An r3 client application (like VBoxTray) is responsible
 *    for Acquiring/Releasing caps properly as needed.
 **/
#define VBOXGUEST_IOCTL_GUEST_CAPS_ACQUIRE \
	VBOXGUEST_IOCTL_CODE(32, sizeof(VBoxGuestCapsAquire))

/** IOCTL to VBoxGuest to set guest capabilities. */
#define VBOXGUEST_IOCTL_SET_GUEST_CAPABILITIES \
	VBOXGUEST_IOCTL_CODE_(33, sizeof(VBoxGuestSetCapabilitiesInfo))

/** Input/output buffer layout for VBOXGUEST_IOCTL_SET_GUEST_CAPABILITIES. */
typedef struct VBoxGuestSetCapabilitiesInfo {
	__u32 u32OrMask;
	__u32 u32NotMask;
} VBoxGuestSetCapabilitiesInfo;
VMMDEV_ASSERT_SIZE(VBoxGuestSetCapabilitiesInfo, 8);

/** @} */

/** @} */

#endif
