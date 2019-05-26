//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/mkfs.ssdfs/mapping_table_cache.c - PEB mapping table cache creation.
 *
 * Copyright (c) 2014-2018 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 *
 * HGST Confidential
 * (C) Copyright 2009-2018, HGST, Inc., All rights reserved.
 *
 * Created by HGST, San Jose Research Center, Storage Architecture Group
 * Authors: Vyacheslav Dubeyko <Vyacheslav.Dubeyko@wdc.com>
 *
 * Acknowledgement: Cyril Guyot <Cyril.Guyot@wdc.com>
 *                  Zvonimir Bandic <Zvonimir.Bandic@wdc.com>
 */

#include "mkfs.h"

/************************************************************************
 *             PEB mapping table cache creation functionality           *
 ************************************************************************/

static
int maptbl_cache_create_fragments_array(struct ssdfs_volume_layout *layout)
{
	int segs_capacity = layout->segs_capacity;
	u32 lebs_count = 0;
	u32 fragments_count;
	size_t fragment_size = PAGE_CACHE_SIZE;
	u32 leb2peb_pair_per_fragment;
	u32 i, j;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	for (i = 0; i < segs_capacity; i++) {
		struct ssdfs_segment_desc *segment = &layout->segs[i];
		u16 pebs_capacity = segment->pebs_capacity;

		for (j = 0; j < pebs_capacity; j++) {
			u64 leb_id = segment->pebs[j].leb_id;

			if (leb_id != U64_MAX)
				lebs_count++;
		}
	}

	leb2peb_pair_per_fragment =
		SSDFS_LEB2PEB_PAIR_PER_FRAGMENT(PAGE_CACHE_SIZE);
	fragments_count = lebs_count + leb2peb_pair_per_fragment - 1;
	fragments_count /= leb2peb_pair_per_fragment;

	layout->maptbl_cache.fragments_array =
				calloc(fragments_count, sizeof(void *));
	if (layout->maptbl_cache.fragments_array == NULL) {
		SSDFS_ERR("fail to allocate maptbl cache's fragments array: "
			  "buffers_count %u\n",
			  fragments_count);
		return -ENOMEM;
	}

	for (i = 0; i < fragments_count; i++) {
		layout->maptbl_cache.fragments_array[i] =
						calloc(1, fragment_size);
		if (layout->maptbl_cache.fragments_array[i] == NULL) {
			SSDFS_ERR("fail to allocate fragment buffer: "
				  "index %d\n", i);
			goto free_buffers;
		}
	}

	layout->maptbl_cache.fragment_size = fragment_size;
	layout->maptbl_cache.fragments_count = fragments_count;

	return 0;

free_buffers:
	for (i--; i >= 0; i--) {
		free(layout->maptbl_cache.fragments_array[i]);
		layout->maptbl_cache.fragments_array[i] = NULL;
	}
	free(layout->maptbl_cache.fragments_array);
	layout->maptbl_cache.fragments_array = NULL;
	return -ENOMEM;
}

void maptbl_cache_destroy_fragments_array(struct ssdfs_volume_layout *layout)
{
	u32 fragments_count = layout->maptbl_cache.fragments_count;
	u32 i;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	if (layout->maptbl_cache.fragments_array) {
		for (i = 0; i < fragments_count; i++) {
			free(layout->maptbl_cache.fragments_array[i]);
			layout->maptbl_cache.fragments_array[i] = NULL;
		}
	}
	free(layout->maptbl_cache.fragments_array);
	layout->maptbl_cache.fragments_array = NULL;
}

static
int maptbl_cache_prepare_fragment(struct ssdfs_volume_layout *layout,
				  u16 sequence_id)
{
	u8 *ptr;
	u32 fragments = layout->maptbl_cache.fragments_count;
	struct ssdfs_maptbl_cache_header *hdr;
	size_t magic_size = SSDFS_PEB_STATE_SIZE;
	size_t threshold_size = SSDFS_MAPTBL_CACHE_HDR_SIZE + magic_size;
	__le32 *magic;

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, sequence_id %u\n", layout, sequence_id);

	if (sequence_id >= fragments) {
		SSDFS_ERR("invalid index: sequence_id %u >= fragments %u\n",
			  sequence_id, fragments);
		return -EINVAL;
	}

	ptr = (u8 *)layout->maptbl_cache.fragments_array[sequence_id];
	memset(ptr, 0, PAGE_CACHE_SIZE);

	hdr = (struct ssdfs_maptbl_cache_header *)ptr;

	hdr->magic.common = cpu_to_le32(SSDFS_SUPER_MAGIC);
	hdr->magic.key = cpu_to_le16(SSDFS_MAPTBL_CACHE_MAGIC);
	hdr->magic.version.major = cpu_to_le8(SSDFS_MAJOR_REVISION);
	hdr->magic.version.minor = cpu_to_le8(SSDFS_MINOR_REVISION);

	hdr->sequence_id = cpu_to_le16(sequence_id);
	hdr->flags = 0;
	hdr->items_count = 0;
	hdr->bytes_count = cpu_to_le16((u16)threshold_size);

	hdr->start_leb = cpu_to_le64(U64_MAX);
	hdr->end_leb = cpu_to_le64(U64_MAX);

	magic = (__le32 *)(ptr + SSDFS_MAPTBL_CACHE_HDR_SIZE);
	*magic = cpu_to_le32(SSDFS_MAPTBL_CACHE_PEB_STATE_MAGIC);

	return 0;
}

int maptbl_cache_mkfs_prepare(struct ssdfs_volume_layout *layout)
{
	u32 fragments;
	u32 i;
	int err;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	err = maptbl_cache_create_fragments_array(layout);
	if (err) {
		SSDFS_ERR("fail to create fragments array: err %d\n",
			  err);
		return err;
	}

	fragments = layout->maptbl_cache.fragments_count;
	BUG_ON(fragments >= U16_MAX);

	for (i = 0; i < fragments; i++) {
		err = maptbl_cache_prepare_fragment(layout, (u16)i);
		if (err) {
			SSDFS_ERR("fail to prepare fragment: "
				  "index %u, err %d\n",
				  i, err);
			return err;
		}
	}

	return 0;
}

static
u32 find_fragment_index(struct ssdfs_volume_layout *layout,
			u64 leb_id)
{
	u8 *ptr;
	struct ssdfs_maptbl_cache_header *hdr;
	u16 items_per_fragment;
	u64 start_leb, end_leb;
	u16 items_count;
	u32 i = 0;

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, leb_id %llu\n", layout, leb_id);

	BUG_ON(leb_id == U64_MAX);

	items_per_fragment = SSDFS_LEB2PEB_PAIR_PER_FRAGMENT(PAGE_CACHE_SIZE);

	for (; i < layout->maptbl_cache.fragments_count; i++) {
		ptr = (u8 *)layout->maptbl_cache.fragments_array[i];
		hdr = (struct ssdfs_maptbl_cache_header *)ptr;

		BUG_ON(le16_to_cpu(hdr->magic.key) != SSDFS_MAPTBL_CACHE_MAGIC);

		start_leb = le64_to_cpu(hdr->start_leb);
		end_leb = le64_to_cpu(hdr->end_leb);
		items_count = le16_to_cpu(hdr->items_count);

		if (start_leb == U64_MAX)
			break;
		else if (leb_id >= start_leb && leb_id <= end_leb)
			break;
		else if (leb_id > end_leb && items_count < items_per_fragment)
			break;
	}

	return i;
}

static inline
int __ssdfs_maptbl_cache_area_size(struct ssdfs_maptbl_cache_header *hdr,
				   size_t *leb2peb_area_size,
				   size_t *peb_state_area_size)
{
	size_t hdr_size = sizeof(struct ssdfs_maptbl_cache_header);
	size_t pair_size = sizeof(struct ssdfs_leb2peb_pair);
	size_t peb_state_size = sizeof(struct ssdfs_maptbl_cache_peb_state);
	size_t magic_size = peb_state_size;
	u16 bytes_count;
	u16 items_count;
	size_t threshold_size;
	size_t capacity;

	*leb2peb_area_size = 0;
	*peb_state_area_size = magic_size;

	bytes_count = le16_to_cpu(hdr->bytes_count);
	items_count = le16_to_cpu(hdr->items_count);

	threshold_size = hdr_size + magic_size;

	if (bytes_count < threshold_size) {
		SSDFS_ERR("fragment is corrupted: "
			  "hdr_size %zu, bytes_count %u\n",
			  hdr_size, bytes_count);
		return -ERANGE;
	} else if (bytes_count == threshold_size) {
		return -ENODATA;
	}

	capacity =
		(bytes_count - threshold_size) / (pair_size + peb_state_size);

	if (items_count > capacity) {
		SSDFS_ERR("items_count %u > capacity %zu\n",
			  items_count, capacity);
		return -ERANGE;
	}

	*leb2peb_area_size = capacity * pair_size;
	*peb_state_area_size = magic_size + (capacity * peb_state_size);

	return 0;
}

static inline
int ssdfs_peb_state_area_size(struct ssdfs_maptbl_cache_header *hdr)
{
	size_t leb2peb_area_size;
	size_t peb_state_area_size;
	int err;

	err = __ssdfs_maptbl_cache_area_size(hdr,
					     &leb2peb_area_size,
					     &peb_state_area_size);
	if (err == -ENODATA) {
		/* empty area */
		err = 0;
	} else if (err) {
		SSDFS_ERR("fail to define peb state area size: "
			  "err %d\n",
			  err);
		return err;
	}

	return (int)peb_state_area_size;
}

static inline
struct ssdfs_leb2peb_pair *LEB2PEB_PAIR_AREA(u8 *fragment)
{
	size_t hdr_size = sizeof(struct ssdfs_maptbl_cache_header);

	return (struct ssdfs_leb2peb_pair *)(fragment + hdr_size);
}

static inline
void *PEB_STATE_AREA(u8 *fragment)
{
	struct ssdfs_maptbl_cache_header *hdr;
	size_t hdr_size = sizeof(struct ssdfs_maptbl_cache_header);
	size_t leb2peb_area_size;
	size_t peb_state_area_size;
	void *start = NULL;
	__le32 *magic = NULL;
	int err;

	hdr = (struct ssdfs_maptbl_cache_header *)fragment;

	err = __ssdfs_maptbl_cache_area_size(hdr,
					     &leb2peb_area_size,
					     &peb_state_area_size);
	if (err == -ENODATA) {
		/* empty area */
		err = 0;
	} else if (err) {
		SSDFS_ERR("fail to get area size: err %d\n", err);
		return ERR_PTR(err);
	}

	start = fragment + hdr_size + leb2peb_area_size;
	magic = (__le32 *)start;

	if (le32_to_cpu(*magic) != SSDFS_MAPTBL_CACHE_PEB_STATE_MAGIC) {
		SSDFS_ERR("invalid magic %#x\n",
			  le32_to_cpu(*magic));
		return ERR_PTR(-ERANGE);
	}

	return start;
}

static inline
struct ssdfs_maptbl_cache_peb_state *FIRST_PEB_STATE(u8 *fragment)
{
	size_t peb_state_size = sizeof(struct ssdfs_maptbl_cache_peb_state);
	size_t magic_size = peb_state_size;
	void *start = PEB_STATE_AREA(fragment);

	if (IS_ERR(start))
		return (struct ssdfs_maptbl_cache_peb_state *)start;

	if (!start)
		return ERR_PTR(-ERANGE);

	return (struct ssdfs_maptbl_cache_peb_state *)((u8 *)start +
							magic_size);
}

static
int add_leb2peb_pair(u8 *fragment,
		     struct ssdfs_leb2peb_pair *new_pair,
		     struct ssdfs_leb2peb_pair *moving_pair,
		     struct ssdfs_maptbl_cache_peb_state *new_state,
		     struct ssdfs_maptbl_cache_peb_state *moving_state)
{
	struct ssdfs_maptbl_cache_header *hdr;
	u16 items_per_fragment;
	u16 items_count;
	struct ssdfs_leb2peb_pair *leb2peb_pairs = NULL;
	size_t pair_size = sizeof(struct ssdfs_leb2peb_pair);
	struct ssdfs_maptbl_cache_peb_state *peb_states = NULL;
	size_t peb_state_size = sizeof(struct ssdfs_maptbl_cache_peb_state);
	size_t magic_size = peb_state_size;
	u16 item_index = 0;
	u16 bytes_count;
	int need_moving = SSDFS_FALSE;
	int i;
	int err;

	hdr = (struct ssdfs_maptbl_cache_header *)fragment;

	items_per_fragment = SSDFS_LEB2PEB_PAIR_PER_FRAGMENT(PAGE_CACHE_SIZE);
	items_count = le16_to_cpu(hdr->items_count);
	BUG_ON(items_count > items_per_fragment);

	leb2peb_pairs = LEB2PEB_PAIR_AREA(fragment);

	peb_states = FIRST_PEB_STATE(fragment);
	if (IS_ERR(peb_states)) {
		SSDFS_ERR("fail to get first PEB state: "
			  "err %d\n",
			  (int)PTR_ERR(peb_states));
		return (int)PTR_ERR(peb_states);
	}

	if (items_count == items_per_fragment) {
		memcpy(moving_pair,
			&leb2peb_pairs[items_count - 1], pair_size);
		memcpy(moving_state,
			&peb_states[items_count - 1], peb_state_size);
		need_moving = SSDFS_TRUE;
	} else {
		u8 *area = NULL;
		int area_size;

		area = PEB_STATE_AREA(fragment);
		if (IS_ERR(area)) {
			SSDFS_ERR("fail to get PEB states  area: "
				  "err %d\n",
				  (int)PTR_ERR(area));
			return (int)PTR_ERR(area);
		}

		area_size = ssdfs_peb_state_area_size(hdr);
		if (area_size < 0) {
			err = area_size < 0 ? area_size : -ERANGE;
			SSDFS_ERR("fail to define the area size: "
				  "err %d\n", err);
			return err;
		} else if (area_size > 0) {
			memmove(area + pair_size, area, area_size);
			peb_states =
			    (struct ssdfs_maptbl_cache_peb_state *)(area +
								    magic_size +
								    pair_size);
		}
	}

	if (items_count > 0) {
		for (i = items_count - 1; i >= 0; i--) {
			u64 cur_leb_id = le64_to_cpu(leb2peb_pairs[i].leb_id);

			BUG_ON(cur_leb_id == U64_MAX);

			if (cur_leb_id <= le64_to_cpu(new_pair->leb_id))
				break;
		}

		item_index = i + 1;

		if (item_index < items_count) {
			u16 moving_items;

			if (items_count == items_per_fragment)
				moving_items = items_count - 1 - item_index;
			else
				moving_items = items_count - item_index;

			memmove(&leb2peb_pairs[item_index + 1],
				&leb2peb_pairs[item_index],
				pair_size * moving_items);
			memmove(&peb_states[item_index + 1],
				&peb_states[item_index],
				peb_state_size * moving_items);
		}
	}

	memcpy(&leb2peb_pairs[item_index], new_pair, pair_size);
	memcpy(&peb_states[item_index], new_state, peb_state_size);

	memcpy(&hdr->start_leb, &leb2peb_pairs[0].leb_id,
		sizeof(hdr->start_leb));
	memcpy(&hdr->end_leb, &leb2peb_pairs[items_count].leb_id,
		sizeof(hdr->end_leb));

	items_count++;
	hdr->items_count = cpu_to_le16(items_count);

	bytes_count = SSDFS_MAPTBL_CACHE_HDR_SIZE + magic_size;
	bytes_count += items_count * (pair_size + peb_state_size);
	hdr->bytes_count = cpu_to_le16(bytes_count);

	return need_moving ? -EAGAIN : 0;
}

int cache_leb2peb_pair(struct ssdfs_volume_layout *layout,
			u64 leb_id, u64 peb_id)
{
	u8 *ptr;
	struct ssdfs_leb2peb_pair new_pair, moving_pair;
	size_t pair_size = sizeof(struct ssdfs_leb2peb_pair);
	struct ssdfs_maptbl_cache_peb_state new_state, moving_state;
	size_t peb_state_size = sizeof(struct ssdfs_maptbl_cache_peb_state);
	u32 index = 0;
	int res = 0;

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, leb_id %llu, peb_id %llu\n",
		  layout, leb_id, peb_id);

	BUG_ON(leb_id == U64_MAX);
	BUG_ON(peb_id == U64_MAX);

	moving_pair.leb_id = cpu_to_le64(leb_id);
	moving_pair.peb_id = cpu_to_le64(peb_id);

	moving_state.consistency = (u8)SSDFS_PEB_STATE_CONSISTENT;
	moving_state.state = (u8)SSDFS_MAPTBL_USING_PEB_STATE;
	moving_state.flags = 0;
	moving_state.shared_peb_index = U8_MAX;

	index = find_fragment_index(layout, leb_id);

	do {
		BUG_ON(index >= layout->maptbl_cache.fragments_count);
		ptr = (u8 *)layout->maptbl_cache.fragments_array[index];

		memcpy(&new_pair, &moving_pair, pair_size);
		memcpy(&new_state, &moving_state, peb_state_size);

		res = add_leb2peb_pair(ptr, &new_pair, &moving_pair,
					&new_state, &moving_state);

		if (res == -EAGAIN) {
			/*
			 * continue to move
			 */
			index++;
		} else if (res) {
			SSDFS_ERR("fail to add new pair: "
				  "fragment_index %u, err %d\n",
				  index, res);
			return res;
		} else
			return 0;
	} while (res == -EAGAIN);

	return 0;
}
