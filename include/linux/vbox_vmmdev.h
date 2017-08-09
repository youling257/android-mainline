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

#ifndef __VBOX_VMMDEV_H__
#define __VBOX_VMMDEV_H__

#include <linux/sizes.h>
#include <uapi/linux/vbox_vmmdev.h>

/**
 * @name VBVA ring defines.
 *
 * The VBVA ring buffer is suitable for transferring large (< 2GB) amount of
 * data. For example big bitmaps which do not fit to the buffer.
 *
 * Guest starts writing to the buffer by initializing a record entry in the
 * aRecords queue. VBVA_F_RECORD_PARTIAL indicates that the record is being
 * written. As data is written to the ring buffer, the guest increases off32End
 * for the record.
 *
 * The host reads the aRecords on flushes and processes all completed records.
 * When host encounters situation when only a partial record presents and
 * cbRecord & ~VBVA_F_RECORD_PARTIAL >= VBVA_RING_BUFFER_SIZE -
 * VBVA_RING_BUFFER_THRESHOLD, the host fetched all record data and updates
 * off32Head. After that on each flush the host continues fetching the data
 * until the record is completed.
 *
 * @{
 */
#define VMMDEV_VBVA_RING_BUFFER_SIZE        (SZ_4M - SZ_1K)
#define VMMDEV_VBVA_RING_BUFFER_THRESHOLD   (SZ_4K)

#define VMMDEV_VBVA_MAX_RECORDS (64)
/** @} */

/** VBVA record. */
typedef struct VMMDEVVBVARECORD {
	/** The length of the record. Changed by guest. */
	u32 cbRecord;
} VMMDEVVBVARECORD;
VMMDEV_ASSERT_SIZE(VMMDEVVBVARECORD, 4);

/**
 * VBVA memory layout.
 *
 * This is a subsection of the VMMDevMemory structure.
 */
typedef struct VBVAMEMORY {
	/** VBVA_F_MODE_*. */
	u32 fu32ModeFlags;

	/** The offset where the data start in the buffer. */
	u32 off32Data;
	/** The offset where next data must be placed in the buffer. */
	u32 off32Free;

	/** The ring buffer for data. */
	u8  au8RingBuffer[VMMDEV_VBVA_RING_BUFFER_SIZE];

	/** The queue of record descriptions. */
	VMMDEVVBVARECORD aRecords[VMMDEV_VBVA_MAX_RECORDS];
	u32 indexRecordFirst;
	u32 indexRecordFree;

	/**
	 * RDP orders supported by the client. The guest reports only them
	 * and falls back to DIRTY rects for not supported ones.
	 *
	 * (1 << VBVA_VRDP_*)
	 */
	u32 fu32SupportedOrders;

} VBVAMEMORY;
VMMDEV_ASSERT_SIZE(VBVAMEMORY, 12 + (SZ_4M-SZ_1K) + 4*64 + 12);

/**
 * The layout of VMMDEV RAM region that contains information for guest.
 */
typedef struct VMMDevMemory {
	/** The size of this structure. */
	u32 u32Size;
	/** The structure version. (VMMDEV_MEMORY_VERSION) */
	u32 u32Version;

	union {
		struct {
			/** Flag telling that VMMDev has events pending. */
			bool fHaveEvents;
		} V1_04;

		struct {
			/** Pending events flags, set by host. */
			u32 u32HostEvents;
			/** Mask of events the guest wants, set by guest. */
			u32 u32GuestEventMask;
		} V1_03;
	} V;

	VBVAMEMORY vbvaMemory;

} VMMDevMemory;
VMMDEV_ASSERT_SIZE(VMMDevMemory, 8 + 8 + sizeof(VBVAMEMORY));
VMMDEV_ASSERT_MEMBER_OFFSET(VMMDevMemory, vbvaMemory, 16);

/** Version of VMMDevMemory structure (VMMDevMemory::u32Version). */
#define VMMDEV_MEMORY_VERSION   (1)

#endif
