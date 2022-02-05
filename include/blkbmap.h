//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * include/blkbmap.h - block bitmap declarations.
 *
 * Copyright (c) 2014-2022 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 *
 * HGST Confidential
 * (C) Copyright 2014-2022, HGST, Inc., All rights reserved.
 *
 * Created by HGST, San Jose Research Center, Storage Architecture Group
 * Authors: Vyacheslav Dubeyko <slava@dubeyko.com>
 *
 * Acknowledgement: Cyril Guyot <Cyril.Guyot@wdc.com>
 *                  Zvonimir Bandic <Zvonimir.Bandic@wdc.com>
 */

#ifndef _SSDFS_BLKBMAP_H
#define _SSDFS_BLKBMAP_H

#include "common_bitmap.h"

#define SSDFS_BLK_STATE_BITS	2
#define SSDFS_BLK_STATE_MASK	0x3

enum {
	SSDFS_BLK_FREE		= 0x0,
	SSDFS_BLK_PRE_ALLOCATED	= 0x1,
	SSDFS_BLK_VALID		= 0x3,
	SSDFS_BLK_INVALID	= 0x2,
	SSDFS_BLK_STATE_MAX	= SSDFS_BLK_VALID + 1,
};

#define BLK_BMAP_BYTES(items_count) \
	((items_count + SSDFS_ITEMS_PER_BYTE(SSDFS_BLK_STATE_BITS) - 1)  / \
	 SSDFS_ITEMS_PER_BYTE(SSDFS_BLK_STATE_BITS))

int ssdfs_blkbmap_set_area(u8 *bmap, u32 start_item,
			   u32 items_count, int state);

#endif /* _SSDFS_BLKBMAP_H */
