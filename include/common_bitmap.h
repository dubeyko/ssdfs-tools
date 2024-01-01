/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * include/common_bitmap.h - common bitmap declarations.
 *
 * Copyright (c) 2014-2019 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 * Copyright (c) 2014-2024 Viacheslav Dubeyko <slava@dubeyko.com>
 *              http://www.ssdfs.org/
 *
 * (C) Copyright 2014-2022, HGST, Inc., All rights reserved.
 *
 * Created by HGST, San Jose Research Center, Storage Architecture Group
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 *
 * Acknowledgement: Cyril Guyot
 *                  Zvonimir Bandic
 */

#ifndef _SSDFS_COMMON_BITMAP_H
#define _SSDFS_COMMON_BITMAP_H

#define SSDFS_ITEMS_PER_BYTE(item_bits) ({ \
	BUG_ON(item_bits > BITS_PER_BYTE); \
	BITS_PER_BYTE / item_bits; \
})

#define ALIGNED_START_ITEM(item, state_bits) ({ \
	u64 aligned_start; \
	aligned_start = (item >> state_bits) << state_bits; \
	aligned_start; \
})

#define ALIGNED_END_ITEM(item, state_bits) ({ \
	u64 aligned_end; \
	aligned_end = item + SSDFS_ITEMS_PER_BYTE(state_bits) - 1; \
	aligned_end >>= state_bits; \
	aligned_end <<= state_bits; \
	aligned_end; \
})

typedef int (*byte_check_func)(u8 *value);
typedef u8 (*byte_op_func)(u8 *value, int state, u8 start_off,
			   u8 state_bits, int state_mask);

/*
 * FIRST_STATE_IN_BYTE() - determine first item's offset for requested state
 * @value: pointer on analysed byte
 * @state: requested state
 * @start_off: starting item's offset for analysis beginning
 * @state_bits: bits per state
 * @state_mask: state mask
 *
 * This function tries to determine an item with @state in
 * @value starting from @start_off.
 *
 * RETURN:
 * [success] - found item's offset.
 * [failure] - BITS_PER_BYTE.
 */
static inline
u8 FIRST_STATE_IN_BYTE(u8 *value, int state,
			u8 start_offset, u8 state_bits,
			int state_mask)
{
	u8 i;

	BUG_ON(!value);
	BUG_ON(state_bits > BITS_PER_BYTE);
	BUG_ON((state_bits % 2) != 0);
	BUG_ON(start_offset > SSDFS_ITEMS_PER_BYTE(state_bits));

	i = start_offset * state_bits;
	for (; i < BITS_PER_BYTE; i += state_bits) {
		if (((*value >> i) & state_mask) == state)
			return i / state_bits;
	}

	return SSDFS_ITEMS_PER_BYTE(state_bits);
}

/*
 * FIND_FIRST_ITEM_IN_BYTE() - find item in byte value
 * @value: pointer on analysed byte
 * @state: requested state or mask
 * @state_bits: bits per state
 * @state_mask: state mask
 * @start_offset: starting item's offset for search
 * @check: pointer on check function
 * @op: pointer on concrete operation function
 * @found_offset: pointer on found item's offset into byte for state [out]
 *
 * This function tries to find in byte items with @state starting from
 * @start_offset.
 *
 * RETURN:
 * [success] - @found_offset contains found items' offset into byte.
 * [failure] - error code:
 *
 * %-ENODATA    - analyzed @value doesn't contain any item for @state.
 */
static inline
int FIND_FIRST_ITEM_IN_BYTE(u8 *value, int state, u8 state_bits,
			    int state_mask, u8 start_offset,
			    byte_check_func check,
			    byte_op_func op, u8 *found_offset)
{
	BUG_ON(!value || !found_offset || !check || !op);
	BUG_ON(state_bits > BITS_PER_BYTE);
	BUG_ON((state_bits % 2) != 0);
	BUG_ON(start_offset > SSDFS_ITEMS_PER_BYTE(state_bits));

	*found_offset = U8_MAX;

	if (check(value)) {
		u8 offset = op(value, state, start_offset,
				state_bits, state_mask);

		if (offset < SSDFS_ITEMS_PER_BYTE(state_bits)) {
			*found_offset = offset;
			return 0;
		}
	}

	return -ENODATA;
}

/*
 * SET_STATE_IN_BYTE() - set state of item in byte
 * @byte_ptr: pointer on byte
 * @byte_item: index of item in byte
 * @state_bits: bits per state
 * @state_mask: state mask
 * @new_state: new state value
 */
static inline
void SET_STATE_IN_BYTE(u8 *byte_ptr, u32 byte_item,
			u8 state_bits, int state_mask,
			int new_state)
{
	u8 value;
	int shift = byte_item * state_bits;

	BUG_ON(!byte_ptr);
	BUG_ON(byte_item >= SSDFS_ITEMS_PER_BYTE(state_bits));

	value = new_state & state_mask;
	value <<= shift;

	*byte_ptr &= ~(state_mask << shift);
	*byte_ptr |= value;
}

#endif /* _SSDFS_COMMON_BITMAP_H */
