/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * include/segbmap.h - segment bitmap declarations.
 *
 * Copyright (c) 2014-2019 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 * Copyright (c) 2014-2024 Viacheslav Dubeyko <slava@dubeyko.com>
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

#ifndef _SSDFS_SEGBMAP_H
#define _SSDFS_SEGBMAP_H

#include "common_bitmap.h"

/* Segment states */
enum {
	SSDFS_SEG_CLEAN			= 0x0,
	SSDFS_SEG_DATA_USING		= 0x1,
	SSDFS_SEG_LEAF_NODE_USING	= 0x2,
	SSDFS_SEG_HYBRID_NODE_USING	= 0x5,
	SSDFS_SEG_INDEX_NODE_USING	= 0x3,
	SSDFS_SEG_USED			= 0x7,
	SSDFS_SEG_PRE_DIRTY		= 0x6,
	SSDFS_SEG_DIRTY			= 0x4,
	SSDFS_SEG_BAD			= 0x8,
	SSDFS_SEG_RESERVED		= 0x9,
	SSDFS_SEG_STATE_MAX		= SSDFS_SEG_RESERVED + 1,
};

#define SSDFS_SEG_STATE_BITS	4
#define SSDFS_SEG_STATE_MASK	0xF

/* lib/segbmap.c */
u32 SEG_BMAP_BYTES(u64 items_count);
u16 SEG_BMAP_FRAGMENTS(u64 items_count, u32 page_size);
u32 ssdfs_segbmap_payload_bytes_per_fragment(size_t fragment_size);
u32 ssdfs_segbmap_items_per_fragment(size_t fragment_size);
u64 ssdfs_segbmap_define_first_fragment_item(int fragment_index,
					     size_t fragment_size);
int SET_FIRST_CLEAN_ITEM_IN_FRAGMENT(struct ssdfs_segbmap_fragment_header *hdr,
				     u8 *fragment, u64 start_item, u64 max_item,
				     u32 page_size, int state, u64 *found_seg);

#endif /* _SSDFS_SEGBMAP_H */
