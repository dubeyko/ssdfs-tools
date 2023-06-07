//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * lib/blkbmap.c - block bitmap functionality.
 *
 * Copyright (c) 2014-2019 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 * Copyright (c) 2014-2023 Viacheslav Dubeyko <slava@dubeyko.com>
 *              http://www.ssdfs.org/
 *
 * (C) Copyright 2014-2019, HGST, Inc., All rights reserved.
 *
 * Created by HGST, San Jose Research Center, Storage Architecture Group
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 *
 * Acknowledgement: Cyril Guyot
 *                  Zvonimir Bandic
 */

#include "ssdfs_tools.h"
#include "blkbmap.h"

/*
 * Table for determination presence of free block
 * state in provided byte. Checking byte is used
 * as index in array.
 */
static
const int detect_free_blk[U8_MAX + 1] = {
/* 00 - 0x00 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 01 - 0x04 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 02 - 0x08 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 03 - 0x0C */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 04 - 0x10 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 05 - 0x14 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 06 - 0x18 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 07 - 0x1C */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 08 - 0x20 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 09 - 0x24 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 10 - 0x28 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 11 - 0x2C */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 12 - 0x30 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 13 - 0x34 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 14 - 0x38 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 15 - 0x3C */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 16 - 0x40 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 17 - 0x44 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 18 - 0x48 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 19 - 0x4C */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 20 - 0x50 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 21 - 0x54 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 22 - 0x58 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 23 - 0x5C */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 24 - 0x60 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 25 - 0x64 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 26 - 0x68 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 27 - 0x6C */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 28 - 0x70 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 29 - 0x74 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 30 - 0x78 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 31 - 0x7C */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 32 - 0x80 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 33 - 0x84 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 34 - 0x88 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 35 - 0x8C */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 36 - 0x90 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 37 - 0x94 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 38 - 0x98 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 39 - 0x9C */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 40 - 0xA0 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 41 - 0xA4 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 42 - 0xA8 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 43 - 0xAC */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 44 - 0xB0 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 45 - 0xB4 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 46 - 0xB8 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 47 - 0xBC */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 48 - 0xC0 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 49 - 0xC4 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 50 - 0xC8 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 51 - 0xCC */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 52 - 0xD0 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 53 - 0xD4 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 54 - 0xD8 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 55 - 0xDC */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 56 - 0xE0 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 57 - 0xE4 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 58 - 0xE8 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 59 - 0xEC */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 60 - 0xF0 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 61 - 0xF4 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 62 - 0xF8 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 63 - 0xFC */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE
};

/*
 * BYTE_CONTAINS_FREE_BLK() - check that byte contains free block
 * @value: pointer on byte
 */
static inline
int BYTE_CONTAINS_FREE_BLK(u8 *value)
{
	return detect_free_blk[*value];
}

/*
 * ssdfs_blkbmap_set_area() - set contiguos area of block bitmap
 * @bmap: block bitmap pointer
 * @start_item: index of start item
 * @items_count: count of items in the area
 * @state: items' state
 */
int ssdfs_blkbmap_set_area(u8 *bmap, u32 start_item,
			   u32 items_count, int state)
{
	u32 items_per_byte;
	u32 aligned_start, aligned_end;
	u32 byte_item;
	u8 *cur_byte;
	u32 bytes_count;
	u32 processed_items = 0;
	u32 i, j;

	BUG_ON(!bmap);

	if (state < SSDFS_BLK_FREE || state >= SSDFS_BLK_STATE_MAX) {
		SSDFS_ERR("invalid state %#x\n",
			  state);
		return -EINVAL;
	}

	items_per_byte = SSDFS_ITEMS_PER_BYTE(SSDFS_BLK_STATE_BITS);
	aligned_start = ALIGNED_START_ITEM(start_item, SSDFS_BLK_STATE_BITS);

	BUG_ON(items_per_byte == 0);

	cur_byte = bmap + (aligned_start / items_per_byte);
	byte_item = start_item - aligned_start;

	aligned_end = ALIGNED_END_ITEM(start_item + items_count,
					SSDFS_BLK_STATE_BITS);
	bytes_count = (aligned_end - aligned_start) / items_per_byte;

	for (i = 0; i < bytes_count; i++) {
		for (j = byte_item; j < items_per_byte; j++) {

			if (processed_items >= items_count)
				goto finish_set_blkbmap;

			SET_STATE_IN_BYTE(cur_byte, j,
					  SSDFS_BLK_STATE_BITS,
					  SSDFS_BLK_STATE_MASK,
					  state);

			processed_items++;
		}

		byte_item = 0;
		cur_byte++;
	}

finish_set_blkbmap:
	return 0;
}
