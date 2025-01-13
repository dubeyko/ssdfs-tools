/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * lib/segbmap.c - segment bitmap functionality.
 *
 * Copyright (c) 2014-2019 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 * Copyright (c) 2014-2025 Viacheslav Dubeyko <slava@dubeyko.com>
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
#include "segbmap.h"

u32 SEG_BMAP_BYTES(u64 items_count)
{
	u64 bytes;

	bytes = items_count + SSDFS_ITEMS_PER_BYTE(SSDFS_SEG_STATE_BITS) - 1;
	bytes /= SSDFS_ITEMS_PER_BYTE(SSDFS_SEG_STATE_BITS);

	BUG_ON(bytes >= U32_MAX);

	return (u32)bytes;
}

u16 SEG_BMAP_FRAGMENTS(u64 items_count, u32 page_size)
{
	u32 hdr_size = sizeof(struct ssdfs_segbmap_fragment_header);
	u32 bytes = SEG_BMAP_BYTES(items_count);
	u32 pages, fragments;

	pages = (bytes + page_size - 1) / page_size;
	bytes += pages * hdr_size;

	fragments = (bytes + page_size - 1) / page_size;
	BUG_ON(fragments >= U16_MAX);

	return (u16)fragments;
}

u32 ssdfs_segbmap_payload_bytes_per_fragment(size_t fragment_size)
{
	u32 hdr_size = sizeof(struct ssdfs_segbmap_fragment_header);

	BUG_ON(hdr_size >= fragment_size);
	return fragment_size - hdr_size;
}

u32 ssdfs_segbmap_items_per_fragment(size_t fragment_size)
{
	u32 hdr_size = sizeof(struct ssdfs_segbmap_fragment_header);
	u32 payload_bytes;
	u64 items;

	BUG_ON(hdr_size >= fragment_size);

	payload_bytes = fragment_size - hdr_size;
	items = payload_bytes * SSDFS_ITEMS_PER_BYTE(SSDFS_SEG_STATE_BITS);

	BUG_ON(items >= U32_MAX);

	return (u32)items;
}

u64 ssdfs_segbmap_define_first_fragment_item(int fragment_index,
					     size_t fragment_size)
{
	return (u64)fragment_index *
		    ssdfs_segbmap_items_per_fragment(fragment_size);
}

/*
 * Table for determination presence of clean segment
 * state in provided byte. Checking byte is used
 * as index in array.
 */
const int detect_clean_seg[U8_MAX + 1] = {
/* 00 - 0x00 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 01 - 0x04 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 02 - 0x08 */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 03 - 0x0C */	SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE, SSDFS_TRUE,
/* 04 - 0x10 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 05 - 0x14 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 06 - 0x18 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 07 - 0x1C */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 08 - 0x20 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 09 - 0x24 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 10 - 0x28 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 11 - 0x2C */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 12 - 0x30 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 13 - 0x34 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 14 - 0x38 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 15 - 0x3C */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 16 - 0x40 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 17 - 0x44 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 18 - 0x48 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 19 - 0x4C */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 20 - 0x50 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 21 - 0x54 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 22 - 0x58 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 23 - 0x5C */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 24 - 0x60 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 25 - 0x64 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 26 - 0x68 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 27 - 0x6C */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 28 - 0x70 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 29 - 0x74 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 30 - 0x78 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 31 - 0x7C */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 32 - 0x80 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 33 - 0x84 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 34 - 0x88 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 35 - 0x8C */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 36 - 0x90 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 37 - 0x94 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 38 - 0x98 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 39 - 0x9C */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 40 - 0xA0 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 41 - 0xA4 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 42 - 0xA8 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 43 - 0xAC */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 44 - 0xB0 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 45 - 0xB4 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 46 - 0xB8 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 47 - 0xBC */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 48 - 0xC0 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 49 - 0xC4 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 50 - 0xC8 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 51 - 0xCC */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 52 - 0xD0 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 53 - 0xD4 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 54 - 0xD8 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 55 - 0xDC */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 56 - 0xE0 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 57 - 0xE4 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 58 - 0xE8 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 59 - 0xEC */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 60 - 0xF0 */	SSDFS_TRUE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 61 - 0xF4 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 62 - 0xF8 */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE,
/* 63 - 0xFC */	SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE, SSDFS_FALSE
};

/*
 * BYTE_CONTAINS_CLEAN_STATE() - check that byte contains clean state
 * @value: pointer on byte
 */
static inline
int BYTE_CONTAINS_CLEAN_STATE(u8 *value)
{
	return detect_clean_seg[*value];
}

/*
 * SET_FIRST_CLEAN_ITEM_IN_FRAGMENT() - find and set the first clean item
 * @hdr: pointer on segbmap fragment's header
 * @fragment: pointer on bitmap in fragment
 * @start_item: start segment number for search
 * @max_item: upper bound of segment number for search
 * @state: state for setting
 * @found_seg: found segment number [out]
 *
 * This method tries to find first item with requested
 * state in fragment.
 *
 * RETURN:
 * [success]
 * [failure] - error code:
 *
 * %-EINVAL     - invalid input.
 * %-ERANGE     - internal error.
 * %-ENODATA    - fragment doesn't include segment with requested state.
 */
int SET_FIRST_CLEAN_ITEM_IN_FRAGMENT(struct ssdfs_segbmap_fragment_header *hdr,
				     u8 *fragment, u64 start_item, u64 max_item,
				     u32 page_size, int state, u64 *found_seg)
{
	u32 items_per_byte = SSDFS_ITEMS_PER_BYTE(SSDFS_SEG_STATE_BITS);
	u64 fragment_start_item;
	u64 aligned_start, aligned_end;
	u32 byte_index, search_bytes;
	u64 byte_range;
	u8 start_offset;
	int err = 0;

	BUG_ON(!hdr || !fragment || !found_seg);

	if (start_item >= max_item) {
		SSDFS_ERR("start_item %llu >= max_item %llu\n",
			  start_item, max_item);
		return -EINVAL;
	}

	*found_seg = U64_MAX;

	fragment_start_item = le64_to_cpu(hdr->start_item);

	if (fragment_start_item == U64_MAX) {
		SSDFS_ERR("invalid fragment start item\n");
		return -ERANGE;
	}

	search_bytes = le16_to_cpu(hdr->fragment_bytes) -
			sizeof(struct ssdfs_segbmap_fragment_header);

	if (search_bytes == 0 || search_bytes > page_size) {
		SSDFS_ERR("invalid fragment_bytes %u\n",
			  search_bytes);
		return -ERANGE;
	}

	aligned_start = ALIGNED_START_ITEM(start_item, SSDFS_SEG_STATE_BITS);
	aligned_end = ALIGNED_END_ITEM(max_item, SSDFS_SEG_STATE_BITS);

	byte_range = (aligned_end - fragment_start_item) / items_per_byte;
	BUG_ON(byte_range >= U32_MAX);

	search_bytes = min_t(u32, search_bytes, (u32)byte_range);

	if (fragment_start_item <= aligned_start) {
		u32 items_range = aligned_start - fragment_start_item;
		byte_index = items_range / items_per_byte;
		start_offset = (u8)(start_item - aligned_start);
	} else {
		byte_index = 0;
		start_offset = 0;
	}

	for (; byte_index < search_bytes; byte_index++) {
		u8 *value = fragment + byte_index;
		u8 found_offset;

		err = FIND_FIRST_ITEM_IN_BYTE(value, SSDFS_SEG_CLEAN,
					      SSDFS_SEG_STATE_BITS,
					      SSDFS_SEG_STATE_MASK,
					      start_offset,
					      BYTE_CONTAINS_CLEAN_STATE,
					      FIRST_STATE_IN_BYTE,
					      &found_offset);
		if (err == -ENODATA) {
			start_offset = 0;
			continue;
		} else if (err) {
			SSDFS_ERR("fail to find items in byte: "
				  "start_offset %u, state %#x, "
				  "err %d\n",
				  start_offset, state, err);
			goto end_search;
		} else {
			SET_STATE_IN_BYTE(value, found_offset,
					  SSDFS_SEG_STATE_BITS,
					  SSDFS_SEG_STATE_MASK,
					  state);

			*found_seg = fragment_start_item;
			*found_seg += byte_index * items_per_byte;
			*found_seg += found_offset;

			if (*found_seg >= max_item)
				*found_seg = U64_MAX;

			break;
		}
	}

	if (*found_seg == U64_MAX)
		err = -ENODATA;

end_search:
	return err;
}
