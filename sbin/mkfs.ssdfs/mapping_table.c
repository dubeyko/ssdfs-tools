/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/mkfs.ssdfs/mapping_table.c - PEB mapping table creation functionality.
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

#include "mkfs.h"

/************************************************************************
 *              PEB mapping table creation functionality                *
 ************************************************************************/

static inline
void correct_maptbl_stripes_per_portion(struct ssdfs_volume_layout *layout)
{
	u16 stripes = layout->maptbl.stripes_per_portion;
	u32 nand_dies = layout->nand_dies_count;
	int value_corrected = SSDFS_FALSE;

	if (stripes > nand_dies) {
		if (stripes % nand_dies) {
			stripes -= stripes % nand_dies;
			value_corrected = SSDFS_TRUE;
		}
	} else if (stripes < nand_dies) {
		if (nand_dies % stripes) {
			stripes -= nand_dies % stripes;
			value_corrected = SSDFS_TRUE;
		}
	}

	if (value_corrected) {
		layout->maptbl.stripes_per_portion = stripes;

		SSDFS_INFO("maptbl layout is corrected: "
			   "stripes_per_portion %u\n",
			   stripes);
	}
}

int maptbl_mkfs_allocation_policy(struct ssdfs_volume_layout *layout,
				   int *segs)
{
	int seg_state = SSDFS_DEDICATED_SEGMENT;
	u64 seg_nums;
	u64 fragments;
	u32 portions_per_fragment;
	u16 stripes_per_portion;
	u32 portion_size;
	u64 pebs_per_volume;
	u64 portions_per_volume;
	u32 pebs_per_seg;
	u32 maptbl_segs;
	u32 maptbl_pebs;
	u32 pebtbl_portion_bytes;
	u32 pebtbl_portion_mempages;
	u32 peb_desc_per_stripe;
	u32 peb_desc_per_portion;
	u32 lebtbl_portion_bytes;
	u32 lebtbl_mempages;
	u32 lebtbl_portion_mempages;
	u32 leb_desc_per_mempage;
	u32 leb_desc_per_portion;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	seg_nums = layout->env.fs_size / layout->seg_size;
	pebs_per_seg = (u32)(layout->seg_size / layout->env.erase_size);

	pebs_per_volume = layout->env.fs_size / layout->env.erase_size;
	BUG_ON(pebs_per_volume < seg_nums);
	BUG_ON(pebs_per_volume % seg_nums);

define_stripes_per_portion:
	correct_maptbl_stripes_per_portion(layout);
	stripes_per_portion = layout->maptbl.stripes_per_portion;

	leb_desc_per_mempage = SSDFS_LEB_DESC_PER_FRAGMENT(PAGE_CACHE_SIZE);
	peb_desc_per_stripe = SSDFS_PEB_DESC_PER_FRAGMENT(PAGE_CACHE_SIZE);

	BUG_ON((leb_desc_per_mempage / peb_desc_per_stripe) != 2);

	peb_desc_per_portion = peb_desc_per_stripe * stripes_per_portion;

	if (leb_desc_per_mempage > peb_desc_per_portion) {
		stripes_per_portion =
			leb_desc_per_mempage / peb_desc_per_stripe;
		layout->maptbl.stripes_per_portion = stripes_per_portion;
	} else if (leb_desc_per_mempage < peb_desc_per_portion) {
		stripes_per_portion =
			peb_desc_per_portion / leb_desc_per_mempage;
		layout->maptbl.stripes_per_portion = stripes_per_portion;
	}

	switch (layout->env.device_type) {
	case SSDFS_MTD_DEVICE:
	case SSDFS_BLK_DEVICE:
		/* do nothing */
		break;

	case SSDFS_ZNS_DEVICE:
		portions_per_volume = pebs_per_volume + peb_desc_per_portion - 1;
		portions_per_volume /= peb_desc_per_portion;

		if (portions_per_volume > U16_MAX) {
			SSDFS_ERR("portions_per_volume %llu is too huge\n",
				  portions_per_volume);
			return -E2BIG;
		}

		stripes_per_portion = (u16) portions_per_volume;
		layout->maptbl.stripes_per_portion = stripes_per_portion;
		break;

	default:
		BUG();
	};

	if (pebs_per_seg > stripes_per_portion) {
		u32 leb_index_per_stripe;

		leb_index_per_stripe = pebs_per_seg + stripes_per_portion - 1;
		leb_index_per_stripe /= stripes_per_portion;

		peb_desc_per_stripe = peb_desc_per_stripe / leb_index_per_stripe;
		peb_desc_per_stripe *= leb_index_per_stripe;
	}

	/* re-calculate peb_desc_per_portion */
	peb_desc_per_portion = peb_desc_per_stripe * stripes_per_portion;
	/*
	 * every PEB table's memory page needs
	 * to be located into physical page
	 */
	pebtbl_portion_bytes = stripes_per_portion * layout->page_size;
	pebtbl_portion_mempages = stripes_per_portion;

	leb_desc_per_portion = peb_desc_per_portion + leb_desc_per_mempage - 1;
	lebtbl_mempages = leb_desc_per_portion / leb_desc_per_mempage;
	leb_desc_per_portion = lebtbl_mempages * leb_desc_per_mempage;
	/*
	 * every LEB table's memory page needs
	 * to be located into physical page
	 */
	lebtbl_portion_bytes = lebtbl_mempages * layout->page_size;
	lebtbl_portion_mempages = lebtbl_mempages;
	leb_desc_per_portion = peb_desc_per_portion;

	portion_size = lebtbl_portion_bytes + pebtbl_portion_bytes;

	if (portion_size > layout->env.erase_size) {
		u32 diff_size;
		u32 diff_stripes;

		SSDFS_INFO("incorrect maptbl fragment size: "
			   "portion_size %u, erase_size %u\n",
			   portion_size, layout->env.erase_size);
		SSDFS_INFO("try to correct maptbl stripes per fragment\n");

		diff_size = layout->env.erase_size - portion_size;
		diff_stripes = diff_size + PAGE_CACHE_SIZE - 1;
		diff_stripes /= PAGE_CACHE_SIZE;

		BUG_ON(diff_stripes >= stripes_per_portion);

		layout->maptbl.stripes_per_portion -= diff_stripes;
		goto define_stripes_per_portion;
	}

	fragments = pebs_per_volume + peb_desc_per_portion - 1;
	fragments /= peb_desc_per_portion;
	BUG_ON(fragments >= U32_MAX);

	portions_per_fragment = layout->env.erase_size / portion_size;

	if (portions_per_fragment < layout->maptbl.portions_per_fragment) {
		u32 bytes_count;

		bytes_count = layout->maptbl.portions_per_fragment * portion_size;

		if (bytes_count > layout->env.erase_size) {
			u32 diff_size;
			u32 diff_fragments;
			u32 corrected_value;

			diff_size = bytes_count - layout->env.erase_size;
			diff_fragments = diff_size + portion_size - 1;
			diff_fragments /= portion_size;
			corrected_value = layout->maptbl.portions_per_fragment -
					  diff_fragments;

			portions_per_fragment = max_t(u32, portions_per_fragment,
						  corrected_value);
			BUG_ON(portions_per_fragment >= U16_MAX);
			layout->maptbl.portions_per_fragment =
						(u16)portions_per_fragment;

			SSDFS_INFO("corrected maptbl portions_per_fragment %u\n",
				   portions_per_fragment);
		} else {
			/* use portions_per_fragment is requested by user */
			portions_per_fragment = layout->maptbl.portions_per_fragment;
		}
	} else if (portions_per_fragment > layout->maptbl.portions_per_fragment) {
		/* use portions_per_fragment is requested by user */
		portions_per_fragment = layout->maptbl.portions_per_fragment;
	}

	maptbl_pebs = (fragments + portions_per_fragment - 1) / portions_per_fragment;
	maptbl_segs = (maptbl_pebs + pebs_per_seg - 1) / pebs_per_seg;

	maptbl_pebs = maptbl_segs * pebs_per_seg;
	fragments = maptbl_pebs * portions_per_fragment;

	if (layout->maptbl.has_backup_copy)
		*segs = maptbl_segs * 2;
	else
		*segs = maptbl_segs;

	if (*segs > ((seg_nums * 10) / 100)) {
		SSDFS_ERR("maptbl is huge: "
			  "maptbl_segs %d, seg_nums %llu\n",
			  *segs, seg_nums);
		return -E2BIG;
	}

	if (layout->maptbl.reserved_pebs_per_fragment >= U16_MAX) {
		layout->maptbl.reserved_pebs_per_fragment =
			SSDFS_MAPTBL_RESERVED_PEBS_DEFAULT;
	}

	layout->maptbl.maptbl_pebs = maptbl_pebs;
	layout->maptbl.lebtbl_portion_bytes = lebtbl_portion_bytes;
	layout->maptbl.lebtbl_portion_mempages = lebtbl_portion_mempages;
	layout->maptbl.pebtbl_portion_bytes = pebtbl_portion_bytes;
	layout->maptbl.pebtbl_portion_mempages = pebtbl_portion_mempages;
	BUG_ON(leb_desc_per_portion >= U16_MAX);

	layout->maptbl.lebs_per_portion = (u16)leb_desc_per_portion;
	BUG_ON(peb_desc_per_portion >= U16_MAX);
	layout->maptbl.pebs_per_portion = (u16)peb_desc_per_portion;
	layout->maptbl.portions_count = fragments;
	layout->maptbl.portion_size = portion_size;

	layout->meta_array[SSDFS_PEB_MAPPING_TABLE].segs_count = *segs;
	layout->meta_array[SSDFS_PEB_MAPPING_TABLE].seg_state = seg_state;

	SSDFS_DBG(layout->env.show_debug,
		  "maptbl: segs %d, stripes_per_portion %u, "
		  "portions_per_fragment %u, maptbl_pebs %u, "
		  "lebtbl_portion_bytes %u, pebtbl_portion_bytes %u, "
		  "lebtbl_portion_mempages %u, pebtbl_portion_mempages %u, "
		  "lebs_per_portion %u, pebs_per_portion %u, "
		  "portions_count %llu, portion_size %u\n",
		  *segs, stripes_per_portion, portions_per_fragment,
		  maptbl_pebs, lebtbl_portion_bytes,
		  pebtbl_portion_bytes, lebtbl_portion_mempages,
		  pebtbl_portion_mempages, leb_desc_per_portion,
		  peb_desc_per_portion, fragments, portion_size);

	return seg_state;
}

static
int maptbl_create_fragments_array(struct ssdfs_volume_layout *layout)
{
	u32 maptbl_pebs = layout->maptbl.maptbl_pebs;
	size_t portion_size = layout->maptbl.portion_size;
	u16 portions_per_fragment = layout->maptbl.portions_per_fragment;
	size_t peb_buffer_size = portion_size * portions_per_fragment;
	u32 i;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	layout->maptbl.fragments_array = calloc(maptbl_pebs, sizeof(void *));
	if (layout->maptbl.fragments_array == NULL) {
		SSDFS_ERR("fail to allocate maptbl's fragments array: "
			  "buffers_count %u\n",
			  maptbl_pebs);
		return -ENOMEM;
	}

	for (i = 0; i < maptbl_pebs; i++) {
		layout->maptbl.fragments_array[i] = calloc(1, peb_buffer_size);
		if (layout->maptbl.fragments_array[i] == NULL) {
			SSDFS_ERR("fail to allocate PEB's buffer: "
				  "index %d\n", i);
			goto free_buffers;
		}
	}

	return 0;

free_buffers:
	for (i--; i >= 0; i--) {
		free(layout->maptbl.fragments_array[i]);
		layout->maptbl.fragments_array[i] = NULL;
	}
	free(layout->maptbl.fragments_array);
	layout->maptbl.fragments_array = NULL;
	return -ENOMEM;
}

void maptbl_destroy_fragments_array(struct ssdfs_volume_layout *layout)
{
	u32 maptbl_pebs = layout->maptbl.maptbl_pebs;
	u32 i;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	if (layout->maptbl.fragments_array) {
		for (i = 0; i < maptbl_pebs; i++) {
			free(layout->maptbl.fragments_array[i]);
			layout->maptbl.fragments_array[i] = NULL;
		}
	}
	free(layout->maptbl.fragments_array);
	layout->maptbl.fragments_array = NULL;
}

static
void maptbl_prepare_leb_table(struct ssdfs_volume_layout *layout,
				u8 *ptr, u16 portion_index,
				u16 mempage_index)
{
	struct ssdfs_leb_table_fragment_header *hdr;
	size_t hdr_size = sizeof(struct ssdfs_leb_table_fragment_header);
	u32 lebtbl_portion_bytes = layout->maptbl.lebtbl_portion_bytes;
	u16 lebs_per_portion = layout->maptbl.lebs_per_portion;
	u32 leb_desc_per_mempage;
	u16 lebtbl_mempages;
	u64 start_portion_leb;
	u64 start_fragment_leb;
	u64 pebs_per_volume;
	u64 lebs_count;
	u32 bytes_count;

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, ptr %p, portion_index %u, "
		  "mempage_index %u\n",
		  layout, ptr, portion_index, mempage_index);

	leb_desc_per_mempage = SSDFS_LEB_DESC_PER_FRAGMENT(PAGE_CACHE_SIZE);
	lebtbl_mempages = (u16)(lebtbl_portion_bytes / layout->page_size);
	BUG_ON(lebtbl_mempages == 0);
	BUG_ON(mempage_index >= lebtbl_mempages);

	start_portion_leb = (u64)lebs_per_portion * portion_index;
	start_fragment_leb = start_portion_leb +
				((u64)leb_desc_per_mempage * mempage_index);

	pebs_per_volume = layout->env.fs_size / layout->env.erase_size;

	if (pebs_per_volume <= start_fragment_leb)
		lebs_count = 0;
	else {
		lebs_count = pebs_per_volume - start_fragment_leb;
		lebs_count = min_t(u64, lebs_count, lebs_per_portion);
		lebs_count = min_t(u64, lebs_count, leb_desc_per_mempage);
	}

	SSDFS_DBG(layout->env.show_debug,
		  "start_portion_leb %llu, start_fragment_leb %llu, "
		  "pebs_per_volume %llu, "
		  "lebs_per_portion %u, lebs_count %llu\n",
		  start_portion_leb, start_fragment_leb,
		  pebs_per_volume, lebs_per_portion, lebs_count);

	bytes_count = hdr_size;
	bytes_count += lebs_count * sizeof(struct ssdfs_leb_descriptor);

	hdr = (struct ssdfs_leb_table_fragment_header *)ptr;

	hdr->magic = cpu_to_le16(SSDFS_LEB_TABLE_MAGIC);
	hdr->flags = 0;

	if (mempage_index == 0)
		hdr->start_leb = cpu_to_le64(start_portion_leb);
	else
		hdr->start_leb = cpu_to_le64(start_fragment_leb);

	BUG_ON(lebs_count >= U16_MAX);
	hdr->lebs_count = cpu_to_le16(lebs_count);

	hdr->mapped_lebs = 0;
	hdr->migrating_lebs = 0;

	hdr->portion_id = cpu_to_le16(portion_index);
	hdr->fragment_id = cpu_to_le16(mempage_index);

	hdr->bytes_count = cpu_to_le32(bytes_count);

	memset(ptr + hdr_size, 0xFF, PAGE_CACHE_SIZE - hdr_size);
}

static
void maptbl_prepare_peb_table(struct ssdfs_volume_layout *layout,
				u8 *ptr,  u16 portion_index,
				u16 stripe_index)
{
	struct ssdfs_peb_table_fragment_header *hdr;
	size_t hdr_size = sizeof(struct ssdfs_peb_table_fragment_header);
	u64 pebs_per_portion = layout->maptbl.pebs_per_portion;
	u16 reserved_pebs_pct = layout->maptbl.reserved_pebs_per_fragment;
	u64 reserved_pebs;
	u16 stripes_per_portion;
	u32 peb_desc_per_stripe;
	u64 pebs_per_volume;
	u64 start_peb;
	u64 pebs_count;
	u32 bytes_count;
	u64 rest_pebs;

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, ptr %p, portion_index %u, "
		  "stripe_index %u\n",
		  layout, ptr, portion_index, stripe_index);

	pebs_per_volume = layout->env.fs_size / layout->env.erase_size;
	stripes_per_portion = layout->maptbl.stripes_per_portion;

	BUG_ON(stripe_index >= stripes_per_portion);

	rest_pebs = pebs_per_volume - (pebs_per_portion * portion_index);
	rest_pebs = min_t(u64, rest_pebs, pebs_per_portion);

	peb_desc_per_stripe = rest_pebs / stripes_per_portion;
	if (rest_pebs % stripes_per_portion)
		peb_desc_per_stripe++;

	start_peb = ((u64)pebs_per_portion * portion_index) +
			((u64)peb_desc_per_stripe * stripe_index);

	if (pebs_per_volume <= start_peb)
		pebs_count = 0;
	else {
		pebs_count = pebs_per_volume -
			(pebs_per_portion * portion_index);
		pebs_count = min_t(u64, pebs_count, pebs_per_portion);
		pebs_count += pebs_count % stripes_per_portion;
		pebs_count /= stripes_per_portion;

		if ((start_peb + pebs_count) > pebs_per_volume)
			pebs_count = pebs_per_volume - start_peb;
	}

	SSDFS_DBG(layout->env.show_debug,
		  "stripes_per_portion %u, peb_desc_per_stripe %u, "
		  "pebs_per_portion %llu, start_peb %llu, "
		  "pebs_per_volume %llu, pebs_count %llu\n",
		  stripes_per_portion, peb_desc_per_stripe,
		  pebs_per_portion, start_peb, pebs_per_volume,
		  pebs_count);

	bytes_count = hdr_size;
	bytes_count += pebs_count * sizeof(struct ssdfs_peb_descriptor);

	hdr = (struct ssdfs_peb_table_fragment_header *)ptr;

	hdr->magic = cpu_to_le16(SSDFS_PEB_TABLE_MAGIC);
	hdr->flags = 0;

	hdr->recover_months = SSDFS_PEB_RECOVER_MONTHS_DEFAULT;
	hdr->recover_threshold = SSDFS_PEBTBL_FIRST_RECOVER_TRY;

	hdr->start_peb = cpu_to_le64(start_peb);
	BUG_ON(pebs_count >= U16_MAX);
	hdr->pebs_count = cpu_to_le16(pebs_count);

	reserved_pebs = (pebs_count * reserved_pebs_pct) / 100;
	BUG_ON(reserved_pebs >= U16_MAX);
	hdr->last_selected_peb = cpu_to_le16(0);
	hdr->reserved_pebs = cpu_to_le16((u16)reserved_pebs);

	hdr->stripe_id = cpu_to_le16(stripe_index);
	hdr->portion_id = cpu_to_le16(portion_index);
	hdr->fragment_id = cpu_to_le16(stripe_index);

	hdr->bytes_count = cpu_to_le32(bytes_count);
}

static
int maptbl_prepare_portion(struct ssdfs_volume_layout *layout,
			    u16 index)
{
	u8 *ptr;
	u32 portions = layout->maptbl.portions_count;
	u16 portions_per_fragment = layout->maptbl.portions_per_fragment;
	size_t portion_size = layout->maptbl.portion_size;
	u16 stripes_per_portion = layout->maptbl.stripes_per_portion;
	u16 lebtbl_mempages;
	u32 lebtbl_portion_bytes = layout->maptbl.lebtbl_portion_bytes;
	u16 i;

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, index %u\n", layout, index);

	if (index >= portions) {
		SSDFS_ERR("invalid index: index %u >= portions %u\n",
			  index, portions);
		return -EINVAL;
	}

	ptr = (u8 *)layout->maptbl.fragments_array[index / portions_per_fragment];
	ptr += (index % portions_per_fragment) * portion_size;

	lebtbl_mempages = (u16)(lebtbl_portion_bytes / layout->page_size);
	BUG_ON(lebtbl_mempages == 0);

	for (i = 0; i < lebtbl_mempages; i++) {
		u8 *lebtbl_ptr;

		lebtbl_ptr = ptr + (i * layout->page_size);
		maptbl_prepare_leb_table(layout, lebtbl_ptr, index, i);
	}

	for (i = 0; i < stripes_per_portion; i++) {
		u8 *pebtbl_ptr;

		pebtbl_ptr = ptr + lebtbl_portion_bytes;
		pebtbl_ptr += (i * layout->page_size);
		maptbl_prepare_peb_table(layout, pebtbl_ptr, index, i);
	}

	return 0;
}

int maptbl_mkfs_prepare(struct ssdfs_volume_layout *layout)
{
	u32 portions;
	u32 i;
	int err;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	err = reserve_segments(layout, SSDFS_PEB_MAPPING_TABLE);
	if (err) {
		SSDFS_ERR("fail to reserve segments: err %d\n", err);
		return err;
	}

	err = maptbl_create_fragments_array(layout);
	if (err) {
		SSDFS_ERR("fail to create fragments array: err %d\n",
			  err);
		return err;
	}

	portions = layout->maptbl.portions_count;
	BUG_ON(portions >= U16_MAX);

	for (i = 0; i < portions; i++) {
		err = maptbl_prepare_portion(layout, (u16)i);
		if (err) {
			SSDFS_ERR("fail to prepare portion: "
				  "index %u, err %d\n",
				  i, err);
			return err;
		}
	}

	return 0;
}

static
int check_portion_pebs_validity(struct ssdfs_volume_layout *layout,
				u8 *portion)
{
	int fd = layout->env.fd;
	struct ssdfs_peb_table_fragment_header *hdr;
	size_t hdr_size = sizeof(struct ssdfs_peb_table_fragment_header);
	struct ssdfs_peb_descriptor *desc;
	size_t desc_size = sizeof(struct ssdfs_peb_descriptor);
	u16 stripes = layout->maptbl.stripes_per_portion;
	u32 lebtbl_portion_bytes = layout->maptbl.lebtbl_portion_bytes;
	u8 *pebtbl = portion + lebtbl_portion_bytes;
	u8 *desc_array;
	u16 stripe_id;
	u64 peb_id;
	u16 pebs_count;
	u32 peb_size = layout->env.erase_size;
	u64 offset;
	u8 flags;
	u16 i;
	u8 *bmap;
	int res;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	for (stripe_id = 0; stripe_id < stripes; stripe_id++) {
		pebtbl += stripe_id * layout->page_size;
		hdr = (struct ssdfs_peb_table_fragment_header *)pebtbl;
		pebs_count = le16_to_cpu(hdr->pebs_count);
		peb_id = le64_to_cpu(hdr->start_peb);
		desc_array = pebtbl + hdr_size;

		for (i = 0; i < pebs_count; i++, peb_id++) {
			u8 *cur_ptr;

			offset = peb_id * peb_size;
			cur_ptr = desc_array + (peb_id * desc_size);
			desc = (struct ssdfs_peb_descriptor *)cur_ptr;

			res = layout->env.dev_ops->check_peb(fd, offset,
							peb_size,
							SSDFS_FALSE,
							layout->env.show_debug);
			if (res < 0) {
				SSDFS_ERR("fail to check PEB: "
					  "offset %llu, err %d\n",
					  offset, res);
				return res;
			}

			switch (res) {
			case SSDFS_PEB_ERASURE_OK:
				desc->erase_cycles = cpu_to_le32(1);
				break;

			case SSDFS_PEB_IS_BAD:
				desc->erase_cycles = cpu_to_le32(U32_MAX);
				desc->state =
				    cpu_to_le8(SSDFS_MAPTBL_BAD_PEB_STATE);

				flags = le8_to_cpu(hdr->flags);
				flags |= SSDFS_PEBTBL_BADBLK_EXIST;
				hdr->flags = cpu_to_le8(flags);

				bmap = &hdr->bmaps[SSDFS_PEBTBL_USED_BMAP][0];
				__set_bit(i, (unsigned long *)bmap);
				bmap = &hdr->bmaps[SSDFS_PEBTBL_BADBLK_BMAP][0];
				__set_bit(i, (unsigned long *)bmap);
				break;

			case SSDFS_RECOVERING_PEB:
				desc->erase_cycles = cpu_to_le32(1);
				desc->state =
				    cpu_to_le8(SSDFS_MAPTBL_RECOVERING_STATE);

				flags = le8_to_cpu(hdr->flags);
				flags |= SSDFS_PEBTBL_UNDER_RECOVERING;
				hdr->flags = cpu_to_le8(flags);

				bmap = &hdr->bmaps[SSDFS_PEBTBL_USED_BMAP][0];
				__set_bit(i, (unsigned long *)bmap);
				bmap = &hdr->bmaps[SSDFS_PEBTBL_RECOVER_BMAP][0];
				__set_bit(i, (unsigned long *)bmap);
				break;

			default:
				BUG();
			};
		}
	}

	return 0;
}

static
int check_pebs_validity(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_nand_geometry info = {
		.erasesize = layout->env.erase_size,
		.writesize = layout->page_size,
	};
	int fd = layout->env.fd;
	u32 maptbl_pebs;
	u16 portions_per_peb;
	size_t portion_size;
	u32 findex, pindex;
	int res;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	switch (layout->env.device_type) {
	case SSDFS_MTD_DEVICE:
		/* continue logic */
		break;

	case SSDFS_ZNS_DEVICE:
	case SSDFS_BLK_DEVICE:
		return 0;

	default:
		BUG();
	};

	res = layout->env.dev_ops->check_nand_geometry(fd, &info,
						       layout->env.show_debug);
	if (res)
		return res;

	maptbl_pebs = layout->maptbl.maptbl_pebs;
	portions_per_peb = layout->maptbl.portions_per_fragment;
	portion_size = layout->maptbl.portion_size;

	for (findex = 0; findex < maptbl_pebs; findex++) {
		u8 *peb_buf = layout->maptbl.fragments_array[findex];

		for (pindex = 0; pindex < portions_per_peb; pindex++) {
			u8 *portion = peb_buf + (pindex * portion_size);

			res = check_portion_pebs_validity(layout, portion);
			if (res) {
				SSDFS_ERR("fail to check portion: "
					  "fragment_index %u, "
					  "portion_index %u, "
					  "err %d\n",
					  findex, pindex, res);
				return res;
			}
		}
	}

	layout->is_volume_erased = SSDFS_TRUE;

	return 0;
}

static
u8 *get_lebtbl_fragment(struct ssdfs_volume_layout *layout,
			u64 leb_id,
			u32 *fragment_index,
			u32 *portion_index,
			u16 *leb_desc_index)
{
	u32 leb_desc_per_mempage;
	u32 leb_desc_per_portion;
	u32 leb_desc_per_peb;
	u32 lebtbls_per_portion;
	u64 diff_leb_id;
	u32 mempage_index;
	u8 *fragment;
	u8 *portion;
	u8 *lebtbl;

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, leb_id %llu\n", layout, leb_id);

	*fragment_index = U32_MAX;
	*portion_index = U32_MAX;

	leb_desc_per_portion = layout->maptbl.lebs_per_portion;
	leb_desc_per_peb =
		leb_desc_per_portion * layout->maptbl.portions_per_fragment;

	*fragment_index = leb_id / leb_desc_per_peb;
	BUG_ON(*fragment_index >= layout->maptbl.maptbl_pebs);

	diff_leb_id = leb_id - (*fragment_index * leb_desc_per_peb);
	*portion_index = diff_leb_id / leb_desc_per_portion;
	BUG_ON(*portion_index >= layout->maptbl.portions_per_fragment);

	diff_leb_id = diff_leb_id - (*portion_index * leb_desc_per_portion);
	leb_desc_per_mempage = SSDFS_LEB_DESC_PER_FRAGMENT(PAGE_CACHE_SIZE);
	mempage_index = diff_leb_id / leb_desc_per_mempage;
	lebtbls_per_portion =
		layout->maptbl.lebtbl_portion_bytes / layout->page_size;
	BUG_ON(mempage_index >= lebtbls_per_portion);

	*leb_desc_index = diff_leb_id % leb_desc_per_mempage;

	fragment = (u8 *)layout->maptbl.fragments_array[*fragment_index];
	portion = fragment + (*portion_index * layout->maptbl.portion_size);
	lebtbl = portion + (mempage_index * layout->page_size);

	return lebtbl;
}

static
u8 *get_pebtbl_fragment(struct ssdfs_volume_layout *layout,
			u32 fragment_index,
			u32 portion_index,
			u16 stripe_index)
{
	u8 *fragment;
	u8 *portion;
	u8 *pebtbl;

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, fragment_index %u, "
		  "portion_index %u, stripe_index %u\n",
		  layout, fragment_index,
		  portion_index, stripe_index);

	fragment = (u8 *)layout->maptbl.fragments_array[fragment_index];
	portion = fragment + (portion_index * layout->maptbl.portion_size);

	pebtbl = portion + layout->maptbl.lebtbl_portion_bytes;
	pebtbl += stripe_index * layout->page_size;

	return pebtbl;
}

static
u16 find_unused_peb(struct ssdfs_peb_table_fragment_header *hdr)
{
	u16 pebs_count = le16_to_cpu(hdr->pebs_count);
	u16 bmap_ulongs = (pebs_count + BITS_PER_LONG - 1) / BITS_PER_LONG;
	unsigned long *bmap;
	u16 index;
	u16 peb_index;

	bmap = (unsigned long *)&hdr->bmaps[SSDFS_PEBTBL_USED_BMAP][0];

	index = 0;
	for (; index < bmap_ulongs; index++) {
		if (bmap[index] != ULONG_MAX)
			break;
	}

	peb_index = index * BITS_PER_LONG;
	for (; peb_index < pebs_count; peb_index++) {
		if (!test_bit(peb_index, bmap))
			break;
	}

	if (peb_index >= pebs_count) {
		SSDFS_ERR("fail to find unused peb\n");
		return U16_MAX;
	}

	return peb_index;
}

static
void define_peb_as_used(u8 *pebtbl, u16 peb_index, int meta_index)
{
	struct ssdfs_peb_table_fragment_header *hdr;
	size_t hdr_size = sizeof(struct ssdfs_peb_table_fragment_header);
	struct ssdfs_peb_descriptor *desc_array, *desc;
	size_t desc_size = sizeof(struct ssdfs_peb_descriptor);
	u16 pebs_count;
	u16 last_selected_peb;
	int peb_type = SEG2PEB_TYPE(META2SEG_TYPE(meta_index));
	unsigned long *bmap;
	u32 bytes_count;

	BUG_ON(meta_index > SSDFS_METADATA_ITEMS_MAX);
	BUG_ON(peb_type <= SSDFS_MAPTBL_UNKNOWN_PEB_TYPE ||
		peb_type >= SSDFS_MAPTBL_PEB_TYPE_MAX);

	hdr = (struct ssdfs_peb_table_fragment_header *)pebtbl;
	BUG_ON(hdr->magic != cpu_to_le16(SSDFS_PEB_TABLE_MAGIC));
	pebs_count = le16_to_cpu(hdr->pebs_count);
	last_selected_peb = le16_to_cpu(hdr->last_selected_peb);
	BUG_ON(last_selected_peb >= pebs_count);
	BUG_ON(peb_index >= pebs_count);
	bytes_count = le32_to_cpu(hdr->bytes_count);

	BUG_ON(bytes_count != (hdr_size + (pebs_count * desc_size)));

	desc_array = (struct ssdfs_peb_descriptor *)(pebtbl + hdr_size);
	desc = &desc_array[peb_index];

	BUG_ON(le8_to_cpu(desc->state) != SSDFS_MAPTBL_UNKNOWN_PEB_STATE);
	BUG_ON(le8_to_cpu(desc->type) != SSDFS_MAPTBL_UNKNOWN_PEB_TYPE);

	desc->type = cpu_to_le8((u8)peb_type);
	desc->state = cpu_to_le8(SSDFS_MAPTBL_USING_PEB_STATE);

	hdr->last_selected_peb = cpu_to_le16(last_selected_peb);

	bmap = (unsigned long *)&hdr->bmaps[SSDFS_PEBTBL_USED_BMAP][0];
	__set_bit(peb_index, bmap);
}

static
void define_peb_as_pre_erased(u8 *pebtbl, u16 peb_index)
{
	struct ssdfs_peb_table_fragment_header *hdr;
	size_t hdr_size = sizeof(struct ssdfs_peb_table_fragment_header);
	struct ssdfs_peb_descriptor *desc_array, *desc;
	size_t desc_size = sizeof(struct ssdfs_peb_descriptor);
	unsigned long *bmap;
	u16 pebs_count;
	u16 last_selected_peb;
	u32 bytes_count;

	hdr = (struct ssdfs_peb_table_fragment_header *)pebtbl;
	BUG_ON(hdr->magic != cpu_to_le16(SSDFS_PEB_TABLE_MAGIC));
	pebs_count = le16_to_cpu(hdr->pebs_count);
	last_selected_peb = le16_to_cpu(hdr->last_selected_peb);
	BUG_ON(last_selected_peb >= pebs_count);
	BUG_ON(peb_index >= pebs_count);
	bytes_count = le32_to_cpu(hdr->bytes_count);

	BUG_ON(bytes_count != (hdr_size + (pebs_count * desc_size)));

	desc_array = (struct ssdfs_peb_descriptor *)(pebtbl + hdr_size);
	desc = &desc_array[peb_index];

	BUG_ON(le8_to_cpu(desc->state) != SSDFS_MAPTBL_UNKNOWN_PEB_STATE);
	BUG_ON(le8_to_cpu(desc->type) != SSDFS_MAPTBL_UNKNOWN_PEB_TYPE);

	desc->type = cpu_to_le8(SSDFS_MAPTBL_UNKNOWN_PEB_TYPE);
	desc->state = cpu_to_le8(SSDFS_MAPTBL_PRE_ERASE_STATE);

	bmap = (unsigned long *)&hdr->bmaps[SSDFS_PEBTBL_DIRTY_BMAP][0];
	__set_bit(peb_index, bmap);
}

static inline
u16 DEFINE_PEB_INDEX_IN_PORTION(u16 stripe_index,
				u16 item_index)
{
	u32 peb_index;

	peb_index = SSDFS_PEB_DESC_PER_FRAGMENT(PAGE_CACHE_SIZE);
	peb_index *= stripe_index;
	peb_index += item_index;

	BUG_ON(peb_index >= U16_MAX);

	return (u16)peb_index;
}

static
void define_leb_as_mapped(u8 *lebtbl, u16 leb_desc_index, u16 physical_index)
{
	struct ssdfs_leb_table_fragment_header *hdr;
	size_t hdr_size = sizeof(struct ssdfs_leb_table_fragment_header);
	struct ssdfs_leb_descriptor *desc_array, *desc;
	size_t desc_size = sizeof(struct ssdfs_leb_descriptor);
	u16 lebs_count;
	u16 mapped_lebs, migrating_lebs;
	u32 bytes_count;

	hdr = (struct ssdfs_leb_table_fragment_header *)lebtbl;
	BUG_ON(hdr->magic != cpu_to_le16(SSDFS_LEB_TABLE_MAGIC));

	lebs_count = le16_to_cpu(hdr->lebs_count);
	BUG_ON(lebs_count == 0);
	mapped_lebs = le16_to_cpu(hdr->mapped_lebs);
	BUG_ON(mapped_lebs > lebs_count);
	migrating_lebs = le16_to_cpu(hdr->migrating_lebs);
	BUG_ON(migrating_lebs > lebs_count);
	BUG_ON((mapped_lebs + migrating_lebs) > lebs_count);
	bytes_count = le32_to_cpu(hdr->bytes_count);
	BUG_ON(bytes_count != (hdr_size + (lebs_count * desc_size)));

	desc_array = (struct ssdfs_leb_descriptor *)(lebtbl + hdr_size);
	desc = &desc_array[leb_desc_index];

	desc->physical_index = cpu_to_le16(physical_index);
	desc->relation_index = cpu_to_le16(U16_MAX);

	mapped_lebs++;
	hdr->mapped_lebs = cpu_to_le16(mapped_lebs);
}

static inline
u16 get_stripe_index(struct ssdfs_volume_layout *layout,
		     struct ssdfs_leb_table_fragment_header *lebtbl_hdr,
		     u64 leb_id)
{
	u16 stripes_per_portion = layout->maptbl.stripes_per_portion;
	u32 pebs_per_seg = (u32)(layout->seg_size / layout->env.erase_size);
	u64 leb_index = leb_id % pebs_per_seg;
	u16 peb_desc_per_stripe;
	u64 start_leb;
	u16 stripe_index;

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, leb_id %llu\n",
		  layout, leb_id);

	start_leb = le64_to_cpu(lebtbl_hdr->start_leb);
	peb_desc_per_stripe = SSDFS_PEB_DESC_PER_FRAGMENT(PAGE_CACHE_SIZE);

	if (stripes_per_portion == 1)
		stripe_index = (leb_id - start_leb) / peb_desc_per_stripe;
	else if (pebs_per_seg > stripes_per_portion) {
		u32 leb_index_per_stripe;

		leb_index_per_stripe = pebs_per_seg + stripes_per_portion - 1;
		leb_index_per_stripe /= stripes_per_portion;

		BUG_ON((leb_index / leb_index_per_stripe) >= U16_MAX);
		stripe_index = (u16)(leb_index / leb_index_per_stripe);
	} else {
		BUG_ON(pebs_per_seg >= U16_MAX);
		stripe_index = (u16)(leb_index / pebs_per_seg);
	}

	return stripe_index;
}

static
u64 map_leb2peb(struct ssdfs_volume_layout *layout,
		u64 leb_id, int meta_index)
{
	struct ssdfs_leb_table_fragment_header *lebtbl_hdr;
	u8 *lebtbl;
	struct ssdfs_peb_table_fragment_header *pebtbl_hdr;
	u8 *pebtbl;
	u32 fragment_index;
	u32 portion_index;
	u64 start_leb, start_peb;
	u16 lebs_count;
	u16 stripe_index;
	u16 peb_index;
	u16 leb_desc_index;
	u16 physical_index;

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, leb_id %llu, meta_index %#x\n",
		  layout, leb_id, meta_index);

	lebtbl = get_lebtbl_fragment(layout, leb_id,
				     &fragment_index,
				     &portion_index,
				     &leb_desc_index);
	lebtbl_hdr = (struct ssdfs_leb_table_fragment_header *)lebtbl;

	BUG_ON(lebtbl_hdr->magic != cpu_to_le16(SSDFS_LEB_TABLE_MAGIC));

	start_leb = le64_to_cpu(lebtbl_hdr->start_leb);
	lebs_count = le16_to_cpu(lebtbl_hdr->lebs_count);

	BUG_ON(leb_id < start_leb);
	BUG_ON(leb_id >= (start_leb + lebs_count));

	stripe_index = get_stripe_index(layout, lebtbl_hdr, leb_id);

	pebtbl = get_pebtbl_fragment(layout, fragment_index,
				     portion_index, stripe_index);
	pebtbl_hdr = (struct ssdfs_peb_table_fragment_header *)pebtbl;

	BUG_ON(pebtbl_hdr->magic != cpu_to_le16(SSDFS_PEB_TABLE_MAGIC));

	peb_index = find_unused_peb(pebtbl_hdr);
	if (peb_index == U16_MAX) {
		SSDFS_ERR("fail to find unused PEB: "
			  "leb_id %llu, fragment_index %u, "
			  "portion_index %u, stripe_index %u\n",
			  leb_id, fragment_index,
			  portion_index, stripe_index);
		return U64_MAX;
	}

	define_peb_as_used(pebtbl, peb_index, meta_index);

	physical_index = DEFINE_PEB_INDEX_IN_PORTION(stripe_index, peb_index);
	define_leb_as_mapped(lebtbl, leb_desc_index, physical_index);

	start_peb = le64_to_cpu(pebtbl_hdr->start_peb);

	SSDFS_DBG(layout->env.show_debug,
		  "peb_index %u, physical_index %u, start_peb %llu\n",
		  peb_index, physical_index, start_peb);

	return start_peb + peb_index;
}

static
int map_segment_lebs2pebs(struct ssdfs_volume_layout *layout,
			  struct ssdfs_segment_desc *segment)
{
	struct ssdfs_peb_content *peb;
	u64 leb_id, peb_id;
	int i;
	int err;

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, pebs_count %u, pebs_capacity %u\n",
		  layout, segment->pebs_count, segment->pebs_capacity);

	for (i = 0; i < segment->pebs_capacity; i++) {
		peb = &segment->pebs[i];
		leb_id = peb->leb_id;

		if (leb_id == U64_MAX) {
			SSDFS_DBG(layout->env.show_debug,
				  "leb_id %llu\n", leb_id);
			continue;
		}

		peb_id = map_leb2peb(layout, leb_id, segment->seg_type);
		if (peb_id == U64_MAX) {
			SSDFS_ERR("fail to map LEB to PEB: "
				  "leb_id %llu\n",
				  leb_id);
			return -ERANGE;
		}

		err = cache_leb2peb_pair(layout, leb_id, peb_id);
		if (err) {
			SSDFS_ERR("fail to cache leb2peb pair: "
				  "leb_id %llu, peb_id %llu, err %d\n",
				  leb_id, peb_id, err);
			return err;
		}

		SSDFS_DBG(layout->env.show_debug,
			  "peb_index %d, leb_id %llu, peb_id %llu\n",
			  i, leb_id, peb_id);

		peb->peb_id = peb_id;
	}

	return 0;
}

static
int map_allocated_lebs2pebs(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_segment_desc *segment;
	int seg_index;
	int segs_count;
	int i, j;
	int err;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	for (i = 0; i < SSDFS_METADATA_ITEMS_MAX; i++) {
		seg_index = layout->meta_array[i].start_seg_index;
		segs_count = layout->meta_array[i].segs_count;

		SSDFS_DBG(layout->env.show_debug,
			  "seg_index %d, segs_count %d\n",
			  seg_index, segs_count);

		for (j = 0; j < segs_count; j++, seg_index++) {
			segment = &layout->segs[seg_index];

			SSDFS_DBG(layout->env.show_debug,
				  "cur_index %d, segs_count %d\n",
				  j, segs_count);

			err = map_segment_lebs2pebs(layout, segment);
			if (err) {
				SSDFS_ERR("fail to map segment's LEBs: "
					  "seg_index %d, err %d\n",
					  seg_index, err);
				return err;
			}
		}
	}

	SSDFS_DBG(layout->env.show_debug, "finished\n");

	return 0;
}

static
int mark_unallocated_pebs_as_pre_erased(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_segment_desc *segment;
	struct ssdfs_leb_table_fragment_header *lebtbl_hdr;
	u8 *lebtbl;
	struct ssdfs_peb_table_fragment_header *pebtbl_hdr;
	u8 *pebtbl;
	u16 stripes_per_portion = layout->maptbl.stripes_per_portion;
	u32 fragment_index;
	u32 portion_index;
	u16 leb_desc_index;
	u64 leb_id;
	u64 peb_id;
	u64 total_pebs_count;
	u64 start_leb, start_peb;
	u16 lebs_count, pebs_count;
	u64 unallocated_pebs;
	u16 stripe_index;
	int i, j;

	if (layout->need_erase_device) {
		SSDFS_DBG(layout->env.show_debug,
			  "do nothing: volume will be erased by mkfs\n");
		return 0;
	}

	total_pebs_count = layout->env.fs_size / layout->env.erase_size;

	segment = &layout->segs[layout->segs_capacity - 1];

	leb_id = segment->pebs[segment->pebs_capacity - 1].leb_id;
	BUG_ON(leb_id >= total_pebs_count);
	leb_id++;

	peb_id = segment->pebs[segment->pebs_capacity - 1].peb_id;
	BUG_ON(peb_id >= total_pebs_count);
	peb_id++;

	SSDFS_DBG(layout->env.show_debug,
		  "leb_id %llu, peb_id %llu, total_pebs_count %llu\n",
		  leb_id, peb_id, total_pebs_count);

	unallocated_pebs = total_pebs_count - leb_id;

	while (leb_id < total_pebs_count) {
		lebtbl = get_lebtbl_fragment(layout, leb_id,
					     &fragment_index,
					     &portion_index,
					     &leb_desc_index);
		lebtbl_hdr = (struct ssdfs_leb_table_fragment_header *)lebtbl;

		BUG_ON(lebtbl_hdr->magic != cpu_to_le16(SSDFS_LEB_TABLE_MAGIC));

		start_leb = le64_to_cpu(lebtbl_hdr->start_leb);
		lebs_count = le16_to_cpu(lebtbl_hdr->lebs_count);

		SSDFS_DBG(layout->env.show_debug,
			  "start_leb %llu, lebs_count %u\n",
			  start_leb, lebs_count);

		BUG_ON(leb_id < start_leb);
		BUG_ON(leb_id >= (start_leb + lebs_count));

		stripe_index = get_stripe_index(layout, lebtbl_hdr, leb_id);

		for (i = stripe_index; i < stripes_per_portion; i++) {
			u64 peb_index;

			pebtbl = get_pebtbl_fragment(layout, fragment_index,
						     portion_index, i);
			pebtbl_hdr =
			    (struct ssdfs_peb_table_fragment_header *)pebtbl;

			BUG_ON(pebtbl_hdr->magic !=
					cpu_to_le16(SSDFS_PEB_TABLE_MAGIC));

			start_peb = le64_to_cpu(pebtbl_hdr->start_peb);
			pebs_count = le64_to_cpu(pebtbl_hdr->pebs_count);

			SSDFS_DBG(layout->env.show_debug,
				  "start_peb %llu, pebs_count %u, peb_id %llu\n",
				  start_peb, pebs_count, peb_id);

			BUG_ON(peb_id < start_peb);
			BUG_ON(peb_id >= (start_peb + pebs_count));

			peb_index = peb_id - start_peb;
			BUG_ON(peb_index >= U16_MAX || peb_index >= pebs_count);

			for (j = peb_index; j < pebs_count; j++) {
				define_peb_as_pre_erased(pebtbl, j);
			}

			peb_id = start_peb + pebs_count;
		}

		leb_id = start_leb + lebs_count;
	}

	layout->maptbl.pre_erased_pebs = unallocated_pebs;

	return 0;
}

static
void define_maptbl_extents(struct ssdfs_volume_layout *layout,
			   int seg_chain_type)
{
	struct ssdfs_maptbl_sb_header *hdr = &layout->sb.vh.maptbl;
	struct ssdfs_meta_area_extent *extent;
	u32 pebs_per_seg = (u32)(layout->seg_size / layout->env.erase_size);
	u32 portions_per_seg =
		pebs_per_seg * layout->maptbl.portions_per_fragment;
	u32 segs_per_copy = (layout->maptbl.portions_count +
				portions_per_seg - 1) / portions_per_seg;
	int seg_index;
	u64 seg_id = U64_MAX;
	u32 extent_len;
	int i, j;

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, pebs_per_seg %u, portions_per_seg %u, "
		  "portions_count %u, segs_per_copy %u\n",
		  layout, pebs_per_seg, portions_per_seg,
		  layout->maptbl.portions_count,
		  segs_per_copy);

	BUG_ON(seg_chain_type < SSDFS_MAIN_MAPTBL_SEG ||
		seg_chain_type >= SSDFS_MAPTBL_SEG_COPY_MAX);

	seg_index = layout->meta_array[SSDFS_PEB_MAPPING_TABLE].start_seg_index;

	switch (seg_chain_type) {
	case SSDFS_MAIN_MAPTBL_SEG:
		/* do nothing */
		break;

	case SSDFS_COPY_MAPTBL_SEG:
		seg_index += segs_per_copy;
		break;

	default:
		BUG();
	};

	for (i = 0; i < SSDFS_MAPTBL_RESERVED_EXTENTS; i++) {
		extent = &hdr->extents[i][seg_chain_type];
		extent->type = cpu_to_le16(SSDFS_EMPTY_EXTENT_TYPE);
	}

	for (i = 0, j = 0, extent_len = 0; i < segs_per_copy; i++) {
		u64 cur_seg_id = layout->segs[seg_index + i].seg_id;

		if (seg_id == U64_MAX) {
			seg_id = cur_seg_id;
			extent_len++;
		} else if ((seg_id + extent_len) != cur_seg_id) {
			extent = &hdr->extents[j][seg_chain_type];

			extent->start_id = cpu_to_le64(seg_id);
			extent->len = cpu_to_le32(extent_len);
			extent->type = cpu_to_le16(SSDFS_SEG_EXTENT_TYPE);
			extent->flags = 0;

			SSDFS_DBG(layout->env.show_debug,
				  "extent (index %d, seg_id %llu, "
				  "extent_len %u)\n",
				  j, seg_id, extent_len);

			seg_id = cur_seg_id;
			extent_len = 1;
			j++;
			BUG_ON(j >= SSDFS_MAPTBL_RESERVED_EXTENTS);
		} else
			extent_len++;
	}

	if (extent_len != 0) {
		BUG_ON(seg_id == U64_MAX);
		BUG_ON(j >= SSDFS_MAPTBL_RESERVED_EXTENTS);

		extent = &hdr->extents[j][seg_chain_type];
		extent->start_id = cpu_to_le64(seg_id);
		extent->len = cpu_to_le32(extent_len);
		extent->type = cpu_to_le16(SSDFS_SEG_EXTENT_TYPE);
		extent->flags = 0;

		SSDFS_DBG(layout->env.show_debug,
			  "extent (index %d, seg_id %llu, "
			  "extent_len %u)\n",
			  j, seg_id, extent_len);
	}
}

static
int init_maptbl_sb_header(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_maptbl_sb_header *hdr = &layout->sb.vh.maptbl;
	struct ssdfs_maptbl_layout *maptbl = &layout->maptbl;
	u64 pebs_count = layout->env.fs_size / layout->env.erase_size;
	u32 pebs_per_seg = (u32)(layout->seg_size / layout->env.erase_size);
	u32 portions_per_seg = pebs_per_seg * maptbl->portions_per_fragment;
	u16 flags = 0;
	u16 lebs_per_portion;
	u16 pebs_per_portion;
	u16 pebs_per_stripe;
	u32 segs_per_copy;
	size_t extent_size = sizeof(struct ssdfs_meta_area_extent);
	int segs_count;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	segs_per_copy = (maptbl->portions_count + portions_per_seg - 1) /
			portions_per_seg;

	hdr->fragments_count = cpu_to_le32(maptbl->portions_count);
	hdr->fragment_bytes = cpu_to_le32(maptbl->portion_size);

	hdr->last_peb_recover_cno = cpu_to_le64(U64_MAX);

	hdr->lebs_count = cpu_to_le64(pebs_count);
	hdr->pebs_count = cpu_to_le64(pebs_count);

	BUG_ON(portions_per_seg >= U16_MAX);
	hdr->fragments_per_seg = cpu_to_le16((u16)portions_per_seg);
	hdr->fragments_per_peb = cpu_to_le16(maptbl->portions_per_fragment);

	if (maptbl->has_backup_copy)
		flags |= SSDFS_MAPTBL_HAS_COPY;

	switch (maptbl->compression) {
	case SSDFS_UNCOMPRESSED_BLOB:
		/* do nothing */
		break;

	case SSDFS_ZLIB_BLOB:
		flags |= SSDFS_MAPTBL_MAKE_ZLIB_COMPR;
		break;

	case SSDFS_LZO_BLOB:
		flags |= SSDFS_MAPTBL_MAKE_LZO_COMPR;
		break;

	default:
		SSDFS_ERR("invalid compression type %#x\n",
			  maptbl->compression);
		return -ERANGE;
	}

	hdr->flags = cpu_to_le16(flags);

	if (layout->maptbl.pre_erased_pebs >= U16_MAX) {
		hdr->pre_erase_pebs = cpu_to_le16(U16_MAX);
	} else {
		hdr->pre_erase_pebs =
			cpu_to_le16((u16)layout->maptbl.pre_erased_pebs);
	}

	lebs_per_portion = min_t(u16, maptbl->lebs_per_portion, pebs_count);
	hdr->lebs_per_fragment = cpu_to_le16(lebs_per_portion);
	pebs_per_portion = min_t(u16, maptbl->pebs_per_portion, pebs_count);
	hdr->pebs_per_fragment = cpu_to_le16(pebs_per_portion);

	pebs_per_stripe = pebs_per_portion / maptbl->stripes_per_portion;
	if (pebs_per_portion % maptbl->stripes_per_portion)
		pebs_per_stripe++;

	hdr->pebs_per_stripe = cpu_to_le16(pebs_per_stripe);
	hdr->stripes_per_fragment = cpu_to_le16(maptbl->stripes_per_portion);

	segs_count = layout->meta_array[SSDFS_PEB_MAPPING_TABLE].segs_count;

	memset(hdr->extents, 0xFF,
	       extent_size * SSDFS_MAPTBL_RESERVED_EXTENTS *
	       SSDFS_MAPTBL_SEG_COPY_MAX);

	if (maptbl->has_backup_copy && ((segs_per_copy * 2) != segs_count)) {
		SSDFS_ERR("invalid maptbl segment allocation: "
			  "segs_per_copy %u, segs_count %u\n",
			  segs_per_copy, segs_count);
		return -ERANGE;
	} else if (segs_per_copy != segs_count) {
		SSDFS_ERR("invalid maptbl segment allocation: "
			  "segs_per_copy %u, segs_count %u\n",
			  segs_per_copy, segs_count);
		return -ERANGE;
	}

	define_maptbl_extents(layout, SSDFS_MAIN_MAPTBL_SEG);

	if (maptbl->has_backup_copy)
		define_maptbl_extents(layout, SSDFS_COPY_MAPTBL_SEG);

	return 0;
}

static int init_sb_segs(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_volume_header *vh = &layout->sb.vh;
	struct ssdfs_metadata_desc *desc;
	int seg_index;
	int i, j;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	desc = &layout->meta_array[SSDFS_SUPERBLOCK];
	seg_index = desc->start_seg_index;

	for (i = 0; i < SSDFS_SB_CHAIN_MAX; i++) {
		for (j = 0; j < SSDFS_SB_SEG_COPY_MAX; j++) {
			int type = layout->segs[seg_index].seg_type;
			u64 leb_id = layout->segs[seg_index].pebs[0].leb_id;
			u64 peb_id = layout->segs[seg_index].pebs[0].peb_id;

			if (i == SSDFS_PREV_SB_SEG) {
				vh->sb_pebs[i][j].leb_id = cpu_to_le64(U64_MAX);
				vh->sb_pebs[i][j].peb_id = cpu_to_le64(U64_MAX);
			} else {
				if (type != SSDFS_SUPERBLOCK) {
					SSDFS_ERR("invalid seg_type %#x\n",
						  type);
					return -ERANGE;
				}

				vh->sb_pebs[i][j].leb_id = cpu_to_le64(leb_id);
				vh->sb_pebs[i][j].peb_id = cpu_to_le64(peb_id);
				seg_index++;
			}
		}
	}

	return 0;
}

static
void set_maptbl_presence_flag(struct ssdfs_volume_layout *layout)
{
	u64 feature_compat = le64_to_cpu(layout->sb.vs.feature_compat);
	feature_compat |= SSDFS_HAS_MAPTBL_COMPAT_FLAG;
	layout->sb.vs.feature_compat = cpu_to_le64(feature_compat);
}

int maptbl_mkfs_validate(struct ssdfs_volume_layout *layout)
{
	u64 seg_size = layout->seg_size;
	u32 erase_size = layout->env.erase_size;
	u32 pebs_per_seg = (u32)(seg_size / erase_size);
	int err;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	err = check_pebs_validity(layout);
	if (err) {
		SSDFS_ERR("fail to check PEBs validity: err %d\n",
			  err);
		return err;
	}

	err = maptbl_cache_mkfs_prepare(layout);
	if (err) {
		SSDFS_ERR("fail to prepare maptbl cache: "
			  "err %d\n",
			  err);
		return err;
	}

	err = map_allocated_lebs2pebs(layout);
	if (err) {
		SSDFS_ERR("fail to map LEBs to PEBs: err %d\n",
			  err);
		return err;
	}

	err = mark_unallocated_pebs_as_pre_erased(layout);
	if (err) {
		SSDFS_ERR("fail to mark unallocated PEBs as pre-erased: err %d\n",
			  err);
		return err;
	}

	err = init_maptbl_sb_header(layout);
	if (err) {
		SSDFS_ERR("fail to initialize maptbl_sb_header: err %d\n",
			  err);
		return err;
	}

	err = init_sb_segs(layout);
	if (err) {
		SSDFS_ERR("fail to initialize sb_segs: err %d\n",
			  err);
		return err;
	}

	if (layout->maptbl.migration_threshold >= U16_MAX) {
		layout->maptbl.migration_threshold =
					layout->migration_threshold;
	} else if (layout->maptbl.migration_threshold > pebs_per_seg) {
		SSDFS_WARN("user data migration threshold %u "
			   "was corrected to %u\n",
			   layout->maptbl.migration_threshold,
			   pebs_per_seg);
		layout->maptbl.migration_threshold = pebs_per_seg;
	}

	set_maptbl_presence_flag(layout);
	return 0;
}

static void maptbl_set_log_pages(struct ssdfs_volume_layout *layout,
				 u32 blks)
{
	u32 erasesize;
	u32 pagesize;
	u32 pages_per_peb;
	u32 log_pages_default;
	u32 log_pages = 0;

	SSDFS_DBG(layout->env.show_debug,
		  "log_pages %u, blks_count %u\n",
		  layout->maptbl.log_pages, blks);

	BUG_ON(blks == 0);
	BUG_ON(blks >= U16_MAX);

	erasesize = layout->env.erase_size;
	pagesize = layout->page_size;
	pages_per_peb = erasesize / pagesize;

	BUG_ON((blks / 2) > pages_per_peb);

	if (pages_per_peb % blks) {
		SSDFS_WARN("pages_per_peb %u, blks %u\n",
			   pages_per_peb, blks);
	}

	SSDFS_DBG(layout->env.show_debug,
		  "log_pages %u\n",
		  layout->maptbl.log_pages);

	blks = min_t(u32, blks, (u32)SSDFS_LOG_MAX_PAGES);

	if (layout->maptbl.log_pages == U16_MAX)
		log_pages = blks;
	else {
		log_pages = layout->maptbl.log_pages;

		if (log_pages < blks) {
			SSDFS_WARN("log_pages is corrected from %u to %u\n",
				   log_pages, blks);
			log_pages = blks;
		} else if (log_pages % blks) {
			SSDFS_WARN("log_pages %u, blks %u\n",
				   log_pages,
				   blks);
		}
	}

try_align_log_pages:
	SSDFS_DBG(layout->env.show_debug,
		  "TRY ALIGN LOG PAGES: "
		  "log_pages %u, blks_count %u\n",
		  log_pages, blks);

	while (layout->env.erase_size % (log_pages * layout->page_size))
		log_pages++;

	SSDFS_DBG(layout->env.show_debug,
		  "ALIGNED: log_pages %u\n",
		  log_pages);

	BUG_ON(log_pages > pages_per_peb);

	if (log_pages == pages_per_peb) {
		/*
		 * Stop align log_pages
		 */
	} else if ((log_pages - blks) < 3) {
		log_pages += 3;
		goto try_align_log_pages;
	}

	if (pages_per_peb % log_pages) {
		SSDFS_WARN("pages_per_peb %u, log_pages %u\n",
			   pages_per_peb, log_pages);
	}

	log_pages_default = pages_per_peb / SSDFS_LOGS_PER_PEB_DEFAULT;
	log_pages = max_t(u32, log_pages, log_pages_default);
	log_pages = min_t(u32, log_pages, (u32)SSDFS_LOG_MAX_PAGES);

	SSDFS_DBG(layout->env.show_debug,
		  "log_pages %u\n",
		  layout->maptbl.log_pages);

	layout->maptbl.log_pages = log_pages;
	BUG_ON(log_pages >= U16_MAX);
	layout->sb.vh.maptbl_log_pages = cpu_to_le16((u16)log_pages);

	SSDFS_DBG(layout->env.show_debug,
		  "log_pages %u\n",
		  layout->maptbl.log_pages);
}

int maptbl_mkfs_define_layout(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_metadata_desc *meta_desc;
	int seg_index;
	int segs_count;
	u32 maptbl_pebs;
	u32 pebs_per_seg;
	u32 page_size = layout->page_size;
	size_t portion_size = layout->maptbl.portion_size;
	u16 portions_per_fragment = layout->maptbl.portions_per_fragment;
	size_t peb_buffer_size = portion_size * portions_per_fragment;
	u32 portions_count = layout->maptbl.portions_count;
	int i, j;
	u32 valid_blks;
	int fragment_index = 0;
	u32 log_pages = 0;
	u32 payload_offset_in_bytes = 0;
	u32 start_logical_blk;
	u32 used_logical_blks = 0;
	u32 last_allocated_blk = U16_MAX;
	int err;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	meta_desc = &layout->meta_array[SSDFS_PEB_MAPPING_TABLE];
	segs_count = meta_desc->segs_count;
	maptbl_pebs = layout->maptbl.maptbl_pebs;
	pebs_per_seg = (u32)(layout->seg_size / layout->env.erase_size);

	if (segs_count <= 0 ||
	    segs_count > ((maptbl_pebs + pebs_per_seg - 1) / pebs_per_seg)) {
		SSDFS_ERR("invalid segs_count %d\n", segs_count);
		return -ERANGE;
	}

	if (meta_desc->start_seg_index >= layout->segs_capacity) {
		SSDFS_ERR("start_seg_index %d >= segs_capacity %d\n",
			  meta_desc->start_seg_index,
			  layout->segs_capacity);
		return -ERANGE;
	}

	if ((layout->segs_count + segs_count) > layout->segs_capacity) {
		SSDFS_ERR("not enough space for commit: "
			  "segs_count %d, request %d, capacity %d\n",
			  layout->segs_count, segs_count,
			  layout->segs_capacity);
		return -E2BIG;
	}

	seg_index = meta_desc->start_seg_index;
	valid_blks = (peb_buffer_size + page_size - 1) / page_size;

	for (i = 0; i < segs_count; i++) {
		start_logical_blk = 0;

		used_logical_blks = 0;
		last_allocated_blk = U16_MAX;

		for (j = 0; j < pebs_per_seg; j++) {
			used_logical_blks += valid_blks;

			if (used_logical_blks == 0) {
				SSDFS_ERR("invalid used_logical_blks %u\n",
					  used_logical_blks);
				return -ERANGE;
			}

			last_allocated_blk = used_logical_blks - 1;
		}

		for (j = 0; j < pebs_per_seg; j++) {
			struct ssdfs_peb_content *peb_desc;
			struct ssdfs_extent_desc *extent;
			u32 blks;
			u64 logical_byte_offset;

			if (fragment_index >= portions_count)
				break;

			logical_byte_offset =
				(u64)fragment_index * portion_size;

			layout->calculated_open_zones++;

			SSDFS_DBG(layout->env.show_debug,
				  "calculated_open_zones %u\n",
				  layout->calculated_open_zones);

			layout->segs[seg_index].pebs_count++;
			BUG_ON(layout->segs[seg_index].pebs_count >
				layout->segs[seg_index].pebs_capacity);
			peb_desc = &layout->segs[seg_index].pebs[j];

			err = set_extent_start_offset(layout,
							SSDFS_MAPTBL_SEG_TYPE,
							peb_desc,
							SSDFS_SEG_HEADER);
			if (err) {
				SSDFS_ERR("fail to define extent's offset: "
					  "err %d\n", err);
				return err;
			}

			err = define_segment_header_layout(layout, seg_index,
							   j);
			if (err) {
				SSDFS_ERR("fail to define seg header's layout: "
					  "err %d\n", err);
				return err;
			}

			err = set_extent_start_offset(layout,
							SSDFS_MAPTBL_SEG_TYPE,
							peb_desc,
							SSDFS_BLOCK_BITMAP);
			if (err) {
				SSDFS_ERR("fail to define extent's offset: "
					  "err %d\n", err);
				return err;
			}

			err = pre_commit_block_bitmap(layout, seg_index, j,
						      peb_buffer_size,
						      start_logical_blk,
						      valid_blks);
			if (err)
				return err;

			err = set_extent_start_offset(layout,
							SSDFS_MAPTBL_SEG_TYPE,
							peb_desc,
							SSDFS_OFFSET_TABLE);
			if (err) {
				SSDFS_ERR("fail to define extent's offset: "
					  "err %d\n", err);
				return err;
			}

			err = pre_commit_offset_table(layout, seg_index, j,
							logical_byte_offset,
							start_logical_blk,
							valid_blks,
							used_logical_blks,
							last_allocated_blk);
			if (err)
				return err;

			err = set_extent_start_offset(layout,
						      SSDFS_MAPTBL_SEG_TYPE,
						      peb_desc,
						      SSDFS_BLOCK_DESCRIPTORS);
			if (err) {
				SSDFS_ERR("fail to define extent's offset: "
					  "err %d\n", err);
				return err;
			}

			err = pre_commit_block_descriptors(layout, seg_index,
							j, start_logical_blk,
							valid_blks,
							SSDFS_MAPTBL_INO,
							payload_offset_in_bytes,
							layout->page_size);
			if (err)
				return err;

			err = set_extent_start_offset(layout,
							SSDFS_MAPTBL_SEG_TYPE,
							peb_desc,
							SSDFS_LOG_PAYLOAD);
			if (err) {
				SSDFS_ERR("fail to define extent's offset: "
					  "err %d\n", err);
				return err;
			}

			extent = &peb_desc->extents[SSDFS_LOG_PAYLOAD];

			BUG_ON(extent->buf);
			extent->buf =
				layout->maptbl.fragments_array[fragment_index];
			if (!extent->buf) {
				SSDFS_ERR("invalid fragment pointer: "
					  "buffer_index %d\n",
					  i + j);
				return -ERANGE;
			}
			layout->maptbl.fragments_array[fragment_index] = NULL;

			extent->bytes_count = peb_buffer_size;

			err = set_extent_start_offset(layout,
							SSDFS_MAPTBL_SEG_TYPE,
							peb_desc,
							SSDFS_LOG_FOOTER);
			if (err) {
				SSDFS_ERR("fail to define extent's offset: "
					  "err %d\n", err);
				return err;
			}

			err = define_log_footer_layout(layout, seg_index, j);
			if (err) {
				SSDFS_ERR("fail to define seg footer's layout: "
					  "err %d\n", err);
				return err;
			}

			if (layout->blkbmap.has_backup_copy) {
				err = set_extent_start_offset(layout,
						    SSDFS_MAPTBL_SEG_TYPE,
						    peb_desc,
						    SSDFS_BLOCK_BITMAP_BACKUP);
				if (err) {
					SSDFS_ERR("fail to define offset: "
						  "err %d\n", err);
					return err;
				}

				err = pre_commit_block_bitmap_backup(layout,
								seg_index,
								j,
								peb_buffer_size,
								start_logical_blk,
								valid_blks);
				if (err)
					return err;
			}

			if (layout->blk2off_tbl.has_backup_copy) {
				err = set_extent_start_offset(layout,
						    SSDFS_MAPTBL_SEG_TYPE,
						    peb_desc,
						    SSDFS_OFFSET_TABLE_BACKUP);
				if (err) {
					SSDFS_ERR("fail to define offset: "
						  "err %d\n", err);
					return err;
				}

				err = pre_commit_offset_table_backup(layout,
							seg_index, j,
							logical_byte_offset,
							start_logical_blk,
							valid_blks,
							used_logical_blks,
							last_allocated_blk);
				if (err)
					return err;
			}

			blks = calculate_log_pages(layout,
						   SSDFS_MAPTBL_SEG_TYPE,
						   peb_desc);
			log_pages = max_t(u32, blks, log_pages);

			fragment_index++;
			payload_offset_in_bytes += peb_buffer_size;
			start_logical_blk += valid_blks;
		}

		seg_index++;
	}

	maptbl_set_log_pages(layout, log_pages);
	return 0;
}

static inline
void calculate_lebtbl_fragment_checksum(u8 *ptr)
{
	struct ssdfs_leb_table_fragment_header *hdr;
	size_t hdr_size = sizeof(struct ssdfs_leb_table_fragment_header);
	u32 bytes_count;

	hdr = (struct ssdfs_leb_table_fragment_header *)ptr;

	BUG_ON(le16_to_cpu(hdr->magic) != SSDFS_LEB_TABLE_MAGIC);

	bytes_count = le32_to_cpu(hdr->bytes_count);

	BUG_ON(bytes_count < hdr_size);

	hdr->checksum = 0;
	hdr->checksum = ssdfs_crc32_le(ptr, bytes_count);
}

static inline
void calculate_pebtbl_fragment_checksum(u8 *ptr)
{
	struct ssdfs_peb_table_fragment_header *hdr;
	size_t hdr_size = sizeof(struct ssdfs_peb_table_fragment_header);
	u32 bytes_count;

	hdr = (struct ssdfs_peb_table_fragment_header *)ptr;

	BUG_ON(le16_to_cpu(hdr->magic) != SSDFS_PEB_TABLE_MAGIC);

	bytes_count = le32_to_cpu(hdr->bytes_count);

	BUG_ON(bytes_count < hdr_size);

	hdr->checksum = 0;
	hdr->checksum = ssdfs_crc32_le(ptr, bytes_count);
}

static
void calculate_peb_fragments_checksum(struct ssdfs_volume_layout *layout,
					void *fragments)
{
	u16 portions_per_fragment = layout->maptbl.portions_per_fragment;
	size_t portion_size = layout->maptbl.portion_size;
	u16 lebtbl_mempages;
	u32 lebtbl_portion_bytes = layout->maptbl.lebtbl_portion_bytes;
	u16 stripes_per_portion = layout->maptbl.stripes_per_portion;
	u8 *ptr;
	int i, j;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	lebtbl_mempages = (u16)(lebtbl_portion_bytes / layout->page_size);
	BUG_ON(lebtbl_mempages == 0);

	for (i = 0; i < portions_per_fragment; i++) {
		ptr = (u8 *)fragments + (i * portion_size);

		for (j = 0; j < lebtbl_mempages; j++) {
			u8 *lebtbl_ptr;

			lebtbl_ptr = ptr + (j * layout->page_size);
			calculate_lebtbl_fragment_checksum(lebtbl_ptr);
		}

		ptr += lebtbl_portion_bytes;

		for (j = 0; j < stripes_per_portion; j++) {
			u8 *pebtbl_ptr;

			pebtbl_ptr = ptr + (j * layout->page_size);
			calculate_pebtbl_fragment_checksum(pebtbl_ptr);
		}
	}
}

static
void maptbl_define_migration_threshold(struct ssdfs_volume_layout *layout,
					int seg_index, int peb_index)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *lf_extent;
	struct ssdfs_log_footer *footer;

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, seg_index %d, peb_index %d, "
		  "segbmap migration_threshold %u\n",
		  layout, seg_index, peb_index,
		  layout->maptbl.migration_threshold);

	seg_desc = &layout->segs[seg_index];
	peb_desc = &seg_desc->pebs[peb_index];
	lf_extent = &peb_desc->extents[SSDFS_LOG_FOOTER];

	BUG_ON(!lf_extent->buf);
	footer = (struct ssdfs_log_footer *)lf_extent->buf;

	footer->volume_state.migration_threshold =
			cpu_to_le16(layout->maptbl.migration_threshold);
}

int maptbl_mkfs_commit(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_metadata_desc *meta_desc;
	int seg_index;
	int segs_count;
	u32 maptbl_pebs;
	u32 pebs_per_seg;
	u32 portions_count = layout->maptbl.portions_count;
	u32 metadata_blks;
	int i, j;
	int fragment_index = 0;
	int err;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

/* TODO: process backup copy of maptbl */

	meta_desc = &layout->meta_array[SSDFS_PEB_MAPPING_TABLE];
	segs_count = meta_desc->segs_count;
	maptbl_pebs = layout->maptbl.maptbl_pebs;
	pebs_per_seg = (u32)(layout->seg_size / layout->env.erase_size);

	if (segs_count <= 0 ||
	    segs_count > ((maptbl_pebs + pebs_per_seg - 1) / pebs_per_seg)) {
		SSDFS_ERR("invalid segs_count %d\n", segs_count);
		return -ERANGE;
	}

	if (meta_desc->start_seg_index >= layout->segs_capacity) {
		SSDFS_ERR("start_seg_index %d >= segs_capacity %d\n",
			  meta_desc->start_seg_index,
			  layout->segs_capacity);
		return -ERANGE;
	}

	seg_index = meta_desc->start_seg_index;

	for (i = 0; i < segs_count; i++) {
		for (j = 0; j < pebs_per_seg; j++) {
			void *ptr;
			struct ssdfs_leb_table_fragment_header *hdr;
			struct ssdfs_peb_content *peb_desc;
			struct ssdfs_extent_desc *extent;
			u32 blks;

			if (fragment_index >= portions_count)
				break;

			BUG_ON(j >= layout->segs[seg_index].pebs_capacity);

			peb_desc = &layout->segs[seg_index].pebs[j];
			extent = &peb_desc->extents[SSDFS_LOG_PAYLOAD];
			BUG_ON(!extent->buf);

			ptr = (u8 *)extent->buf;
			hdr = (struct ssdfs_leb_table_fragment_header *)ptr;

			if (le16_to_cpu(hdr->magic) != SSDFS_LEB_TABLE_MAGIC)
				break;

			err = pre_commit_segment_header(layout, seg_index, j,
							SSDFS_MAPTBL_SEG_TYPE);
			if (err)
				return err;

			calculate_peb_fragments_checksum(layout, extent->buf);

			err = pre_commit_log_footer(layout, seg_index,
						    j);
			if (err)
				return err;

			maptbl_define_migration_threshold(layout,
							  seg_index, j);

			metadata_blks = calculate_metadata_blks(layout,
							SSDFS_MAPTBL_SEG_TYPE,
							peb_desc);

			commit_block_bitmap(layout, seg_index, j,
					    metadata_blks);
			commit_offset_table(layout, seg_index, j);
			commit_block_descriptors(layout, seg_index, j);

			if (layout->blkbmap.has_backup_copy) {
				commit_block_bitmap_backup(layout, seg_index,
							   j, metadata_blks);
			}

			if (layout->blk2off_tbl.has_backup_copy) {
				commit_offset_table_backup(layout, seg_index,
							   j);
			}

			blks = calculate_log_pages(layout,
						   SSDFS_MAPTBL_SEG_TYPE,
						   peb_desc);
			commit_log_footer(layout, seg_index, j, blks);
			commit_segment_header(layout, seg_index, j,
					      blks);

			fragment_index++;
		}

		seg_index++;
	}

	layout->segs_count += segs_count;
	return 0;
}
