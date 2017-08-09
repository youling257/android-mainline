/*
 * Virtual Device for Guest <-> VMM/Host communication (ADD,DEV).
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

#ifndef __UAPI_VBOX_VMMDEV_H__
#define __UAPI_VBOX_VMMDEV_H__

#include <asm/bitsperlong.h>
#include <linux/types.h>
#include <linux/vbox_ostypes.h>

/*
 * We cannot use linux' compiletime_assert here because it expects to be used
 * inside a function only. Use a typedef to a char array with a negative size.
 */
#define VMMDEV_ASSERT_SIZE(type, size) \
	typedef char type ## _assert_size[1 - 2*!!(sizeof(type) != (size))]
#define VMMDEV_ASSERT_MEMBER_OFFSET(type, member, offset) \
	typedef char type ## _ ## member ## _assert_member_offset \
	[1 - 2*!!(offsetof(type, member) != (offset))]

/*
 * The host expects dwords / 32 bit packing. Using __aligned(4) everywhere is
 * not really practical and also does not seem to work. Specifically I've been
 * unable to get some structs (HGCMFunctionParameter32|64) to compile to the
 * right size using __aligned(), so we're sticking with pragma pack(4) here.
 */
#pragma pack(4)

/**
 * @defgroup grp_vmmdev    VMM Device
 *
 * @note This interface cannot be changed, it can only be extended!
 *
 * @{
 */

/** Port for generic request interface (relative offset). */
#define VMMDEV_PORT_OFF_REQUEST                             0

/**
 * @name VMMDev events.
 *
 * Used mainly by VMMDevReq_AcknowledgeEvents/VMMDevEvents and version 1.3 of
 * VMMDevMemory.
 *
 * @{
 */
/** Host mouse capabilities has been changed. */
#define VMMDEV_EVENT_MOUSE_CAPABILITIES_CHANGED             BIT(0)
/** HGCM event. */
#define VMMDEV_EVENT_HGCM                                   BIT(1)
/** A display change request has been issued. */
#define VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST                 BIT(2)
/** Credentials are available for judgement. */
#define VMMDEV_EVENT_JUDGE_CREDENTIALS                      BIT(3)
/** The guest has been restored. */
#define VMMDEV_EVENT_RESTORED                               BIT(4)
/** Seamless mode state changed. */
#define VMMDEV_EVENT_SEAMLESS_MODE_CHANGE_REQUEST           BIT(5)
/** Memory balloon size changed. */
#define VMMDEV_EVENT_BALLOON_CHANGE_REQUEST                 BIT(6)
/** Statistics interval changed. */
#define VMMDEV_EVENT_STATISTICS_INTERVAL_CHANGE_REQUEST     BIT(7)
/** VRDP status changed. */
#define VMMDEV_EVENT_VRDP                                   BIT(8)
/** New mouse position data available. */
#define VMMDEV_EVENT_MOUSE_POSITION_CHANGED                 BIT(9)
/** CPU hotplug event occurred. */
#define VMMDEV_EVENT_CPU_HOTPLUG                            BIT(10)
/** The mask of valid events, for sanity checking. */
#define VMMDEV_EVENT_VALID_EVENT_MASK                       0x000007ffU
/** @} */

/** @defgroup grp_vmmdev_req    VMMDev Generic Request Interface
 * @{
 */

/** @name Current version of the VMMDev interface.
 *
 * Additions are allowed to work only if
 * additions_major == vmmdev_current && additions_minor <= vmmdev_current.
 * Additions version is reported to host (VMMDev) by VMMDevReq_ReportGuestInfo.
 *
 * @remarks These defines also live in the 16-bit and assembly versions of this
 *          header.
 */
#define VMMDEV_VERSION                      0x00010004
#define VMMDEV_VERSION_MAJOR                (VMMDEV_VERSION >> 16)
#define VMMDEV_VERSION_MINOR                (VMMDEV_VERSION & 0xffff)
/** @} */

/** Maximum request packet size. */
#define VMMDEV_MAX_VMMDEVREQ_SIZE           1048576
/** Maximum number of HGCM parameters. */
#define VMMDEV_MAX_HGCM_PARMS               1024
/** Maximum total size of hgcm buffers in one call. */
#define VMMDEV_MAX_HGCM_DATA_SIZE           0x7fffffffU

/**
 * VMMDev request types.
 * @note when updating this, adjust vmmdevGetRequestSize() as well
 */
typedef enum {
	VMMDevReq_InvalidRequest             =  0,
	VMMDevReq_GetMouseStatus             =  1,
	VMMDevReq_SetMouseStatus             =  2,
	VMMDevReq_SetPointerShape            =  3,
	VMMDevReq_GetHostVersion             =  4,
	VMMDevReq_Idle                       =  5,
	VMMDevReq_GetHostTime                = 10,
	VMMDevReq_GetHypervisorInfo          = 20,
	VMMDevReq_SetHypervisorInfo          = 21,
	VMMDevReq_RegisterPatchMemory        = 22, /* since version 3.0.6 */
	VMMDevReq_DeregisterPatchMemory      = 23, /* since version 3.0.6 */
	VMMDevReq_SetPowerStatus             = 30,
	VMMDevReq_AcknowledgeEvents          = 41,
	VMMDevReq_CtlGuestFilterMask         = 42,
	VMMDevReq_ReportGuestInfo            = 50,
	VMMDevReq_ReportGuestInfo2           = 58, /* since version 3.2.0 */
	VMMDevReq_ReportGuestStatus          = 59, /* since version 3.2.8 */
	VMMDevReq_ReportGuestUserState       = 74, /* since version 4.3 */
	/**
	 * Retrieve a display resize request sent by the host using
	 * @a IDisplay:setVideoModeHint.  Deprecated.
	 *
	 * Similar to @a VMMDevReq_GetDisplayChangeRequest2, except that it only
	 * considers host requests sent for the first virtual display. This
	 * guest-req should not be used in new guest code, and the results are
	 * undefined if a guest mixes calls to this and
	 * @a VMMDevReq_GetDisplayChangeRequest2.
	 */
	VMMDevReq_GetDisplayChangeRequest    = 51,
	VMMDevReq_VideoModeSupported         = 52,
	VMMDevReq_GetHeightReduction         = 53,
	/**
	 * Retrieve a display resize request sent by the host using
	 * @a IDisplay:setVideoModeHint.
	 *
	 * Queries a display resize request sent from the host.  If the
	 * @a eventAck member is sent to true and there is an unqueried request
	 * available for one of the virtual display then that request will
	 * be returned.  If several displays have unqueried requests the lowest
	 * numbered display will be chosen first.  Only the most recent unseen
	 * request for each display is remembered.
	 * If @a eventAck is set to false, the last host request queried with
	 * @a eventAck set is resent, or failing that the most recent received
	 * from the host.  If no host request was ever received then all zeros
	 * are returned.
	 */
	VMMDevReq_GetDisplayChangeRequest2   = 54,
	VMMDevReq_ReportGuestCapabilities    = 55,
	VMMDevReq_SetGuestCapabilities       = 56,
	VMMDevReq_VideoModeSupported2        = 57, /* since version 3.2.0 */
	VMMDevReq_GetDisplayChangeRequestEx  = 80, /* since version 4.2.4 */
	VMMDevReq_HGCMConnect                = 60,
	VMMDevReq_HGCMDisconnect             = 61,
	VMMDevReq_HGCMCall32                 = 62,
	VMMDevReq_HGCMCall64                 = 63,
	VMMDevReq_HGCMCancel                 = 64,
	VMMDevReq_HGCMCancel2                = 65,
	VMMDevReq_VideoAccelEnable           = 70,
	VMMDevReq_VideoAccelFlush            = 71,
	VMMDevReq_VideoSetVisibleRegion      = 72,
	VMMDevReq_GetSeamlessChangeRequest   = 73,
	VMMDevReq_QueryCredentials           = 100,
	VMMDevReq_ReportCredentialsJudgement = 101,
	VMMDevReq_ReportGuestStats           = 110,
	VMMDevReq_GetMemBalloonChangeRequest = 111,
	VMMDevReq_GetStatisticsChangeRequest = 112,
	VMMDevReq_ChangeMemBalloon           = 113,
	VMMDevReq_GetVRDPChangeRequest       = 150,
	VMMDevReq_LogString                  = 200,
	VMMDevReq_GetCpuHotPlugRequest       = 210,
	VMMDevReq_SetCpuHotPlugStatus        = 211,
	VMMDevReq_RegisterSharedModule       = 212,
	VMMDevReq_UnregisterSharedModule     = 213,
	VMMDevReq_CheckSharedModules         = 214,
	VMMDevReq_GetPageSharingStatus       = 215,
	VMMDevReq_DebugIsPageShared          = 216,
	VMMDevReq_GetSessionId               = 217, /* since version 3.2.8 */
	VMMDevReq_WriteCoreDump              = 218,
	VMMDevReq_GuestHeartbeat             = 219,
	VMMDevReq_HeartbeatConfigure         = 220,
	VMMDevReq_SizeHack                   = 0x7fffffff
} VMMDevRequestType;

#if __BITS_PER_LONG == 64
#define VMMDevReq_HGCMCall VMMDevReq_HGCMCall64
#else
#define VMMDevReq_HGCMCall VMMDevReq_HGCMCall32
#endif

/** Version of VMMDevRequestHeader structure. */
#define VMMDEV_REQUEST_HEADER_VERSION (0x10001)

/**
 * Generic VMMDev request header.
 */
typedef struct {
	/** IN: Size of the structure in bytes (including body). */
	__u32 size;
	/** IN: Version of the structure.  */
	__u32 version;
	/** IN: Type of the request. */
	VMMDevRequestType requestType;
	/** OUT: Return code. */
	__s32 rc;
	/** Reserved field no.1. MBZ. */
	__u32 reserved1;
	/** Reserved field no.2. MBZ. */
	__u32 reserved2;
} VMMDevRequestHeader;
VMMDEV_ASSERT_SIZE(VMMDevRequestHeader, 24);

/**
 * Mouse status request structure.
 *
 * Used by VMMDevReq_GetMouseStatus and VMMDevReq_SetMouseStatus.
 */
typedef struct {
	/** header */
	VMMDevRequestHeader header;
	/** Mouse feature mask. See VMMDEV_MOUSE_*. */
	__u32 mouseFeatures;
	/** Mouse x position. */
	__s32 pointerXPos;
	/** Mouse y position. */
	__s32 pointerYPos;
} VMMDevReqMouseStatus;
VMMDEV_ASSERT_SIZE(VMMDevReqMouseStatus, 24+12);

/**
 * @name Mouse capability bits (VMMDevReqMouseStatus::mouseFeatures).
 * @{
 */
/** The guest can (== wants to) handle absolute coordinates.  */
#define VMMDEV_MOUSE_GUEST_CAN_ABSOLUTE                     BIT(0)
/**
 * The host can (== wants to) send absolute coordinates.
 * (Input not captured.)
 */
#define VMMDEV_MOUSE_HOST_WANTS_ABSOLUTE                    BIT(1)
/**
 * The guest can *NOT* switch to software cursor and therefore depends on the
 * host cursor.
 *
 * When guest additions are installed and the host has promised to display the
 * cursor itself, the guest installs a hardware mouse driver. Don't ask the
 * guest to switch to a software cursor then.
 */
#define VMMDEV_MOUSE_GUEST_NEEDS_HOST_CURSOR                BIT(2)
/** The host does NOT provide support for drawing the cursor itself. */
#define VMMDEV_MOUSE_HOST_CANNOT_HWPOINTER                  BIT(3)
/** The guest can read VMMDev events to find out about pointer movement */
#define VMMDEV_MOUSE_NEW_PROTOCOL                           BIT(4)
/**
 * If the guest changes the status of the VMMDEV_MOUSE_GUEST_NEEDS_HOST_CURSOR
 * bit, the host will honour this.
 */
#define VMMDEV_MOUSE_HOST_RECHECKS_NEEDS_HOST_CURSOR        BIT(5)
/**
 * The host supplies an absolute pointing device.  The Guest Additions may
 * wish to use this to decide whether to install their own driver.
 */
#define VMMDEV_MOUSE_HOST_HAS_ABS_DEV                       BIT(6)
/** The mask of all VMMDEV_MOUSE_* flags */
#define VMMDEV_MOUSE_MASK                                   0x0000007fU
/**
 * The mask of guest capability changes for which notification events should
 * be sent.
 */
#define VMMDEV_MOUSE_NOTIFY_HOST_MASK \
	(VMMDEV_MOUSE_GUEST_CAN_ABSOLUTE | VMMDEV_MOUSE_GUEST_NEEDS_HOST_CURSOR)
/** The mask of all capabilities which the guest can legitimately change */
#define VMMDEV_MOUSE_GUEST_MASK \
	(VMMDEV_MOUSE_NOTIFY_HOST_MASK | VMMDEV_MOUSE_NEW_PROTOCOL)
/**
 * The mask of host capability changes for which notification events should
 * be sent.
 */
#define VMMDEV_MOUSE_NOTIFY_GUEST_MASK \
	VMMDEV_MOUSE_HOST_WANTS_ABSOLUTE
/** The mask of all capabilities which the host can legitimately change */
#define VMMDEV_MOUSE_HOST_MASK \
	(VMMDEV_MOUSE_NOTIFY_GUEST_MASK |\
	 VMMDEV_MOUSE_HOST_CANNOT_HWPOINTER |\
	 VMMDEV_MOUSE_HOST_RECHECKS_NEEDS_HOST_CURSOR| \
	 VMMDEV_MOUSE_HOST_HAS_ABS_DEV)
/** @} */

/**
 * @name Absolute mouse reporting range
 * @{
 */
/** @todo Should these be here?  They are needed by both host and guest. */
/** The minimum value our pointing device can return. */
#define VMMDEV_MOUSE_RANGE_MIN 0
/** The maximum value our pointing device can return. */
#define VMMDEV_MOUSE_RANGE_MAX 0xFFFF
/** The full range our pointing device can return. */
#define VMMDEV_MOUSE_RANGE (VMMDEV_MOUSE_RANGE_MAX - VMMDEV_MOUSE_RANGE_MIN)
/** @} */

/**
 * Mouse pointer shape/visibility change request.
 *
 * Used by VMMDevReq_SetPointerShape. The size is variable.
 */
typedef struct VMMDevReqMousePointer {
	/** Header. */
	VMMDevRequestHeader header;
	/** VBOX_MOUSE_POINTER_* bit flags from VBox/Graphics/VBoxVideo.h. */
	__u32 fFlags;
	/** x coordinate of hot spot. */
	__u32 xHot;
	/** y coordinate of hot spot. */
	__u32 yHot;
	/** Width of the pointer in pixels. */
	__u32 width;
	/** Height of the pointer in scanlines. */
	__u32 height;
	/**
	 * Pointer data.
	 *
	 ****
	 * The data consists of 1 bpp AND mask followed by 32 bpp XOR (color)
	 * mask.
	 *
	 * For pointers without alpha channel the XOR mask pixels are 32 bit
	 * values: (lsb)BGR0(msb).
	 * For pointers with alpha channel the XOR mask consists of
	 * (lsb)BGRA(msb) 32 bit values.
	 *
	 * Guest driver must create the AND mask for pointers with alpha chan,
	 * so if host does not support alpha, the pointer could be displayed as
	 * a normal color pointer. The AND mask can be constructed from alpha
	 * values. For example alpha value >= 0xf0 means bit 0 in the AND mask.
	 *
	 * The AND mask is 1 bpp bitmap with byte aligned scanlines. Size of AND
	 * mask, therefore, is cbAnd = (width + 7) / 8 * height. The padding
	 * bits at the end of any scanline are undefined.
	 *
	 * The XOR mask follows the AND mask on the next 4 bytes aligned offset:
	 * u8 *pXor = pAnd + (cbAnd + 3) & ~3
	 * Bytes in the gap between the AND and the XOR mask are undefined.
	 * XOR mask scanlines have no gap between them and size of XOR mask is:
	 * cXor = width * 4 * height.
	 ****
	 *
	 * Preallocate 4 bytes for accessing actual data as p->pointerData.
	 */
	char pointerData[4];
} VMMDevReqMousePointer;
VMMDEV_ASSERT_SIZE(VMMDevReqMousePointer, 24+24);

/**
 * String log request structure.
 *
 * Used by VMMDevReq_LogString.
 * @deprecated  Use the IPRT logger or VbglR3WriteLog instead.
 */
typedef struct {
	/** header */
	VMMDevRequestHeader header;
	/** variable length string data */
	char szString[1];
} VMMDevReqLogString;
VMMDEV_ASSERT_SIZE(VMMDevReqLogString, 24+4);

/**
 * VirtualBox host version request structure.
 *
 * Used by VMMDevReq_GetHostVersion.
 *
 * @remarks VBGL uses this to detect the precense of new features in the
 *          interface.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Major version. */
	__u16 major;
	/** Minor version. */
	__u16 minor;
	/** Build number. */
	__u32 build;
	/** SVN revision. */
	__u32 revision;
	/** Feature mask. */
	__u32 features;
} VMMDevReqHostVersion;
VMMDEV_ASSERT_SIZE(VMMDevReqHostVersion, 24+16);

/**
 * @name VMMDevReqHostVersion::features
 * @{
 */
/** Physical page lists are supported by HGCM. */
#define VMMDEV_HVF_HGCM_PHYS_PAGE_LIST  BIT(0)
/** @} */

/**
 * Guest capabilities structure.
 *
 * Used by VMMDevReq_ReportGuestCapabilities.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Capabilities (VMMDEV_GUEST_*). */
	__u32 caps;
} VMMDevReqGuestCapabilities;
VMMDEV_ASSERT_SIZE(VMMDevReqGuestCapabilities, 24+4);

/**
 * Guest capabilities structure, version 2.
 *
 * Used by VMMDevReq_SetGuestCapabilities.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Mask of capabilities to be added. */
	__u32 u32OrMask;
	/** Mask of capabilities to be removed. */
	__u32 u32NotMask;
} VMMDevReqGuestCapabilities2;
VMMDEV_ASSERT_SIZE(VMMDevReqGuestCapabilities2, 24+8);

/**
 * @name Guest capability bits.
 * Used by VMMDevReq_ReportGuestCapabilities and VMMDevReq_SetGuestCapabilities.
 * @{
 */
/** The guest supports seamless display rendering. */
#define VMMDEV_GUEST_SUPPORTS_SEAMLESS                      BIT(0)
/** The guest supports mapping guest to host windows. */
#define VMMDEV_GUEST_SUPPORTS_GUEST_HOST_WINDOW_MAPPING     BIT(1)
/**
 * The guest graphical additions are active.
 * Used for fast activation and deactivation of certain graphical operations
 * (e.g. resizing & seamless). The legacy VMMDevReq_ReportGuestCapabilities
 * request sets this automatically, but VMMDevReq_SetGuestCapabilities does
 * not.
 */
#define VMMDEV_GUEST_SUPPORTS_GRAPHICS                      BIT(2)
/** The mask of valid events, for sanity checking. */
#define VMMDEV_GUEST_CAPABILITIES_MASK                      0x00000007U
/** @} */

/**
 * Idle request structure.
 *
 * Used by VMMDevReq_Idle.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
} VMMDevReqIdle;
VMMDEV_ASSERT_SIZE(VMMDevReqIdle, 24);

/**
 * Host time request structure.
 *
 * Used by VMMDevReq_GetHostTime.
 */
typedef struct {
	/** Header */
	VMMDevRequestHeader header;
	/** OUT: Time in milliseconds since unix epoch. */
	__u64 time;
} VMMDevReqHostTime;
VMMDEV_ASSERT_SIZE(VMMDevReqHostTime, 24+8);

/**
 * Hypervisor info structure.
 *
 * Used by VMMDevReq_GetHypervisorInfo and VMMDevReq_SetHypervisorInfo.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/**
	 * Guest virtual address of proposed hypervisor start.
	 * Not used by VMMDevReq_GetHypervisorInfo.
	 * @todo Make this 64-bit compatible?
	 */
	__u32 hypervisorStart;
	/** Hypervisor size in bytes. */
	__u32 hypervisorSize;
} VMMDevReqHypervisorInfo;
VMMDEV_ASSERT_SIZE(VMMDevReqHypervisorInfo, 24+8);

/**
 * @name Default patch memory size .
 * Used by VMMDevReq_RegisterPatchMemory and VMMDevReq_DeregisterPatchMemory.
 * @{
 */
#define VMMDEV_GUEST_DEFAULT_PATCHMEM_SIZE          8192
/** @} */

/**
 * Patching memory structure. (locked executable & read-only page from the
 * guest's perspective)
 *
 * Used by VMMDevReq_RegisterPatchMemory and VMMDevReq_DeregisterPatchMemory
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Guest virtual address of the patching page(s). */
	__u64 pPatchMem;
	/** Patch page size in bytes. */
	__u32 cbPatchMem;
} VMMDevReqPatchMemory;
VMMDEV_ASSERT_SIZE(VMMDevReqPatchMemory, 24+12);

/**
 * Guest power requests.
 *
 * See VMMDevReq_SetPowerStatus and VMMDevPowerStateRequest.
 */
typedef enum {
	VMMDevPowerState_Invalid   = 0,
	VMMDevPowerState_Pause     = 1,
	VMMDevPowerState_PowerOff  = 2,
	VMMDevPowerState_SaveState = 3,
	VMMDevPowerState_SizeHack = 0x7fffffff
} VMMDevPowerState;
VMMDEV_ASSERT_SIZE(VMMDevPowerState, 4);

/**
 * VM power status structure.
 *
 * Used by VMMDevReq_SetPowerStatus.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Power state request. */
	VMMDevPowerState powerState;
} VMMDevPowerStateRequest;
VMMDEV_ASSERT_SIZE(VMMDevPowerStateRequest, 24+4);

/**
 * Pending events structure.
 *
 * Used by VMMDevReq_AcknowledgeEvents.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** OUT: Pending event mask. */
	__u32 events;
} VMMDevEvents;
VMMDEV_ASSERT_SIZE(VMMDevEvents, 24+4);

/**
 * Guest event filter mask control.
 *
 * Used by VMMDevReq_CtlGuestFilterMask.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Mask of events to be added to the filter. */
	__u32 u32OrMask;
	/** Mask of events to be removed from the filter. */
	__u32 u32NotMask;
} VMMDevCtlGuestFilterMask;
VMMDEV_ASSERT_SIZE(VMMDevCtlGuestFilterMask, 24+8);

/**
 * Guest information structure.
 *
 * Used by VMMDevReportGuestInfo and PDMIVMMDEVCONNECTOR::pfnUpdateGuestVersion.
 */
typedef struct VBoxGuestInfo {
	/**
	 * The VMMDev interface version expected by additions.
	 * *Deprecated*, do not use anymore! Will be removed.
	 */
	__u32 interfaceVersion;
	/** Guest OS type. */
	VBOXOSTYPE osType;
} VBoxGuestInfo;
VMMDEV_ASSERT_SIZE(VBoxGuestInfo, 8);

/**
 * Guest information report.
 *
 * Used by VMMDevReq_ReportGuestInfo.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Guest information. */
	VBoxGuestInfo guestInfo;
} VMMDevReportGuestInfo;
VMMDEV_ASSERT_SIZE(VMMDevReportGuestInfo, 24+8);

/**
 * Guest information structure, version 2.
 *
 * Used by VMMDevReportGuestInfo2.
 */
typedef struct VBoxGuestInfo2 {
	/** Major version. */
	__u16 additionsMajor;
	/** Minor version. */
	__u16 additionsMinor;
	/** Build number. */
	__u32 additionsBuild;
	/** SVN revision. */
	__u32 additionsRevision;
	/** Feature mask, currently unused. */
	__u32 additionsFeatures;
	/**
	 * The intentional meaning of this field was:
	 * Some additional information, for example 'Beta 1' or something like
	 * that.
	 *
	 * The way it was implemented was implemented: VBOX_VERSION_STRING.
	 *
	 * This means the first three members are duplicated in this field (if
	 * the guest build config is sane). So, the user must check this and
	 * chop it off before usage. There is, because of the Main code's blind
	 * trust in the field's content, no way back.
	 */
	char szName[128];
} VBoxGuestInfo2;
VMMDEV_ASSERT_SIZE(VBoxGuestInfo2, 144);

/**
 * Guest information report, version 2.
 *
 * Used by VMMDevReq_ReportGuestInfo2.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Guest information. */
	VBoxGuestInfo2 guestInfo;
} VMMDevReportGuestInfo2;
VMMDEV_ASSERT_SIZE(VMMDevReportGuestInfo2, 24+144);

/**
 * The guest facility.
 * This needs to be kept in sync with AdditionsFacilityType of the Main API!
 */
typedef enum {
	VBoxGuestFacilityType_Unknown         = 0,
	VBoxGuestFacilityType_VBoxGuestDriver = 20,
	/* VBoxGINA / VBoxCredProv / pam_vbox. */
	VBoxGuestFacilityType_AutoLogon       = 90,
	VBoxGuestFacilityType_VBoxService     = 100,
	/* VBoxTray (Windows), VBoxClient (Linux, Unix). */
	VBoxGuestFacilityType_VBoxTrayClient  = 101,
	VBoxGuestFacilityType_Seamless        = 1000,
	VBoxGuestFacilityType_Graphics        = 1100,
	VBoxGuestFacilityType_All             = 0x7ffffffe,
	VBoxGuestFacilityType_SizeHack        = 0x7fffffff
} VBoxGuestFacilityType;
VMMDEV_ASSERT_SIZE(VBoxGuestFacilityType, 4);

/**
 * The current guest status of a facility.
 * This needs to be kept in sync with AdditionsFacilityStatus of the Main API!
 *
 * @remarks r=bird: Pretty please, for future types like this, simply do a
 *          linear allocation without any gaps. This stuff is impossible to work
 *          efficiently with, let alone validate.  Applies to the other facility
 *          enums too.
 */
typedef enum {
	VBoxGuestFacilityStatus_Inactive    = 0,
	VBoxGuestFacilityStatus_Paused      = 1,
	VBoxGuestFacilityStatus_PreInit     = 20,
	VBoxGuestFacilityStatus_Init        = 30,
	VBoxGuestFacilityStatus_Active      = 50,
	VBoxGuestFacilityStatus_Terminating = 100,
	VBoxGuestFacilityStatus_Terminated  = 101,
	VBoxGuestFacilityStatus_Failed      = 800,
	VBoxGuestFacilityStatus_Unknown     = 999,
	VBoxGuestFacilityStatus_SizeHack    = 0x7fffffff
} VBoxGuestFacilityStatus;
VMMDEV_ASSERT_SIZE(VBoxGuestFacilityStatus, 4);

/**
 * The facility class.
 * This needs to be kept in sync with AdditionsFacilityClass of the Main API!
 */
typedef enum {
	VBoxGuestFacilityClass_None       = 0,
	VBoxGuestFacilityClass_Driver     = 10,
	VBoxGuestFacilityClass_Service    = 30,
	VBoxGuestFacilityClass_Program    = 50,
	VBoxGuestFacilityClass_Feature    = 100,
	VBoxGuestFacilityClass_ThirdParty = 999,
	VBoxGuestFacilityClass_All        = 0x7ffffffe,
	VBoxGuestFacilityClass_SizeHack   = 0x7fffffff
} VBoxGuestFacilityClass;
VMMDEV_ASSERT_SIZE(VBoxGuestFacilityClass, 4);

/**
 * Guest status structure.
 *
 * Used by VMMDevReqGuestStatus.
 */
typedef struct VBoxGuestStatus {
	/** Facility the status is indicated for. */
	VBoxGuestFacilityType facility;
	/** Current guest status. */
	VBoxGuestFacilityStatus status;
	/** Flags, not used at the moment. */
	__u32 flags;
} VBoxGuestStatus;
VMMDEV_ASSERT_SIZE(VBoxGuestStatus, 12);

/**
 * Guest Additions status structure.
 *
 * Used by VMMDevReq_ReportGuestStatus.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Guest information. */
	VBoxGuestStatus guestStatus;
} VMMDevReportGuestStatus;
VMMDEV_ASSERT_SIZE(VMMDevReportGuestStatus, 24+12);

/**
 * The current status of specific guest user.
 * This needs to be kept in sync with GuestUserState of the Main API!
 */
typedef enum VBoxGuestUserState {
	VBoxGuestUserState_Unknown            = 0,
	VBoxGuestUserState_LoggedIn           = 1,
	VBoxGuestUserState_LoggedOut          = 2,
	VBoxGuestUserState_Locked             = 3,
	VBoxGuestUserState_Unlocked           = 4,
	VBoxGuestUserState_Disabled           = 5,
	VBoxGuestUserState_Idle               = 6,
	VBoxGuestUserState_InUse              = 7,
	VBoxGuestUserState_Created            = 8,
	VBoxGuestUserState_Deleted            = 9,
	VBoxGuestUserState_SessionChanged     = 10,
	VBoxGuestUserState_CredentialsChanged = 11,
	VBoxGuestUserState_RoleChanged        = 12,
	VBoxGuestUserState_GroupAdded         = 13,
	VBoxGuestUserState_GroupRemoved       = 14,
	VBoxGuestUserState_Elevated           = 15,
	VBoxGuestUserState_SizeHack           = 0x7fffffff
} VBoxGuestUserState;
VMMDEV_ASSERT_SIZE(VBoxGuestUserState, 4);

/**
 * Guest user status updates.
 */
typedef struct VBoxGuestUserStatus {
	/** The guest user state to send. */
	VBoxGuestUserState state;
	/** Size (in bytes) of szUser. */
	__u32 cbUser;
	/** Size (in bytes) of szDomain. */
	__u32 cbDomain;
	/** Size (in bytes) of aDetails. */
	__u32 cbDetails;
	/** Note: Here begins the dynamically allocated region. */
	/** Guest user to report state for. */
	char szUser[1];
	/** Domain the guest user is bound to. */
	char szDomain[1];
	/** Optional details of the state. */
	__u8 aDetails[1];
} VBoxGuestUserStatus;
VMMDEV_ASSERT_SIZE(VBoxGuestUserStatus, 20);

/**
 * Guest user status structure.
 *
 * Used by VMMDevReq_ReportGuestUserStatus.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Guest user status. */
	VBoxGuestUserStatus status;
} VMMDevReportGuestUserState;
VMMDEV_ASSERT_SIZE(VMMDevReportGuestUserState, 24+20);

/**
 * Guest statistics structure.
 *
 * Used by VMMDevReportGuestStats and PDMIVMMDEVCONNECTOR::pfnReportStatistics.
 */
typedef struct VBoxGuestStatistics {
	/** Virtual CPU ID. */
	__u32 u32CpuId;
	/** Reported statistics. */
	__u32 u32StatCaps;
	/** Idle CPU load (0-100) for last interval. */
	__u32 u32CpuLoad_Idle;
	/** Kernel CPU load (0-100) for last interval. */
	__u32 u32CpuLoad_Kernel;
	/** User CPU load (0-100) for last interval. */
	__u32 u32CpuLoad_User;
	/** Nr of threads. */
	__u32 u32Threads;
	/** Nr of processes. */
	__u32 u32Processes;
	/** Nr of handles. */
	__u32 u32Handles;
	/** Memory load (0-100). */
	__u32 u32MemoryLoad;
	/** Page size of guest system. */
	__u32 u32PageSize;
	/** Total physical memory (in 4KB pages). */
	__u32 u32PhysMemTotal;
	/** Available physical memory (in 4KB pages). */
	__u32 u32PhysMemAvail;
	/** Ballooned physical memory (in 4KB pages). */
	__u32 u32PhysMemBalloon;
	/** Total committed memory (not necessarily in-use) (in 4KB pages). */
	__u32 u32MemCommitTotal;
	/** Total amount of memory used by the kernel (in 4KB pages). */
	__u32 u32MemKernelTotal;
	/** Total amount of paged memory used by the kernel (in 4KB pages). */
	__u32 u32MemKernelPaged;
	/** Total amount of nonpaged memory used by the kernel (4KB pages). */
	__u32 u32MemKernelNonPaged;
	/** Total amount of memory used for the system cache (in 4KB pages). */
	__u32 u32MemSystemCache;
	/** Pagefile size (in 4KB pages). */
	__u32 u32PageFileSize;
} VBoxGuestStatistics;
VMMDEV_ASSERT_SIZE(VBoxGuestStatistics, 19*4);

/**
 * @name Guest statistics values (VBoxGuestStatistics::u32StatCaps).
 * @{
 */
#define VBOX_GUEST_STAT_CPU_LOAD_IDLE       BIT(0)
#define VBOX_GUEST_STAT_CPU_LOAD_KERNEL     BIT(1)
#define VBOX_GUEST_STAT_CPU_LOAD_USER       BIT(2)
#define VBOX_GUEST_STAT_THREADS             BIT(3)
#define VBOX_GUEST_STAT_PROCESSES           BIT(4)
#define VBOX_GUEST_STAT_HANDLES             BIT(5)
#define VBOX_GUEST_STAT_MEMORY_LOAD         BIT(6)
#define VBOX_GUEST_STAT_PHYS_MEM_TOTAL      BIT(7)
#define VBOX_GUEST_STAT_PHYS_MEM_AVAIL      BIT(8)
#define VBOX_GUEST_STAT_PHYS_MEM_BALLOON    BIT(9)
#define VBOX_GUEST_STAT_MEM_COMMIT_TOTAL    BIT(10)
#define VBOX_GUEST_STAT_MEM_KERNEL_TOTAL    BIT(11)
#define VBOX_GUEST_STAT_MEM_KERNEL_PAGED    BIT(12)
#define VBOX_GUEST_STAT_MEM_KERNEL_NONPAGED BIT(13)
#define VBOX_GUEST_STAT_MEM_SYSTEM_CACHE    BIT(14)
#define VBOX_GUEST_STAT_PAGE_FILE_SIZE      BIT(15)
/** @} */

/**
 * Guest statistics command structure.
 *
 * Used by VMMDevReq_ReportGuestStats.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Guest information. */
	VBoxGuestStatistics guestStats;
} VMMDevReportGuestStats;
VMMDEV_ASSERT_SIZE(VMMDevReportGuestStats, 24+19*4);

/**
 * @name The ballooning chunk size which VMMDev works at.
 * @{
 */
#define VMMDEV_MEMORY_BALLOON_CHUNK_SIZE             (1048576)
#define VMMDEV_MEMORY_BALLOON_CHUNK_PAGES            (1048576 / 4096)
/** @} */

/**
 * Poll for ballooning change request.
 *
 * Used by VMMDevReq_GetMemBalloonChangeRequest.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Balloon size in megabytes. */
	__u32 cBalloonChunks;
	/** Guest ram size in megabytes. */
	__u32 cPhysMemChunks;
	/**
	 * Setting this to VMMDEV_EVENT_BALLOON_CHANGE_REQUEST indicates that
	 * the request is a response to that event.
	 * (Don't confuse this with VMMDevReq_AcknowledgeEvents.)
	 */
	__u32 eventAck;
} VMMDevGetMemBalloonChangeRequest;
VMMDEV_ASSERT_SIZE(VMMDevGetMemBalloonChangeRequest, 24+12);

/**
 * Change the size of the balloon.
 *
 * Used by VMMDevReq_ChangeMemBalloon.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** The number of pages in the array. */
	__u32 pages;
	/** true = inflate, false = deflate.  */
	__u32 inflate;
	/** Physical address (u64) of each page. */
	__u64 phys_page[VMMDEV_MEMORY_BALLOON_CHUNK_PAGES];
} VMMDevChangeMemBalloon;

/**
 * Guest statistics interval change request structure.
 *
 * Used by VMMDevReq_GetStatisticsChangeRequest.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** The interval in seconds. */
	__u32 u32StatInterval;
	/**
	 * Setting this to VMMDEV_EVENT_STATISTICS_INTERVAL_CHANGE_REQUEST
	 * indicates that the request is a response to that event.
	 * (Don't confuse this with VMMDevReq_AcknowledgeEvents.)
	 */
	__u32 eventAck;
} VMMDevGetStatisticsChangeRequest;
VMMDEV_ASSERT_SIZE(VMMDevGetStatisticsChangeRequest, 24+8);

/**
 * The size of a string field in the credentials request (including '\\0').
 * @see VMMDevCredentials
 */
#define VMMDEV_CREDENTIALS_SZ_SIZE          128

/**
 * Credentials request structure.
 *
 * Used by VMMDevReq_QueryCredentials.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** IN/OUT: Request flags. */
	__u32 u32Flags;
	/** OUT: User name (UTF-8). */
	char szUserName[VMMDEV_CREDENTIALS_SZ_SIZE];
	/** OUT: Password (UTF-8). */
	char szPassword[VMMDEV_CREDENTIALS_SZ_SIZE];
	/** OUT: Domain name (UTF-8). */
	char szDomain[VMMDEV_CREDENTIALS_SZ_SIZE];
} VMMDevCredentials;
VMMDEV_ASSERT_SIZE(VMMDevCredentials, 24+4+3*128);

/**
 * @name Credentials request flag (VMMDevCredentials::u32Flags)
 * @{
 */
/** query from host whether credentials are present */
#define VMMDEV_CREDENTIALS_QUERYPRESENCE     BIT(1)
/** read credentials from host (can be combined with clear) */
#define VMMDEV_CREDENTIALS_READ              BIT(2)
/** clear credentials on host (can be combined with read) */
#define VMMDEV_CREDENTIALS_CLEAR             BIT(3)
/** read credentials for judgement in the guest */
#define VMMDEV_CREDENTIALS_READJUDGE         BIT(8)
/** clear credentials for judegement on the host */
#define VMMDEV_CREDENTIALS_CLEARJUDGE        BIT(9)
/** report credentials acceptance by guest */
#define VMMDEV_CREDENTIALS_JUDGE_OK          BIT(10)
/** report credentials denial by guest */
#define VMMDEV_CREDENTIALS_JUDGE_DENY        BIT(11)
/** report that no judgement could be made by guest */
#define VMMDEV_CREDENTIALS_JUDGE_NOJUDGEMENT BIT(12)

/** flag telling the guest that credentials are present */
#define VMMDEV_CREDENTIALS_PRESENT           BIT(16)
/** flag telling guest that local logons should be prohibited */
#define VMMDEV_CREDENTIALS_NOLOCALLOGON      BIT(17)
/** @} */

/**
 * Seamless mode.
 *
 * Used by VbglR3SeamlessWaitEvent
 *
 * @ingroup grp_vmmdev_req
 *
 * @todo DARN! DARN! DARN! Who forgot to do the 32-bit hack here???
 *       FIXME! XXX!
 *
 *       We will now have to carefully check how our compilers have treated this
 *       flag. If any are compressing it into a byte type, we'll have to check
 *       how the request memory is initialized. If we are 104% sure it's ok to
 *       expand it, we'll expand it. If not, we must redefine the field to a
 *       u8 and a 3 byte padding.
 */
typedef enum {
	/** normal mode; entire guest desktop displayed. */
	VMMDev_Seamless_Disabled         = 0,
	/** visible region mode; only top-level guest windows displayed. */
	VMMDev_Seamless_Visible_Region   = 1,
	/**
	 * windowed mode; each top-level guest window is represented in a
	 * host window.
	 */
	VMMDev_Seamless_Host_Window      = 2
} VMMDevSeamlessMode;

/**
 * Seamless mode change request structure.
 *
 * Used by VMMDevReq_GetSeamlessChangeRequest.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;

	/** New seamless mode. */
	VMMDevSeamlessMode mode;
	/**
	 * Setting this to VMMDEV_EVENT_SEAMLESS_MODE_CHANGE_REQUEST indicates
	 * that the request is a response to that event.
	 * (Don't confuse this with VMMDevReq_AcknowledgeEvents.)
	 */
	__u32 eventAck;
} VMMDevSeamlessChangeRequest;
VMMDEV_ASSERT_SIZE(VMMDevSeamlessChangeRequest, 24+8);
VMMDEV_ASSERT_MEMBER_OFFSET(VMMDevSeamlessChangeRequest, eventAck, 24+4);

/**
 * Display change request structure.
 *
 * Used by VMMDevReq_GetDisplayChangeRequest.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Horizontal pixel resolution (0 = do not change). */
	__u32 xres;
	/** Vertical pixel resolution (0 = do not change). */
	__u32 yres;
	/** Bits per pixel (0 = do not change). */
	__u32 bpp;
	/**
	 * Setting this to VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST indicates
	 * that the request is a response to that event.
	 * (Don't confuse this with VMMDevReq_AcknowledgeEvents.)
	 */
	__u32 eventAck;
} VMMDevDisplayChangeRequest;
VMMDEV_ASSERT_SIZE(VMMDevDisplayChangeRequest, 24+16);

/**
 * Display change request structure, version 2.
 *
 * Used by VMMDevReq_GetDisplayChangeRequest2.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Horizontal pixel resolution (0 = do not change). */
	__u32 xres;
	/** Vertical pixel resolution (0 = do not change). */
	__u32 yres;
	/** Bits per pixel (0 = do not change). */
	__u32 bpp;
	/**
	 * Setting this to VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST indicates
	 * that the request is a response to that event.
	 * (Don't confuse this with VMMDevReq_AcknowledgeEvents.)
	 */
	__u32 eventAck;
	/** 0 for primary display, 1 for the first secondary, etc. */
	__u32 display;
} VMMDevDisplayChangeRequest2;
VMMDEV_ASSERT_SIZE(VMMDevDisplayChangeRequest2, 24+20);

/**
 * Display change request structure, version Extended.
 *
 * Used by VMMDevReq_GetDisplayChangeRequestEx.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Horizontal pixel resolution (0 = do not change). */
	__u32 xres;
	/** Vertical pixel resolution (0 = do not change). */
	__u32 yres;
	/** Bits per pixel (0 = do not change). */
	__u32 bpp;
	/**
	 * Setting this to VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST indicates
	 * that the request is a response to that event.
	 * (Don't confuse this with VMMDevReq_AcknowledgeEvents.)
	 */
	__u32 eventAck;
	/** 0 for primary display, 1 for the first secondary, etc. */
	__u32 display;
	/** New OriginX of secondary virtual screen */
	__u32 cxOrigin;
	/** New OriginY of secondary virtual screen  */
	__u32 cyOrigin;
	/** Change in origin of the secondary virtual screen is required */
	u8 fChangeOrigin;
	/** Secondary virtual screen enabled or disabled */
	u8 fEnabled;
	/** Alignment */
	u8 alignment[2];
} VMMDevDisplayChangeRequestEx;
VMMDEV_ASSERT_SIZE(VMMDevDisplayChangeRequestEx, 24+32);

/**
 * Video mode supported request structure.
 *
 * Used by VMMDevReq_VideoModeSupported.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** IN: Horizontal pixel resolution. */
	__u32 width;
	/** IN: Vertical pixel resolution. */
	__u32 height;
	/** IN: Bits per pixel. */
	__u32 bpp;
	/** OUT: Support indicator. */
	u8 fSupported;
	/** Alignment */
	u8 alignment[3];
} VMMDevVideoModeSupportedRequest;
VMMDEV_ASSERT_SIZE(VMMDevVideoModeSupportedRequest, 24+16);

/**
 * Video mode supported request structure for a specific display.
 *
 * Used by VMMDevReq_VideoModeSupported2.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** IN: The guest display number. */
	__u32 display;
	/** IN: Horizontal pixel resolution. */
	__u32 width;
	/** IN: Vertical pixel resolution. */
	__u32 height;
	/** IN: Bits per pixel. */
	__u32 bpp;
	/** OUT: Support indicator. */
	u8 fSupported;
	/** Alignment */
	u8 alignment[3];
} VMMDevVideoModeSupportedRequest2;
VMMDEV_ASSERT_SIZE(VMMDevVideoModeSupportedRequest2, 24+20);

/**
 * Video modes height reduction request structure.
 *
 * Used by VMMDevReq_GetHeightReduction.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** OUT: Height reduction in pixels. */
	__u32 heightReduction;
} VMMDevGetHeightReductionRequest;
VMMDEV_ASSERT_SIZE(VMMDevGetHeightReductionRequest, 24+4);

/**
 * VRDP change request structure.
 *
 * Used by VMMDevReq_GetVRDPChangeRequest.
 */
typedef struct {
	/** Header */
	VMMDevRequestHeader header;
	/** Whether VRDP is active or not. */
	__u8 u8VRDPActive;
	/** The configured experience level for active VRDP. */
	__u32 u32VRDPExperienceLevel;
} VMMDevVRDPChangeRequest;
VMMDEV_ASSERT_SIZE(VMMDevVRDPChangeRequest, 24+8);
VMMDEV_ASSERT_MEMBER_OFFSET(VMMDevVRDPChangeRequest, u8VRDPActive, 24);
VMMDEV_ASSERT_MEMBER_OFFSET(VMMDevVRDPChangeRequest, u32VRDPExperienceLevel,
			    24+4);

/**
 * @name VRDP Experience level (VMMDevVRDPChangeRequest::u32VRDPExperienceLevel)
 * @{
 */
#define VRDP_EXPERIENCE_LEVEL_ZERO     0 /**< Theming disabled. */
#define VRDP_EXPERIENCE_LEVEL_LOW      1 /**< Full win drag + wallpaper dis. */
#define VRDP_EXPERIENCE_LEVEL_MEDIUM   2 /**< Font smoothing, gradients. */
#define VRDP_EXPERIENCE_LEVEL_HIGH     3 /**< Animation effects disabled. */
#define VRDP_EXPERIENCE_LEVEL_FULL     4 /**< Everything enabled. */
/** @} */

/**
 * VBVA enable request structure.
 *
 * Used by VMMDevReq_VideoAccelEnable.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** 0 - disable, !0 - enable. */
	__u32 u32Enable;
	/**
	 * The size of VBVAMEMORY::au8RingBuffer expected by driver.
	 *  The host will refuse to enable VBVA if the size is not equal to
	 *  VBVA_RING_BUFFER_SIZE.
	 */
	__u32 cbRingBuffer;
	/**
	 * Guest initializes the status to 0. Host sets appropriate
	 * VBVA_F_STATUS_ flags.
	 */
	__u32 fu32Status;
} VMMDevVideoAccelEnable;
VMMDEV_ASSERT_SIZE(VMMDevVideoAccelEnable, 24+12);

/**
 * @name VMMDevVideoAccelEnable::fu32Status.
 * @{
 */
#define VBVA_F_STATUS_ACCEPTED (0x01)
#define VBVA_F_STATUS_ENABLED  (0x02)
/** @} */

/**
 * VBVA flush request structure.
 *
 * Used by VMMDevReq_VideoAccelFlush.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
} VMMDevVideoAccelFlush;
VMMDEV_ASSERT_SIZE(VMMDevVideoAccelFlush, 24);

/**
 * Rectangle data type, double point.
 */
typedef struct RTRECT {
	/** left X coordinate. */
	__s32 xLeft;
	/** top Y coordinate. */
	__s32 yTop;
	/** right X coordinate. (exclusive) */
	__s32 xRight;
	/** bottom Y coordinate. (exclusive) */
	__s32 yBottom;
} RTRECT;

/**
 * VBVA set visible region request structure.
 *
 * Used by VMMDevReq_VideoSetVisibleRegion.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Number of rectangles */
	__u32 cRect;
	/**
	 * Rectangle array.
	 * @todo array is spelled aRects[1].
	 */
	RTRECT Rect;
} VMMDevVideoSetVisibleRegion;
VMMDEV_ASSERT_SIZE(RTRECT, 16);
VMMDEV_ASSERT_SIZE(VMMDevVideoSetVisibleRegion, 24+4+16);

/**
 * CPU event types.
 */
typedef enum {
	VMMDevCpuStatusType_Invalid  = 0,
	VMMDevCpuStatusType_Disable  = 1,
	VMMDevCpuStatusType_Enable   = 2,
	VMMDevCpuStatusType_SizeHack = 0x7fffffff
} VMMDevCpuStatusType;

/**
 * CPU hotplug event status request.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Status type */
	VMMDevCpuStatusType enmStatusType;
} VMMDevCpuHotPlugStatusRequest;
VMMDEV_ASSERT_SIZE(VMMDevCpuHotPlugStatusRequest, 24+4);

/**
 * CPU event types.
 *
 * Used by VbglR3CpuHotplugWaitForEvent
 *
 * @ingroup grp_vmmdev_req
 */
typedef enum {
	VMMDevCpuEventType_Invalid  = 0,
	VMMDevCpuEventType_None     = 1,
	VMMDevCpuEventType_Plug     = 2,
	VMMDevCpuEventType_Unplug   = 3,
	VMMDevCpuEventType_SizeHack = 0x7fffffff
} VMMDevCpuEventType;

/**
 * Get the ID of the changed CPU and event type.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Event type */
	VMMDevCpuEventType enmEventType;
	/** core id of the CPU changed */
	__u32 idCpuCore;
	/** package id of the CPU changed */
	__u32 idCpuPackage;
} VMMDevGetCpuHotPlugRequest;
VMMDEV_ASSERT_SIZE(VMMDevGetCpuHotPlugRequest, 24+4+4+4);

/**
 * Shared region description
 */
typedef struct VMMDEVSHAREDREGIONDESC {
	__u64 GCRegionAddr;
	__u32 cbRegion;
	__u32 alignment;
} VMMDEVSHAREDREGIONDESC;
VMMDEV_ASSERT_SIZE(VMMDEVSHAREDREGIONDESC, 16);

#define VMMDEVSHAREDREGIONDESC_MAX          32

/**
 * Shared module registration
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Shared module size. */
	__u32 cbModule;
	/** Number of included region descriptors */
	__u32 cRegions;
	/** Base address of the shared module. */
	__u64 GCBaseAddr;
	/** Guest OS type. */
	VBOXOSFAMILY enmGuestOS;
	/** Alignment. */
	__u32 alignment;
	/** Module name */
	char szName[128];
	/** Module version */
	char szVersion[16];
	/** Shared region descriptor(s). */
	VMMDEVSHAREDREGIONDESC aRegions[1];
} VMMDevSharedModuleRegistrationRequest;
VMMDEV_ASSERT_SIZE(VMMDevSharedModuleRegistrationRequest,
		   24+4+4+8+4+4+128+16+16);

/**
 * Shared module unregistration
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Shared module size. */
	__u32 cbModule;
	/** Align at 8 byte boundary. */
	__u32 alignment;
	/** Base address of the shared module. */
	__u64 GCBaseAddr;
	/** Module name */
	char szName[128];
	/** Module version */
	char szVersion[16];
} VMMDevSharedModuleUnregistrationRequest;
VMMDEV_ASSERT_SIZE(VMMDevSharedModuleUnregistrationRequest, 24+4+4+8+128+16);

/**
 * Shared module periodic check
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
} VMMDevSharedModuleCheckRequest;
VMMDEV_ASSERT_SIZE(VMMDevSharedModuleCheckRequest, 24);

/**
 * Paging sharing enabled query
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Enabled flag (out) */
	u8 fEnabled;
	/** Alignment */
	u8 alignment[3];
} VMMDevPageSharingStatusRequest;
VMMDEV_ASSERT_SIZE(VMMDevPageSharingStatusRequest, 24+4);

/**
 * Page sharing status query (debug build only)
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Page address, 32 bits on 32 bit builds, 64 bit on 64 bit builds */
	unsigned long GCPtrPage;
	/** Page flags. */
	__u64 uPageFlags;
	/** Shared flag (out) */
	u8 fShared;
	/** Alignment */
	u8 alignment[3];
} VMMDevPageIsSharedRequest;

/**
 * Session id request structure.
 *
 * Used by VMMDevReq_GetSessionId.
 */
typedef struct {
	/** Header */
	VMMDevRequestHeader header;
	/**
	 * OUT: unique session id; the id will be different after each start,
	 * reset or restore of the VM.
	 */
	__u64 idSession;
} VMMDevReqSessionId;
VMMDEV_ASSERT_SIZE(VMMDevReqSessionId, 24+8);

/**
 * Write Core Dump request.
 *
 * Used by VMMDevReq_WriteCoreDump.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** Flags (reserved, MBZ). */
	__u32 fFlags;
} VMMDevReqWriteCoreDump;
VMMDEV_ASSERT_SIZE(VMMDevReqWriteCoreDump, 24+4);

/** Heart beat check state structure. Used by VMMDevReq_HeartbeatConfigure. */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** OUT: Guest heartbeat interval in nanosec. */
	__u64 cNsInterval;
	/** Heartbeat check flag. */
	u8 fEnabled;
	/** Alignment */
	u8 alignment[3];
} VMMDevReqHeartbeat;
VMMDEV_ASSERT_SIZE(VMMDevReqHeartbeat, 24+12);

/**
 * @name HGCM flags.
 * @{
 */
#define VBOX_HGCM_REQ_DONE      BIT(VBOX_HGCM_REQ_DONE_BIT)
#define VBOX_HGCM_REQ_DONE_BIT  0
#define VBOX_HGCM_REQ_CANCELLED (0x2)
/** @} */

/**
 * HGCM request header.
 */
typedef struct VMMDevHGCMRequestHeader {
	/** Request header. */
	VMMDevRequestHeader header;

	/** HGCM flags. */
	__u32 fu32Flags;

	/** Result code. */
	__s32 result;
} VMMDevHGCMRequestHeader;
VMMDEV_ASSERT_SIZE(VMMDevHGCMRequestHeader, 24+8);

/**
 * HGCM service location types.
 * @ingroup grp_vmmdev_req
 */
typedef enum {
	VMMDevHGCMLoc_Invalid    = 0,
	VMMDevHGCMLoc_LocalHost  = 1,
	VMMDevHGCMLoc_LocalHost_Existing = 2,
	VMMDevHGCMLoc_SizeHack   = 0x7fffffff
} HGCMServiceLocationType;
VMMDEV_ASSERT_SIZE(HGCMServiceLocationType, 4);

/**
 * HGCM host service location.
 * @ingroup grp_vmmdev_req
 */
typedef struct {
	char achName[128]; /**< This is really szName. */
} HGCMServiceLocationHost;
VMMDEV_ASSERT_SIZE(HGCMServiceLocationHost, 128);

/**
 * HGCM service location.
 * @ingroup grp_vmmdev_req
 */
typedef struct HGCMSERVICELOCATION {
	/** Type of the location. */
	HGCMServiceLocationType type;

	union {
		HGCMServiceLocationHost host;
	} u;
} HGCMServiceLocation;
VMMDEV_ASSERT_SIZE(HGCMServiceLocation, 128+4);

/**
 * HGCM connect request structure.
 *
 * Used by VMMDevReq_HGCMConnect.
 */
typedef struct {
	/** HGCM request header. */
	VMMDevHGCMRequestHeader header;

	/** IN: Description of service to connect to. */
	HGCMServiceLocation loc;

	/** OUT: Client identifier assigned by local instance of HGCM. */
	__u32 u32ClientID;
} VMMDevHGCMConnect;
VMMDEV_ASSERT_SIZE(VMMDevHGCMConnect, 32+132+4);

/**
 * HGCM disconnect request structure.
 *
 * Used by VMMDevReq_HGCMDisconnect.
 */
typedef struct {
	/** HGCM request header. */
	VMMDevHGCMRequestHeader header;

	/** IN: Client identifier. */
	__u32 u32ClientID;
} VMMDevHGCMDisconnect;
VMMDEV_ASSERT_SIZE(VMMDevHGCMDisconnect, 32+4);

/**
 * HGCM parameter type.
 */
typedef enum {
	VMMDevHGCMParmType_Invalid            = 0,
	VMMDevHGCMParmType_32bit              = 1,
	VMMDevHGCMParmType_64bit              = 2,
	/** @deprecated Doesn't work, use PageList. */
	VMMDevHGCMParmType_PhysAddr           = 3,
	/** In and Out */
	VMMDevHGCMParmType_LinAddr            = 4,
	/** In  (read;  host<-guest) */
	VMMDevHGCMParmType_LinAddr_In         = 5,
	/** Out (write; host->guest) */
	VMMDevHGCMParmType_LinAddr_Out        = 6,
	/* 7 - 9 VMMDevHGCMParmType_LinAddr_Locked*, non Linux R0 usage only */
	/** Physical addresses of locked pages for a buffer. */
	VMMDevHGCMParmType_PageList           = 10,
	VMMDevHGCMParmType_SizeHack           = 0x7fffffff
} HGCMFunctionParameterType;
VMMDEV_ASSERT_SIZE(HGCMFunctionParameterType, 4);

/**
 * HGCM function parameter, 32-bit client.
 */
typedef struct HGCMFunctionParameter32 {
	HGCMFunctionParameterType type;
	union {
		__u32 value32;
		__u64 value64;
		struct {
			__u32 size;
			union {
				__u32 physAddr;
				__u32 linearAddr;
			} u;
		} Pointer;
		struct {
			/** Size of the buffer described by the page list. */
			__u32 size;
			/** Relative to the request header. */
			__u32 offset;
		} PageList;
	} u;
} HGCMFunctionParameter32;
VMMDEV_ASSERT_SIZE(HGCMFunctionParameter32, 4+8);

/**
 * HGCM function parameter, 64-bit client.
 */
typedef struct HGCMFunctionParameter64 {
	HGCMFunctionParameterType type;
	union {
		__u32 value32;
		__u64 value64;
		struct {
			__u32 size;
			union {
				__u64 physAddr;
				__u64 linearAddr;
			} u;
		} Pointer;
		struct {
			/** Size of the buffer described by the page list. */
			__u32 size;
			/** Relative to the request header. */
			__u32 offset;
		} PageList;
	} u;
} HGCMFunctionParameter64;
VMMDEV_ASSERT_SIZE(HGCMFunctionParameter64, 4+12);

#if __BITS_PER_LONG == 64
#define HGCMFunctionParameter HGCMFunctionParameter64
#else
#define HGCMFunctionParameter HGCMFunctionParameter32
#endif

/**
 * HGCM call request structure.
 *
 * Used by VMMDevReq_HGCMCall32 and VMMDevReq_HGCMCall64.
 */
typedef struct {
	/* request header */
	VMMDevHGCMRequestHeader header;

	/** IN: Client identifier. */
	__u32 u32ClientID;
	/** IN: Service function number. */
	__u32 u32Function;
	/** IN: Number of parameters. */
	__u32 cParms;
	/** Parameters follow in form: HGCMFunctionParameter32|64 aParms[X]; */
} VMMDevHGCMCall;
VMMDEV_ASSERT_SIZE(VMMDevHGCMCall, 32+12);

/**
 * @name Direction of data transfer (HGCMPageListInfo::flags). Bit flags.
 * @{
 */
#define VBOX_HGCM_F_PARM_DIRECTION_NONE      0x00000000U
#define VBOX_HGCM_F_PARM_DIRECTION_TO_HOST   0x00000001U
#define VBOX_HGCM_F_PARM_DIRECTION_FROM_HOST 0x00000002U
#define VBOX_HGCM_F_PARM_DIRECTION_BOTH      0x00000003U
/**
 * Macro for validating that the specified flags are valid.
 * Note BOTH is not valid.
 */
#define VBOX_HGCM_F_PARM_ARE_VALID(fFlags) \
	((fFlags) > VBOX_HGCM_F_PARM_DIRECTION_NONE && \
	 (fFlags) < VBOX_HGCM_F_PARM_DIRECTION_BOTH)
/** @} */

/**
 * VMMDevHGCMParmType_PageList points to this structure to actually describe
 * the buffer.
 */
typedef struct {
	__u32 flags;        /**< VBOX_HGCM_F_PARM_*. */
	__u16 offFirstPage; /**< Offset in the first page where data begins. */
	__u16 cPages;       /**< Number of pages. */
	__u64 aPages[1];    /**< Page addresses. */
} HGCMPageListInfo;
VMMDEV_ASSERT_SIZE(HGCMPageListInfo, 4+2+2+8);

/** Get the pointer to the first parmater of a HGCM call request.  */
#define VMMDEV_HGCM_CALL_PARMS(a) \
	((HGCMFunctionParameter *)((__u8 *)(a) + sizeof(VMMDevHGCMCall)))

#define VBOX_HGCM_MAX_PARMS 32

/**
 * HGCM cancel request structure.
 *
 * The Cancel request is issued using the same physical memory address as was
 * used for the corresponding initial HGCMCall.
 *
 * Used by VMMDevReq_HGCMCancel.
 */
typedef struct {
	/** Header. */
	VMMDevHGCMRequestHeader header;
} VMMDevHGCMCancel;
VMMDEV_ASSERT_SIZE(VMMDevHGCMCancel, 32);

/**
 * HGCM cancel request structure, version 2.
 *
 * Used by VMMDevReq_HGCMCancel2.
 *
 * VINF_SUCCESS when cancelled.
 * VERR_NOT_FOUND if the specified request cannot be found.
 * VERR_INVALID_PARAMETER if the address is invalid valid.
 */
typedef struct {
	/** Header. */
	VMMDevRequestHeader header;
	/** The physical address of the request to cancel. */
	__u32 physReqToCancel;
} VMMDevHGCMCancel2;
VMMDEV_ASSERT_SIZE(VMMDevHGCMCancel2, 24+4);

/** @} */

#pragma pack()

#endif
