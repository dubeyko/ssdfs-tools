//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/mkfs.ssdfs/segment_bitmap.c - bitmap of segments creation functionality.
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

#include "mkfs.h"

/************************************************************************
 *                Segment bitmap creation functionality                 *
 ************************************************************************/

int segbmap_mkfs_allocation_policy(struct ssdfs_volume_layout *layout,
				   int *segs)
{
	int seg_state = SSDFS_DEDICATED_SEGMENT;
	u64 seg_nums;
	u16 fragments;
	u16 fragments_per_peb = layout->segbmap.fragments_per_peb;
	u16 fragments_per_seg;
	u8 segs_per_chain = layout->segbmap.segs_per_chain;
	u32 fragment_size = PAGE_CACHE_SIZE;
	u16 pebs_per_seg;
	u8 segbmap_segs;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	seg_nums = layout->env.fs_size / layout->seg_size;
	layout->segbmap.bmap_bytes = SEG_BMAP_BYTES(seg_nums);
	fragments = SEG_BMAP_FRAGMENTS(seg_nums);
	pebs_per_seg = (u64)(layout->seg_size / layout->env.erase_size);

	fragments_per_seg = fragments_per_peb * pebs_per_seg;
	segbmap_segs = (u8)((fragments + fragments_per_seg - 1) /
				fragments_per_seg);

	if (layout->env.erase_size < ((u32)fragments_per_peb * fragment_size) ||
	    segbmap_segs > SSDFS_SEGBMAP_SEGS) {
		fragments_per_peb = (u16)(layout->env.erase_size / fragment_size);
		fragments_per_peb = (fragments_per_peb * 70) / 100;
		fragments_per_peb = min_t(u16, fragments_per_peb, fragments);
		layout->segbmap.fragments_per_peb = fragments_per_peb;

		SSDFS_WARN("it will be used the new value: "
			   "fragments_per_peb %d\n",
			   fragments_per_peb);
	}

	layout->segbmap.pebs_per_seg = pebs_per_seg;
	fragments_per_seg = fragments_per_peb * pebs_per_seg;
	segbmap_segs =  (u8)((fragments + fragments_per_seg - 1) /
				fragments_per_seg);

	if (segbmap_segs > SSDFS_SEGBMAP_SEGS) {
		SSDFS_ERR("segbmap_segs %u > max %u\n",
			  segbmap_segs,
			  SSDFS_SEGBMAP_SEGS);
		return -E2BIG;
	}

	if (segs_per_chain != segbmap_segs) {
		segs_per_chain = segbmap_segs;
		layout->segbmap.segs_per_chain = segs_per_chain;

		SSDFS_WARN("it will be used the new value: "
			   "segs_per_chain %d\n",
			   segs_per_chain);
	}

	if (layout->segbmap.has_backup_copy)
		*segs = segs_per_chain * 2;
	else
		*segs = segs_per_chain;

	layout->segbmap.fragments_count = fragments;
	layout->segbmap.fragment_size = fragment_size;

	layout->meta_array[SSDFS_SEGBMAP].segs_count = *segs;
	layout->meta_array[SSDFS_SEGBMAP].seg_state = seg_state;

	SSDFS_DBG(layout->env.show_debug,
		  "segbmap: segs %d, segs_per_chain %u, "
		  "fragments_count %u, fragment_size %u, "
		  "fragments_per_peb %u\n",
		  *segs, segs_per_chain, fragments,
		  fragment_size, fragments_per_peb);
	return seg_state;
}

static int segbmap_create_fragments_array(struct ssdfs_volume_layout *layout)
{
	size_t fragment_size = layout->segbmap.fragment_size;
	u16 pebs_per_seg = layout->segbmap.pebs_per_seg;
	u16 fragments_per_peb = layout->segbmap.fragments_per_peb;
	size_t peb_buffer_size = fragment_size * fragments_per_peb;
	u16 segs_per_chain = layout->segbmap.segs_per_chain;
	int buffers_count = (int)segs_per_chain * pebs_per_seg;
	int i;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	layout->segbmap.fragments_array = calloc(buffers_count, sizeof(void *));
	if (layout->segbmap.fragments_array == NULL) {
		SSDFS_ERR("fail to allocate segbmap's fragments array: "
			  "buffers_count %u\n",
			  buffers_count);
		return -ENOMEM;
	}

	for (i = 0; i < buffers_count; i++) {
		layout->segbmap.fragments_array[i] = calloc(1, peb_buffer_size);
		if (layout->segbmap.fragments_array[i] == NULL) {
			SSDFS_ERR("fail to allocate PEB's buffer: "
				  "index %d\n", i);
			goto free_buffers;
		}
	}

	return 0;

free_buffers:
	for (i--; i >= 0; i--) {
		free(layout->segbmap.fragments_array[i]);
		layout->segbmap.fragments_array[i] = NULL;
	}
	free(layout->segbmap.fragments_array);
	layout->segbmap.fragments_array = NULL;
	return -ENOMEM;
}

void segbmap_destroy_fragments_array(struct ssdfs_volume_layout *layout)
{
	u16 pebs_per_seg = layout->segbmap.pebs_per_seg;
	u8 segs_per_chain = layout->segbmap.segs_per_chain;
	int buffers_count = (int)segs_per_chain * pebs_per_seg;
	int i;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	if (layout->segbmap.fragments_array) {
		for (i = 0; i < buffers_count; i++) {
			free(layout->segbmap.fragments_array[i]);
			layout->segbmap.fragments_array[i] = NULL;
		}
	}
	free(layout->segbmap.fragments_array);
	layout->segbmap.fragments_array = NULL;
}

static int segbmap_prepare_fragment(struct ssdfs_volume_layout *layout,
				    int index)
{
	struct ssdfs_segbmap_fragment_header *hdr;
	size_t hdr_size = sizeof(struct ssdfs_segbmap_fragment_header);
	u8 *ptr;
	u16 fragments = layout->segbmap.fragments_count;
	u16 fragments_per_peb = layout->segbmap.fragments_per_peb;
	size_t fragment_size = layout->segbmap.fragment_size;
	u32 payload_bytes;
	u64 fragment_bytes;
	u16 pebs_per_seg = layout->segbmap.pebs_per_seg;
	u16 fragments_per_seg;
	u16 seg_index;
	u16 peb_index;
	u64 start_item;
	u32 items_per_fragment;

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, index %d\n", layout, index);

	if (index >= fragments) {
		SSDFS_ERR("invalid index: index %d >= fragments %u\n",
			  index, fragments);
		return -EINVAL;
	}

	fragments_per_seg = pebs_per_seg * fragments_per_peb;
	seg_index = index / fragments_per_seg;
	peb_index = (seg_index - index) / fragments_per_peb;

	SSDFS_DBG(layout->env.show_debug,
		  "fragments_per_seg %u, fragments_per_peb %u, "
		  "index %d, seg_index %u, peb_index %u\n",
		  fragments_per_seg, fragments_per_peb,
		  index, seg_index, peb_index);

	ptr = (u8 *)layout->segbmap.fragments_array[index / fragments_per_peb];
	ptr += (index % fragments_per_peb) * fragment_size;
	hdr = (struct ssdfs_segbmap_fragment_header *)(ptr);

	start_item = ssdfs_segbmap_define_first_fragment_item(index,
							      fragment_size);

	payload_bytes = ssdfs_segbmap_payload_bytes_per_fragment(fragment_size);
	fragment_bytes = layout->segbmap.bmap_bytes + (fragments * hdr_size);
	fragment_bytes -= ((u64)index * (payload_bytes + hdr_size));
	fragment_bytes = min_t(u64, fragment_bytes, (u64)fragment_size);
	BUG_ON(fragment_bytes >= USHRT_MAX);

	items_per_fragment = ssdfs_segbmap_items_per_fragment(fragment_bytes);
	BUG_ON(items_per_fragment >= USHRT_MAX);

	hdr->magic = cpu_to_le16(SSDFS_SEGBMAP_HDR_MAGIC);
	hdr->seg_index = cpu_to_le16(seg_index);
	hdr->peb_index = cpu_to_le16(peb_index);
	hdr->flags = 0;

	hdr->start_item = cpu_to_le64(start_item);
	hdr->sequence_id = cpu_to_le16(index);
	hdr->fragment_bytes = cpu_to_le16(fragment_bytes);

	hdr->total_segs = cpu_to_le16(items_per_fragment);
	hdr->clean_or_using_segs = cpu_to_le16(items_per_fragment);
	hdr->used_or_dirty_segs = 0;
	hdr->bad_segs = 0;

	return 0;
}

int segbmap_mkfs_prepare(struct ssdfs_volume_layout *layout)
{
	u16 fragments = layout->segbmap.fragments_count;
	int i;
	int err;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	err = reserve_segments(layout, SSDFS_SEGBMAP);
	if (err) {
		SSDFS_ERR("fail to reserve segments: err %d\n", err);
		return err;
	}

	err = segbmap_create_fragments_array(layout);
	if (err) {
		SSDFS_ERR("fail to create fragments array: err %d\n",
			  err);
		return err;
	}

	for (i = 0; i < fragments; i++) {
		err = segbmap_prepare_fragment(layout, i);
		if (err) {
			SSDFS_ERR("fail to prepare fragment: "
				  "index %d, err %d\n",
				  i, err);
			return err;
		}
	}

	return 0;
}

static void define_leb_id(struct ssdfs_segment_desc *desc)
{
	u64 start_leb_id = desc->seg_id * desc->pebs_capacity;
	int i;

	for (i = 0; i < desc->pebs_capacity; i++)
		desc->pebs[i].leb_id = start_leb_id + i;
}

static int define_seg_id(struct ssdfs_volume_layout *layout,
			 int new_state,
			 struct ssdfs_segment_desc *desc)
{
	struct ssdfs_segbmap_fragment_header *hdr;
	size_t fragment_size = layout->segbmap.fragment_size;
	u16 fragments_per_peb = layout->segbmap.fragments_per_peb;
	u64 nsegs = layout->env.fs_size / layout->seg_size;
	int fragment_index;
	void *fragment;
	u8 *bmap;
	u64 found_seg = U64_MAX;
	int err;

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, new_state %#x\n", layout, new_state);

	fragment_index = 0;

try_fragment:
	fragment = (u8 *)layout->segbmap.fragments_array[0] +
			(fragment_index * fragment_size);
	hdr = (struct ssdfs_segbmap_fragment_header *)fragment;

	bmap = (u8 *)fragment + sizeof(struct ssdfs_segbmap_fragment_header);

	err = SET_FIRST_CLEAN_ITEM_IN_FRAGMENT(hdr, bmap, 0, nsegs,
						new_state, &found_seg);
	if (err == -ENODATA || found_seg == U64_MAX) {
		if ((fragment_index + 1) >= fragments_per_peb) {
			/*
			 * Amount of fragments_per_peb should be enough
			 * for volume creation. Otherwise, it needs
			 * to add implementation here.
			 */
			SSDFS_DBG(layout->env.show_debug,
				  "index %d >= fragments_per_peb %u\n",
				  fragment_index + 1,
				  fragments_per_peb);
			return -ERANGE;
		}

		fragment_index++;
		goto try_fragment;
	} else if (err) {
		SSDFS_ERR("fail to find clean segment: err %d\n", err);
		return err;
	}

	desc->seg_id = found_seg;

	define_leb_id(desc);

	SSDFS_DBG(layout->env.show_debug,
		  "seg_type %#x, seg_id %llu\n",
		  desc->seg_type, desc->seg_id);

	return 0;
}

static int define_snap_seg_id(struct ssdfs_volume_layout *layout,
			      struct ssdfs_segment_desc *desc)
{
	if (desc->seg_type != SSDFS_INITIAL_SNAPSHOT) {
		SSDFS_ERR("invalid seg_type %#x\n", desc->seg_type);
		return -ERANGE;
	}

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	return define_seg_id(layout, SSDFS_SEG_RESERVED, desc);
}

static int define_sb_seg_id(struct ssdfs_volume_layout *layout,
			    struct ssdfs_segment_desc *desc)
{
	if (desc->seg_type != SSDFS_SUPERBLOCK) {
		SSDFS_ERR("invalid seg_type %#x\n", desc->seg_type);
		return -ERANGE;
	}

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	return define_seg_id(layout, SSDFS_SEG_RESERVED, desc);
}

static int define_segbmap_seg_id(struct ssdfs_volume_layout *layout,
				 struct ssdfs_segment_desc *desc)
{
	if (desc->seg_type != SSDFS_SEGBMAP) {
		SSDFS_ERR("invalid seg_type %#x\n", desc->seg_type);
		return -ERANGE;
	}

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	return define_seg_id(layout, SSDFS_SEG_RESERVED, desc);
}

static int define_maptbl_seg_id(struct ssdfs_volume_layout *layout,
				 struct ssdfs_segment_desc *desc)
{
	if (desc->seg_type != SSDFS_PEB_MAPPING_TABLE) {
		SSDFS_ERR("invalid seg_type %#x\n", desc->seg_type);
		return -ERANGE;
	}

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	return define_seg_id(layout, SSDFS_SEG_RESERVED, desc);
}

static int define_user_data_seg_id(struct ssdfs_volume_layout *layout,
				   struct ssdfs_segment_desc *desc)
{
	if (desc->seg_type != SSDFS_USER_DATA) {
		SSDFS_ERR("invalid seg_type %#x\n", desc->seg_type);
		return -ERANGE;
	}

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	/* do nothing */
	return 0;
}

typedef int (*define_seg_op)(struct ssdfs_volume_layout *ptr,
			     struct ssdfs_segment_desc *desc);

static define_seg_op define_seg[SSDFS_METADATA_ITEMS_MAX] = {
	define_snap_seg_id,
	define_sb_seg_id,
	define_segbmap_seg_id,
	define_maptbl_seg_id,
	define_user_data_seg_id,
};

static int init_segbmap_sb_header(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_segbmap_sb_header *hdr = &layout->sb.vh.segbmap;
	struct ssdfs_metadata_desc *desc;
	u32 fragments_per_peb = layout->segbmap.fragments_per_peb;
	u32 pebs_per_seg = layout->segbmap.pebs_per_seg;
	u32 fragments_per_seg;
	u16 flags = 0;
	int seg_index;
	int i, j;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	BUG_ON(layout->segbmap.fragments_count >= USHRT_MAX);
	hdr->fragments_count = cpu_to_le16(layout->segbmap.fragments_count);

	fragments_per_seg = fragments_per_peb * pebs_per_seg;
	BUG_ON(fragments_per_seg >= USHRT_MAX);
	hdr->fragments_per_seg = cpu_to_le16(fragments_per_seg);

	hdr->fragments_per_peb = cpu_to_le16(fragments_per_peb);

	BUG_ON(layout->segbmap.fragment_size >= USHRT_MAX);
	hdr->fragment_size = cpu_to_le16(layout->segbmap.fragment_size);

	hdr->bytes_count = cpu_to_le32(layout->segbmap.bmap_bytes);

	if (layout->segbmap.has_backup_copy)
		flags |= SSDFS_SEGBMAP_HAS_COPY;

	switch (layout->segbmap.compression) {
	case SSDFS_UNCOMPRESSED_BLOB:
		/* do nothing */
		break;

	case SSDFS_ZLIB_BLOB:
		flags |= SSDFS_SEGBMAP_MAKE_ZLIB_COMPR;
		break;

	case SSDFS_LZO_BLOB:
		flags |= SSDFS_SEGBMAP_MAKE_LZO_COMPR;
		break;

	default:
		SSDFS_ERR("invalid compression type %#x\n",
			  layout->segbmap.compression);
		return -ERANGE;
	}

	hdr->flags = cpu_to_le16(flags);

	hdr->segs_count = cpu_to_le16(layout->segbmap.segs_per_chain);

	desc = &layout->meta_array[SSDFS_SEGBMAP];
	seg_index = desc->start_seg_index;

	for (i = 0; i < SSDFS_SEGBMAP_SEGS; i++) {
		for (j = 0; j < SSDFS_SEGBMAP_SEG_COPY_MAX; j++) {
			hdr->segs[i][j] = cpu_to_le64(U64_MAX);
		}
	}

	BUG_ON(layout->segbmap.segs_per_chain > SSDFS_SEGBMAP_SEGS);

	for (i = 0; i < layout->segbmap.segs_per_chain; i++) {
		int replication = SSDFS_SEGBMAP_SEG_COPY_MAX;

		for (j = 0; j < replication; j++) {
			int type = layout->segs[seg_index].seg_type;
			u64 seg_id = layout->segs[seg_index].seg_id;

			if (type != SSDFS_SEGBMAP) {
				SSDFS_ERR("invalid seg_type %#x\n", type);
				return -ERANGE;
			}

			hdr->segs[i][j] = cpu_to_le64(seg_id);
			seg_index++;

			if (!layout->segbmap.has_backup_copy)
				break;
		}
	}

	return 0;
}

static void set_segbmap_presence_flag(struct ssdfs_volume_layout *layout)
{
	u64 feature_compat = le64_to_cpu(layout->sb.vs.feature_compat);
	feature_compat |= SSDFS_HAS_SEGBMAP_COMPAT_FLAG;
	layout->sb.vs.feature_compat = cpu_to_le64(feature_compat);
}

int segbmap_mkfs_validate(struct ssdfs_volume_layout *layout)
{
	u64 seg_size = layout->seg_size;
	u32 erase_size = layout->env.erase_size;
	u32 pebs_per_seg = (u32)(seg_size / erase_size);
	int i;
	int err;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	for (i = 0; i < layout->segs_capacity; i++) {
		int seg_type = layout->segs[i].seg_type;

		if (seg_type < 0 || seg_type >= SSDFS_METADATA_ITEMS_MAX) {
			SSDFS_ERR("invalid seg_type %#x\n", seg_type);
			return -ERANGE;
		}

		err = define_seg[seg_type](layout, &layout->segs[i]);
		if (err) {
			SSDFS_ERR("fail to define segment number: "
				  "seg_type %#x, seg_index %d, err %d\n",
				  seg_type, i, err);
			return err;
		}
	}

	err = init_segbmap_sb_header(layout);
	if (err) {
		SSDFS_ERR("fail to initialize segbmap_sb_header: err %d\n",
			  err);
		return err;
	}

	if (layout->segbmap.migration_threshold >= U16_MAX) {
		layout->segbmap.migration_threshold =
					layout->migration_threshold;
	} else if (layout->segbmap.migration_threshold > pebs_per_seg) {
		SSDFS_WARN("user data migration threshold %u "
			   "was corrected to %u\n",
			   layout->segbmap.migration_threshold,
			   pebs_per_seg);
		layout->segbmap.migration_threshold = pebs_per_seg;
	}

	set_segbmap_presence_flag(layout);
	return 0;
}

static void segbmap_set_log_pages(struct ssdfs_volume_layout *layout,
				  u32 blks)
{
	u32 erasesize;
	u32 pagesize;
	u32 pages_per_peb;
	u32 log_pages = 0;

	SSDFS_DBG(layout->env.show_debug,
		  "log_pages %u, blks_count %u\n",
		  layout->segbmap.log_pages, blks);

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

	blks = min_t(u32, blks, (u32)SSDFS_LOG_MAX_PAGES);

	if (layout->segbmap.log_pages == U16_MAX)
		log_pages = blks;
	else {
		log_pages = layout->segbmap.log_pages;

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
	while (layout->env.erase_size % (log_pages * layout->page_size))
		log_pages++;

	if ((log_pages - blks) < 3) {
		log_pages += 3;
		goto try_align_log_pages;
	}

	if (pages_per_peb % log_pages) {
		SSDFS_WARN("pages_per_peb %u, log_pages %u\n",
			   pages_per_peb, log_pages);
	}

	layout->segbmap.log_pages = log_pages;
	BUG_ON(log_pages >= U16_MAX);
	layout->sb.vh.segbmap_log_pages = cpu_to_le16((u16)log_pages);
}

int segbmap_mkfs_define_layout(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_metadata_desc *meta_desc;
	int seg_index;
	int segs_count;
	u16 pebs_per_seg = layout->segbmap.pebs_per_seg;
	u32 fragments_count = layout->segbmap.fragments_count;
	size_t fragment_size = layout->segbmap.fragment_size;
	u16 fragments_per_peb = layout->segbmap.fragments_per_peb;
	size_t peb_buffer_size = fragment_size * fragments_per_peb;
	u32 page_size = layout->page_size;
	u32 valid_blks;
	int i, j;
	int fragment_index = 0;
	u32 log_pages = 0;
	u32 payload_offset_in_bytes = 0;
	u32 start_logical_blk;
	int err;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	meta_desc = &layout->meta_array[SSDFS_SEGBMAP];
	segs_count = meta_desc->segs_count;

	if (segs_count <= 0 ||
	    segs_count > (SSDFS_SEGBMAP_SEGS * SSDFS_SEGBMAP_SEG_COPY_MAX)) {
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

		for (j = 0; j < pebs_per_seg; j++) {
			struct ssdfs_peb_content *peb_desc;
			struct ssdfs_extent_desc *extent;
			u32 blks;
			u64 logical_byte_offset;

			if (fragment_index >= fragments_count)
				break;

			logical_byte_offset =
				(u64)fragment_index * fragment_size;

			layout->segs[seg_index].pebs_count++;
			BUG_ON(layout->segs[seg_index].pebs_count >
				layout->segs[seg_index].pebs_capacity);
			peb_desc = &layout->segs[seg_index].pebs[j];

			err = set_extent_start_offset(layout, peb_desc,
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

			err = set_extent_start_offset(layout, peb_desc,
							SSDFS_BLOCK_BITMAP);
			if (err) {
				SSDFS_ERR("fail to define extent's offset: "
					  "err %d\n", err);
				return err;
			}

			err = pre_commit_block_bitmap(layout, seg_index, j,
						      valid_blks);
			if (err)
				return err;

			err = set_extent_start_offset(layout, peb_desc,
							SSDFS_OFFSET_TABLE);
			if (err) {
				SSDFS_ERR("fail to define extent's offset: "
					  "err %d\n", err);
				return err;
			}

			err = pre_commit_offset_table(layout, seg_index, j,
							logical_byte_offset,
							start_logical_blk,
							valid_blks);
			if (err)
				return err;

			err = set_extent_start_offset(layout, peb_desc,
						      SSDFS_BLOCK_DESCRIPTORS);
			if (err) {
				SSDFS_ERR("fail to define extent's offset: "
					  "err %d\n", err);
				return err;
			}

			SSDFS_DBG(layout->env.show_debug,
				  "fragment_index %d, "
				  "start_logical_blk %u, "
				  "payload_offset_in_bytes %u\n",
				  fragment_index,
				  start_logical_blk,
				  payload_offset_in_bytes);

			err = pre_commit_block_descriptors(layout, seg_index,
							j, valid_blks,
							SSDFS_SEG_BMAP_INO,
							payload_offset_in_bytes,
							PAGE_CACHE_SIZE);
			if (err)
				return err;

			err = set_extent_start_offset(layout, peb_desc,
							SSDFS_LOG_PAYLOAD);
			if (err) {
				SSDFS_ERR("fail to define extent's offset: "
					  "err %d\n", err);
				return err;
			}

			extent = &peb_desc->extents[SSDFS_LOG_PAYLOAD];

			BUG_ON(extent->buf);
			extent->buf =
				layout->segbmap.fragments_array[fragment_index];
			if (!extent->buf) {
				SSDFS_ERR("invalid fragment pointer: "
					  "buffer_index %d\n",
					  i + j);
				return -ERANGE;
			}
			layout->segbmap.fragments_array[fragment_index] = NULL;

			extent->bytes_count = peb_buffer_size;

			err = set_extent_start_offset(layout, peb_desc,
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
				err = set_extent_start_offset(layout, peb_desc,
						    SSDFS_BLOCK_BITMAP_BACKUP);
				if (err) {
					SSDFS_ERR("fail to define offset: "
						  "err %d\n", err);
					return err;
				}

				err = pre_commit_block_bitmap_backup(layout,
								    seg_index,
								    j,
								    valid_blks);
				if (err)
					return err;
			}

			if (layout->blk2off_tbl.has_backup_copy) {
				err = set_extent_start_offset(layout, peb_desc,
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
							valid_blks);
				if (err)
					return err;
			}

			blks = calculate_log_pages(layout, peb_desc);
			log_pages = max_t(u32, blks, log_pages);

			fragment_index++;
			payload_offset_in_bytes += peb_buffer_size;
			start_logical_blk += valid_blks;
		}

		seg_index++;
	}

	segbmap_set_log_pages(layout, log_pages);
	return 0;
}

static void calculate_peb_fragments_checksum(struct ssdfs_volume_layout *layout,
					     void *fragments)
{
	size_t fragment_size = layout->segbmap.fragment_size;
	u16 fragments_per_peb = layout->segbmap.fragments_per_peb;
	int i;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	for (i = 0; i < fragments_per_peb; i++) {
		struct ssdfs_segbmap_fragment_header *hdr;
		u16 fragment_bytes;
		u8 *ptr = (u8 *)fragments + (i * fragment_size);

		hdr = (struct ssdfs_segbmap_fragment_header *)ptr;
		fragment_bytes = le16_to_cpu(hdr->fragment_bytes);

		hdr->checksum = 0;
		hdr->checksum = ssdfs_crc32_le(ptr, fragment_bytes);
	}
}

static
void segbmap_define_migration_threshold(struct ssdfs_volume_layout *layout,
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
			cpu_to_le16(layout->segbmap.migration_threshold);
}

int segbmap_mkfs_commit(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_metadata_desc *meta_desc;
	int seg_index;
	int segs_count;
	u16 pebs_per_seg = layout->segbmap.pebs_per_seg;
	u32 fragments_count = layout->segbmap.fragments_count;
	u32 metadata_blks;
	int i, j;
	int fragment_index = 0;
	int err;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

/* TODO: process backup copy of segbmap */

	meta_desc = &layout->meta_array[SSDFS_SEGBMAP];
	segs_count = meta_desc->segs_count;

	if (segs_count <= 0 ||
	    segs_count > (SSDFS_SEGBMAP_SEGS * SSDFS_SEGBMAP_SEG_COPY_MAX)) {
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
			u8 *ptr;
			struct ssdfs_segbmap_fragment_header *hdr;
			struct ssdfs_peb_content *peb_desc;
			struct ssdfs_extent_desc *extent;
			u32 blks;

			if (fragment_index >= fragments_count)
				break;

			BUG_ON(j >= layout->segs[seg_index].pebs_capacity);

			peb_desc = &layout->segs[seg_index].pebs[j];
			extent = &peb_desc->extents[SSDFS_LOG_PAYLOAD];

			ptr = (u8 *)extent->buf;
			BUG_ON(!ptr);
			hdr = (struct ssdfs_segbmap_fragment_header *)ptr;

			if (le16_to_cpu(hdr->magic) != SSDFS_SEGBMAP_HDR_MAGIC)
				break;

			err = pre_commit_segment_header(layout, seg_index, j,
							SSDFS_SEGBMAP_SEG_TYPE);
			if (err)
				return err;

			calculate_peb_fragments_checksum(layout, extent->buf);

			err = pre_commit_log_footer(layout, seg_index,
						    j);
			if (err)
				return err;

			segbmap_define_migration_threshold(layout,
							   seg_index, j);

			metadata_blks = calculate_metadata_blks(layout,
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

			blks = calculate_log_pages(layout, peb_desc);
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
