//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/mkfs.ssdfs/common.c - common creation functionality.
 *
 * Copyright (c) 2014-2018 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 *
 * HGST Confidential
 * (C) Copyright 2009-2018, HGST, Inc., All rights reserved.
 *
 * Created by HGST, San Jose Research Center, Storage Architecture Group
 * Authors: Vyacheslav Dubeyko <slava@dubeyko.com>
 *
 * Acknowledgement: Cyril Guyot <Cyril.Guyot@wdc.com>
 *                  Zvonimir Bandic <Zvonimir.Bandic@wdc.com>
 */

#include <zlib.h>

#include "mkfs.h"
#include "blkbmap.h"

/************************************************************************
 *                      Common creation functionality                   *
 ************************************************************************/

const char *uuid_string(const unsigned char *uuid)
{
	static char buf[256];

	sprintf(buf, "%02x%02x%02x%02x-%02x%02x-%02x%02x-" \
		"%02x%02x-%02x%02x%02x%02x%02x%02x", uuid[0], uuid[1],
		uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7],
		uuid[8], uuid[9], uuid[10], uuid[11], uuid[12],
		uuid[13], uuid[14], uuid[15]);
	return buf;
}

int reserve_segments(struct ssdfs_volume_layout *layout,
		     int meta_index)
{
	struct ssdfs_metadata_desc *desc;
	int index = layout->last_allocated_seg_index + 1;
	int i;

	if (meta_index >= SSDFS_METADATA_ITEMS_MAX) {
		SSDFS_ERR("invalid meta_index %d\n", meta_index);
		return -EINVAL;
	}

	desc = &layout->meta_array[meta_index];

	if (desc->segs_count <= 0) {
		SSDFS_WARN("desc->segs_count %d\n", desc->segs_count);
		return 0;
	}

	if (index >= layout->segs_capacity) {
		SSDFS_ERR("start_seg_index %d >= layout->segs_capacity %d\n",
			  index, layout->segs_capacity);
		return -ERANGE;
	}

	desc->start_seg_index = index;

	for (i = 0; i < desc->segs_count; i++) {
		layout->segs[index + i].seg_type = meta_index;
		layout->segs[index + i].seg_state = desc->seg_state;
		layout->last_allocated_seg_index++;
	}

	SSDFS_DBG(layout->env.show_debug,
		  "meta_index %d, start_seg_index %d, segs_count %d\n",
		  meta_index, desc->start_seg_index, desc->segs_count);

	return 0;
}

int set_extent_start_offset(struct ssdfs_volume_layout *layout,
			    struct ssdfs_peb_content *desc,
			    int extent_index)
{
	u32 offset = desc->extents[SSDFS_SEG_HEADER].offset;

	switch (extent_index) {
	case SSDFS_MAPTBL_CACHE:
	case SSDFS_LOG_PAYLOAD:
	case SSDFS_LOG_FOOTER:
	case SSDFS_BLOCK_BITMAP_BACKUP:
	case SSDFS_OFFSET_TABLE_BACKUP:
		offset += desc->extents[SSDFS_BLOCK_DESCRIPTORS].bytes_count;
		/* pass through */

	case SSDFS_BLOCK_DESCRIPTORS:
		offset += desc->extents[SSDFS_OFFSET_TABLE].bytes_count;
		/* pass through */

	case SSDFS_OFFSET_TABLE:
		offset += desc->extents[SSDFS_BLOCK_BITMAP].bytes_count;
		/* pass through */

	case SSDFS_BLOCK_BITMAP:
		offset += desc->extents[SSDFS_SEG_HEADER].bytes_count;
		/* pass through */

	case SSDFS_SEG_HEADER:
		/* do nothing */
		break;

	default:
		SSDFS_ERR("invalid extent_index %d\n",
			  extent_index);
		return -EINVAL;
	}

	if (extent_index < SSDFS_MAPTBL_CACHE)
		goto set_extent_offset;

	offset += layout->page_size - 1;
	offset = (offset / layout->page_size) * layout->page_size;

	switch (extent_index) {
	case SSDFS_OFFSET_TABLE_BACKUP:
	case SSDFS_BLOCK_BITMAP_BACKUP:
		/* do nothing */
		/* pass through */

	case SSDFS_LOG_FOOTER:
		offset += desc->extents[SSDFS_LOG_PAYLOAD].bytes_count;
		offset += layout->page_size - 1;
		offset = (offset / layout->page_size) * layout->page_size;
		/* pass through */

	case SSDFS_LOG_PAYLOAD:
		offset += desc->extents[SSDFS_MAPTBL_CACHE].bytes_count;
		offset += layout->page_size - 1;
		offset = (offset / layout->page_size) * layout->page_size;
		break;

	case SSDFS_MAPTBL_CACHE:
		/* do nothing */
		break;

	default:
		BUG();
	}

	if (extent_index < SSDFS_BLOCK_BITMAP_BACKUP)
		goto set_extent_offset;

	switch (extent_index) {
	case SSDFS_OFFSET_TABLE_BACKUP:
		offset += desc->extents[SSDFS_BLOCK_BITMAP_BACKUP].bytes_count;
		/* pass through */

	case SSDFS_BLOCK_BITMAP_BACKUP:
		offset += desc->extents[SSDFS_LOG_FOOTER].bytes_count;
		break;

	default:
		BUG();
	}

set_extent_offset:
	desc->extents[extent_index].offset = offset;

	return 0;
}

u32 calculate_log_pages(struct ssdfs_volume_layout *layout,
			struct ssdfs_peb_content *desc)
{
	u32 bytes_count = 0;
	u32 pages_count;

	bytes_count += desc->extents[SSDFS_SEG_HEADER].bytes_count;
	bytes_count += desc->extents[SSDFS_BLOCK_BITMAP].bytes_count;
	bytes_count += desc->extents[SSDFS_OFFSET_TABLE].bytes_count;

	bytes_count += layout->page_size - 1;
	bytes_count = (bytes_count / layout->page_size) * layout->page_size;

	bytes_count += desc->extents[SSDFS_MAPTBL_CACHE].bytes_count;

	bytes_count += layout->page_size - 1;
	bytes_count = (bytes_count / layout->page_size) * layout->page_size;

	bytes_count += desc->extents[SSDFS_LOG_PAYLOAD].bytes_count;

	bytes_count += layout->page_size - 1;
	bytes_count = (bytes_count / layout->page_size) * layout->page_size;

	bytes_count += desc->extents[SSDFS_LOG_FOOTER].bytes_count;
	bytes_count += desc->extents[SSDFS_BLOCK_BITMAP_BACKUP].bytes_count;
	bytes_count += desc->extents[SSDFS_OFFSET_TABLE_BACKUP].bytes_count;

	bytes_count += layout->page_size - 1;
	BUG_ON(bytes_count > layout->env.erase_size);

	pages_count = bytes_count / layout->page_size;

	/* TODO: temporary alignment */
	pages_count++;
	while (layout->env.erase_size % (pages_count * layout->page_size))
		pages_count++;

	return pages_count;
}

u32 calculate_metadata_blks(struct ssdfs_volume_layout *layout,
			    struct ssdfs_peb_content *desc)
{
	u32 bytes_count = 0;
	u32 pages_count;

	bytes_count += desc->extents[SSDFS_SEG_HEADER].bytes_count;
	bytes_count += desc->extents[SSDFS_BLOCK_BITMAP].bytes_count;
	bytes_count += desc->extents[SSDFS_OFFSET_TABLE].bytes_count;

	bytes_count += layout->page_size - 1;
	bytes_count = (bytes_count / layout->page_size) * layout->page_size;

	bytes_count += desc->extents[SSDFS_MAPTBL_CACHE].bytes_count;

	bytes_count += layout->page_size - 1;
	bytes_count = (bytes_count / layout->page_size) * layout->page_size;

	bytes_count += desc->extents[SSDFS_LOG_FOOTER].bytes_count;
	bytes_count += desc->extents[SSDFS_BLOCK_BITMAP_BACKUP].bytes_count;
	bytes_count += desc->extents[SSDFS_OFFSET_TABLE_BACKUP].bytes_count;

	bytes_count += layout->page_size - 1;
	BUG_ON(bytes_count > layout->env.erase_size);

	pages_count = bytes_count / layout->page_size;

	return pages_count;
}

int define_segment_header_layout(struct ssdfs_volume_layout *layout,
				 int seg_index, int peb_index)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;
	size_t hdr_len = sizeof(struct ssdfs_segment_header);

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, peb_index %d\n",
		  seg_index, peb_index);

	if (seg_index >= layout->segs_capacity) {
		SSDFS_ERR("seg_index %d >= segs_capacity %d\n",
			  seg_index, layout->segs_capacity);
		return -EINVAL;
	}

	seg_desc = &layout->segs[seg_index];

	if (peb_index >= seg_desc->pebs_capacity) {
		SSDFS_ERR("peb_index %d >= pebs_capacity %d\n",
			  peb_index, seg_desc->pebs_capacity);
		return -EINVAL;
	}

	peb_desc = &seg_desc->pebs[peb_index];
	extent = &peb_desc->extents[SSDFS_SEG_HEADER];

	BUG_ON(extent->buf);

	extent->buf = calloc(1, hdr_len);
	if (!extent->buf) {
		SSDFS_ERR("unable to allocate memory of size %zu\n",
			  hdr_len);
		return -ENOMEM;
	}

	extent->bytes_count = hdr_len;

	return 0;
}

int pre_commit_segment_header(struct ssdfs_volume_layout *layout,
			      int seg_index, int peb_index,
			      u16 seg_type)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;
	struct ssdfs_segment_header *hdr;
	char *begin_ptr;

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, peb_index %d\n",
		  seg_index, peb_index);

	if (seg_index >= layout->segs_capacity) {
		SSDFS_ERR("seg_index %d >= segs_capacity %d\n",
			  seg_index, layout->segs_capacity);
		return -EINVAL;
	}

	seg_desc = &layout->segs[seg_index];

	if (peb_index >= seg_desc->pebs_capacity) {
		SSDFS_ERR("peb_index %d >= pebs_capacity %d\n",
			  peb_index, seg_desc->pebs_capacity);
		return -EINVAL;
	}

	peb_desc = &seg_desc->pebs[peb_index];
	extent = &peb_desc->extents[SSDFS_SEG_HEADER];

	BUG_ON(!extent->buf);
	BUG_ON(seg_type > SSDFS_LAST_KNOWN_SEG_TYPE);

	begin_ptr = extent->buf;
	memcpy(begin_ptr, (char *)&layout->sb.vh, sizeof(layout->sb.vh));

	hdr = (struct ssdfs_segment_header *)begin_ptr;
	hdr->volume_hdr.magic.key = cpu_to_le16(SSDFS_SEGMENT_HDR_MAGIC);
	hdr->timestamp = cpu_to_le64(layout->create_timestamp);
	hdr->cno = cpu_to_le64(layout->create_cno);
	hdr->seg_type = cpu_to_le16(seg_type);

	hdr->peb_migration_id[SSDFS_PREV_MIGRATING_PEB] =
					SSDFS_PEB_UNKNOWN_MIGRATION_ID;
	hdr->peb_migration_id[SSDFS_CUR_MIGRATING_PEB] =
					SSDFS_PEB_MIGRATION_ID_START;

	return 0;
}

static
void prepare_blkbmap_metadata_descriptor(struct ssdfs_volume_layout *layout,
					 struct ssdfs_extent_desc *extent,
					 struct ssdfs_metadata_descriptor *desc)
{
	size_t bmp_hdr_size = sizeof(struct ssdfs_block_bitmap_header);

	BUG_ON(!layout || !extent || !desc);
	BUG_ON(extent->bytes_count == 0 ||
		extent->bytes_count >= layout->env.erase_size);
	BUG_ON(extent->offset == 0 ||
		extent->offset >= layout->env.erase_size);

	desc->offset = cpu_to_le32(extent->offset);
	desc->size = cpu_to_le32(extent->bytes_count);

	desc->check.bytes = cpu_to_le16(bmp_hdr_size);
	desc->check.flags = cpu_to_le16(SSDFS_CRC32);
	desc->check.csum = 0;
	desc->check.csum = ssdfs_crc32_le(extent->buf, bmp_hdr_size);
}

static void
prepare_offset_table_metadata_descriptor(struct ssdfs_volume_layout *layout,
					 struct ssdfs_extent_desc *extent,
					 struct ssdfs_metadata_descriptor *desc)
{
	struct ssdfs_blk2off_table_header *hdr;

	BUG_ON(!layout || !extent || !desc);
	BUG_ON(extent->bytes_count == 0 ||
		extent->bytes_count >= layout->env.erase_size);
	BUG_ON(extent->offset == 0 ||
		extent->offset >= layout->env.erase_size);

	desc->offset = cpu_to_le32(extent->offset);
	desc->size = cpu_to_le32(extent->bytes_count);

	hdr = (struct ssdfs_blk2off_table_header *)extent->buf;

	memcpy(&desc->check, &hdr->check,
		sizeof(struct ssdfs_metadata_check));
}

static void
prepare_blk_desc_table_metadata_descriptor(struct ssdfs_volume_layout *layout,
					 struct ssdfs_extent_desc *extent,
					 struct ssdfs_metadata_descriptor *desc)
{
	struct ssdfs_block_state_descriptor *hdr;
	u16 fragments_count;
	u32 bytes_count;

	BUG_ON(!layout || !extent || !desc);
	BUG_ON(extent->bytes_count == 0 ||
		extent->bytes_count >= layout->env.erase_size);
	BUG_ON(extent->offset == 0 ||
		extent->offset >= layout->env.erase_size);

	desc->offset = cpu_to_le32(extent->offset);
	desc->size = cpu_to_le32(extent->bytes_count);

	hdr = (struct ssdfs_block_state_descriptor *)extent->buf;
	fragments_count = le16_to_cpu(hdr->chain_hdr.fragments_count);
	bytes_count = BLK_DESC_HEADER_SIZE(fragments_count);
	BUG_ON(bytes_count >= U16_MAX);

	desc->check.bytes = cpu_to_le16(bytes_count);
	desc->check.flags = cpu_to_le16(SSDFS_CRC32);
	desc->check.csum = 0;
	desc->check.csum = ssdfs_crc32_le(extent->buf, bytes_count);
}

static void
prepare_payload_metadata_descriptor(struct ssdfs_volume_layout *layout,
				    struct ssdfs_extent_desc *extent,
				    struct ssdfs_metadata_descriptor *desc)
{
	BUG_ON(!layout || !extent || !desc);
	BUG_ON(extent->bytes_count == 0 ||
		extent->bytes_count >= layout->env.erase_size);
	BUG_ON(extent->offset == 0 ||
		extent->offset >= layout->env.erase_size);

	desc->offset = cpu_to_le32(extent->offset);
	desc->size = cpu_to_le32(extent->bytes_count);

	BUG_ON(extent->bytes_count >= U16_MAX);

	desc->check.bytes = cpu_to_le16((u16)extent->bytes_count);
	desc->check.flags = cpu_to_le16(SSDFS_CRC32);
	desc->check.csum = 0;
	desc->check.csum = ssdfs_crc32_le(extent->buf, extent->bytes_count);
}

static void
prepare_maptbl_cache_metadata_descriptor(struct ssdfs_volume_layout *layout,
					 struct ssdfs_extent_desc *extent,
					 struct ssdfs_metadata_descriptor *desc)
{
	struct ssdfs_maptbl_cache_header *hdr;
	u32 fragments;
	size_t fragment_size;
	u32 bytes_count = 0;
	u32 csum = 0;
	u32 i;

	BUG_ON(!layout || !extent || !desc);
	BUG_ON(extent->bytes_count == 0 ||
		extent->bytes_count >= layout->env.erase_size);
	BUG_ON(extent->offset == 0 ||
		extent->offset >= layout->env.erase_size);
	BUG_ON(!extent->buf);

	desc->offset = cpu_to_le32(extent->offset);
	desc->size = cpu_to_le32(extent->bytes_count);

	BUG_ON(extent->bytes_count >= U16_MAX);

	fragments = layout->maptbl_cache.fragments_count;
	fragment_size = layout->maptbl_cache.fragment_size;

	for (i = 0; i < fragments; i++) {
		u8 *ptr = (u8 *)extent->buf + (i * fragment_size);
		u16 size;

		BUG_ON(!ptr);
		hdr = (struct ssdfs_maptbl_cache_header *)ptr;

		size = le16_to_cpu(hdr->bytes_count);
		csum = crc32(csum, ptr, size);

		bytes_count += size;
	}

	BUG_ON(bytes_count >= U16_MAX);

	desc->check.bytes = cpu_to_le16((u16)bytes_count);
	desc->check.flags = cpu_to_le16(SSDFS_CRC32);
	desc->check.csum = cpu_to_le32(~csum);
}

static
void prepare_footer_metadata_descriptor(struct ssdfs_volume_layout *layout,
					struct ssdfs_extent_desc *extent,
					struct ssdfs_metadata_descriptor *desc)
{
	struct ssdfs_log_footer *footer;

	BUG_ON(!layout || !extent || !desc);
	BUG_ON(extent->bytes_count == 0 ||
		extent->bytes_count >= layout->env.erase_size);
	BUG_ON(extent->offset == 0 ||
		extent->offset >= layout->env.erase_size);

	desc->offset = cpu_to_le32(extent->offset);
	desc->size = cpu_to_le32(extent->bytes_count);

	footer = (struct ssdfs_log_footer *)extent->buf;

	memcpy(&desc->check, &footer->volume_state.check,
		sizeof(struct ssdfs_metadata_check));
}

void commit_segment_header(struct ssdfs_volume_layout *layout,
			   int seg_index, int peb_index,
			   u32 blks_count)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *hdr_ext, *extent;
	struct ssdfs_metadata_descriptor *meta_desc;
	struct ssdfs_segment_header *hdr;
	size_t hdr_len = sizeof(struct ssdfs_segment_header);
	char *begin_ptr;
	u16 log_pages;
	u32 seg_flags = 0;

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, blks_count %u\n",
		  seg_index, blks_count);

	seg_desc = &layout->segs[seg_index];
	peb_desc = &seg_desc->pebs[peb_index];
	hdr_ext = &peb_desc->extents[SSDFS_SEG_HEADER];

	BUG_ON(!hdr_ext->buf);
	BUG_ON(blks_count >= USHRT_MAX);

	begin_ptr = hdr_ext->buf;
	hdr = (struct ssdfs_segment_header *)begin_ptr;

	switch (seg_desc->seg_type) {
	case SSDFS_INITIAL_SNAPSHOT:
		/* do nothing */
		break;

	case SSDFS_SUPERBLOCK:
		log_pages = layout->sb.log_pages;
		BUG_ON(log_pages == 0);
		BUG_ON(log_pages == U16_MAX);
		BUG_ON(log_pages != blks_count);
		break;

	case SSDFS_SEGBMAP:
		log_pages = layout->segbmap.log_pages;
		BUG_ON(log_pages == 0);
		BUG_ON(log_pages == U16_MAX);
		if (log_pages != blks_count)
			seg_flags |= SSDFS_LOG_IS_PARTIAL;
		break;

	case SSDFS_PEB_MAPPING_TABLE:
		log_pages = layout->maptbl.log_pages;
		BUG_ON(log_pages == 0);
		BUG_ON(log_pages == U16_MAX);
		if (log_pages != blks_count)
			seg_flags |= SSDFS_LOG_IS_PARTIAL;
		break;

	default:
		SSDFS_WARN("unprocessed type of segment: %#x\n",
			   seg_index);
	}

	hdr->log_pages = cpu_to_le16((u16)blks_count);

	if (peb_desc->extents[SSDFS_BLOCK_BITMAP].bytes_count > 0) {
		extent = &peb_desc->extents[SSDFS_BLOCK_BITMAP];
		meta_desc = &hdr->desc_array[SSDFS_BLK_BMAP_INDEX];
		prepare_blkbmap_metadata_descriptor(layout, extent, meta_desc);
		seg_flags |= SSDFS_SEG_HDR_HAS_BLK_BMAP;
	}

	if (peb_desc->extents[SSDFS_OFFSET_TABLE].bytes_count > 0) {
		extent = &peb_desc->extents[SSDFS_OFFSET_TABLE];
		meta_desc = &hdr->desc_array[SSDFS_OFF_TABLE_INDEX];
		prepare_offset_table_metadata_descriptor(layout, extent,
							 meta_desc);
		seg_flags |= SSDFS_SEG_HDR_HAS_OFFSET_TABLE;
	}

	if (peb_desc->extents[SSDFS_BLOCK_DESCRIPTORS].bytes_count > 0) {
		extent = &peb_desc->extents[SSDFS_BLOCK_DESCRIPTORS];
		meta_desc = &hdr->desc_array[SSDFS_BLK_DESC_AREA_INDEX];
		prepare_blk_desc_table_metadata_descriptor(layout, extent,
							    meta_desc);
		seg_flags |= SSDFS_LOG_HAS_BLK_DESC_CHAIN;
	}

	if (peb_desc->extents[SSDFS_MAPTBL_CACHE].bytes_count > 0) {

		if (seg_desc->seg_type != SSDFS_SUPERBLOCK) {
			SSDFS_ERR("sb segment should have maptbl cache\n");
			BUG();
		}

		extent = &peb_desc->extents[SSDFS_MAPTBL_CACHE];
		meta_desc = &hdr->desc_array[SSDFS_MAPTBL_CACHE_INDEX];
		prepare_maptbl_cache_metadata_descriptor(layout, extent,
							 meta_desc);
		seg_flags |= SSDFS_LOG_HAS_MAPTBL_CACHE;
	}

	if (peb_desc->extents[SSDFS_LOG_PAYLOAD].bytes_count > 0) {
		extent = &peb_desc->extents[SSDFS_LOG_PAYLOAD];
		meta_desc = &hdr->desc_array[SSDFS_COLD_PAYLOAD_AREA_INDEX];
		prepare_payload_metadata_descriptor(layout, extent,
						    meta_desc);
		seg_flags |= SSDFS_LOG_HAS_COLD_PAYLOAD;
	}

	if (peb_desc->extents[SSDFS_LOG_FOOTER].bytes_count > 0) {
		extent = &peb_desc->extents[SSDFS_LOG_FOOTER];
		meta_desc = &hdr->desc_array[SSDFS_LOG_FOOTER_INDEX];
		prepare_footer_metadata_descriptor(layout, extent, meta_desc);
		seg_flags |= SSDFS_LOG_HAS_FOOTER;
	}

	hdr->seg_flags = cpu_to_le32(seg_flags);

	hdr->volume_hdr.check.bytes = cpu_to_le16(hdr_len);
	hdr->volume_hdr.check.flags = cpu_to_le16(SSDFS_CRC32);
	hdr->volume_hdr.check.csum = 0;
	hdr->volume_hdr.check.csum = ssdfs_crc32_le(hdr, hdr_len);
}

static void set_blkbmap_compression_flag(struct ssdfs_volume_layout *layout)
{
	u64 feature_compat_ro = le64_to_cpu(layout->sb.vs.feature_compat_ro);

	if (layout->blkbmap.compression == SSDFS_ZLIB_BLOB)
		feature_compat_ro |= SSDFS_ZLIB_COMPAT_RO_FLAG;
	else if (layout->blkbmap.compression == SSDFS_LZO_BLOB)
		feature_compat_ro |= SSDFS_LZO_COMPAT_RO_FLAG;

	layout->sb.vs.feature_compat_ro = cpu_to_le64(feature_compat_ro);
}

static int ssdfs_fragment_descriptor_init(struct ssdfs_fragment_desc *desc,
					  u8 *fragment, u32 offset,
					  u16 compr_size, u16 uncompr_size,
					  u8 sequence_id, int type, int flags)
{
	BUG_ON(!desc || !fragment);

	if (compr_size == 0 || uncompr_size == 0 ||
	    compr_size > uncompr_size) {
		SSDFS_ERR("invalid size: compr_size %u, uncompr_size %u\n",
			  compr_size, uncompr_size);
		return -EINVAL;
	}

	if (type < SSDFS_FRAGMENT_UNCOMPR_BLOB ||
	    type > SSDFS_FRAGMENT_LZO_BLOB) {
		SSDFS_ERR("invalid type %#x\n", type);
		return -EINVAL;
	}

	if (flags & ~SSDFS_FRAGMENT_DESC_FLAGS_MASK) {
		SSDFS_ERR("unknown flags %#x\n", flags);
		return -EINVAL;
	}

	desc->magic = SSDFS_FRAGMENT_DESC_MAGIC;

	/* TODO: temporary compression doesn't supported */
	BUG_ON(type != SSDFS_FRAGMENT_UNCOMPR_BLOB);
	desc->type = cpu_to_le8((u8)type);

	desc->flags = cpu_to_le8((u8)flags);
	desc->sequence_id = cpu_to_le8(sequence_id);

	desc->offset = cpu_to_le32(offset);
	desc->compr_size = cpu_to_le16(compr_size);
	desc->uncompr_size = cpu_to_le16(uncompr_size);

	if (flags & SSDFS_FRAGMENT_HAS_CSUM)
		desc->checksum = ssdfs_crc32_le(fragment, compr_size);

	return 0;
}

static int __pre_commit_block_bitmap(struct ssdfs_volume_layout *layout,
				     struct ssdfs_extent_desc *extent,
				     int peb_index,
				     u16 valid_blks)
{
	struct ssdfs_block_bitmap_header *bmp_hdr;
	struct ssdfs_block_bitmap_fragment *bmp_frag_hdr;
	u8 *bmap;
	size_t bmp_hdr_size = sizeof(struct ssdfs_block_bitmap_header);
	size_t bmp_frag_hdr_size = sizeof(struct ssdfs_block_bitmap_fragment);
	size_t frag_desc_size = sizeof(struct ssdfs_fragment_desc);
	u32 pages_per_peb;
	u32 allocation_size;
	u32 bmap_bytes, written_bmap_bytes;
	u32 fragments_count;
	u32 bmap_offset;
	u32 fragment_offset;
	u8 flags = 0;
	u8 type = SSDFS_BLK_BMAP_UNCOMPRESSED_BLOB;
	u32 i;
	int err;

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, extent %p, "
		  "valid_blks %u\n",
		  layout, extent, valid_blks);

	BUG_ON(!layout || !extent);
	BUG_ON(extent->buf);

	BUG_ON(layout->page_size == 0);
	BUG_ON(layout->page_size > layout->env.erase_size);
	pages_per_peb = layout->env.erase_size / layout->page_size;

	if (valid_blks > pages_per_peb) {
		SSDFS_ERR("valid_blks %u > pages_per_peb %u\n",
			  valid_blks, pages_per_peb);
		return -EINVAL;
	}

	bmap_bytes = BLK_BMAP_BYTES(pages_per_peb);

	fragments_count = bmap_bytes + PAGE_CACHE_SIZE - 1;
	fragments_count >>= PAGE_CACHE_SHIFT;
	BUG_ON(fragments_count >= U16_MAX);
	BUG_ON(fragments_count > SSDFS_FRAGMENTS_CHAIN_MAX);

	allocation_size = bmap_bytes + bmp_hdr_size + bmp_frag_hdr_size;
	allocation_size += fragments_count * frag_desc_size;

	extent->buf = calloc(1, allocation_size);
	if (!extent->buf) {
		SSDFS_ERR("unable to allocate memory of size %u\n",
			  allocation_size);
		return -ENOMEM;
	}

	bmp_hdr = (struct ssdfs_block_bitmap_header *)extent->buf;

	bmp_hdr->magic.common = cpu_to_le32(SSDFS_SUPER_MAGIC);
	bmp_hdr->magic.key = cpu_to_le16(SSDFS_BLK_BMAP_MAGIC);
	bmp_hdr->magic.version.major = cpu_to_le8(SSDFS_MAJOR_REVISION);
	bmp_hdr->magic.version.minor = cpu_to_le8(SSDFS_MINOR_REVISION);

	bmp_hdr->fragments_count = cpu_to_le16(1);
	bmp_hdr->bytes_count = cpu_to_le32(allocation_size);

	if (layout->blkbmap.has_backup_copy)
		flags |= SSDFS_BLK_BMAP_BACKUP;

	switch (layout->blkbmap.compression) {
	case SSDFS_UNCOMPRESSED_BLOB:
		type = SSDFS_BLK_BMAP_UNCOMPRESSED_BLOB;
		break;

	case SSDFS_ZLIB_BLOB:
		flags |= SSDFS_BLK_BMAP_COMPRESSED;
		type = SSDFS_BLK_BMAP_ZLIB_BLOB;
		/* TODO: temporary compression doesn't supported */
		BUG();
		break;

	case SSDFS_LZO_BLOB:
		flags |= SSDFS_BLK_BMAP_COMPRESSED;
		type = SSDFS_BLK_BMAP_LZO_BLOB;
		/* TODO: temporary compression doesn't supported */
		BUG();
		break;

	default:
		BUG();
	}

	bmp_hdr->flags = cpu_to_le8(flags);
	bmp_hdr->type = cpu_to_le8(type);

	bmap_offset = bmp_hdr_size;
	bmap_offset += bmp_frag_hdr_size;
	bmap_offset += fragments_count * frag_desc_size;

	bmap = (u8 *)extent->buf + bmap_offset;

	err = ssdfs_blkbmap_set_area(bmap, 0, valid_blks, SSDFS_BLK_VALID);
	if (err) {
		SSDFS_ERR("fail to set block bitmap: "
			  "valid_blks %u, err %d\n",
			  valid_blks, err);
		goto fail_pre_commit_blk_bmap;
	}

	bmp_frag_hdr =
		(struct ssdfs_block_bitmap_fragment *)((u8 *)extent->buf +
							bmp_hdr_size);

	bmp_frag_hdr->peb_index = cpu_to_le16(peb_index);
	bmp_frag_hdr->sequence_id = 0;
	bmp_frag_hdr->flags = 0;
	bmp_frag_hdr->type = cpu_to_le16(SSDFS_SRC_BLK_BMAP);
	bmp_frag_hdr->last_free_blk = cpu_to_le16(valid_blks);
	bmp_frag_hdr->invalid_blks = 0;

	bmp_frag_hdr->chain_hdr.magic = cpu_to_le8(SSDFS_CHAIN_HDR_MAGIC);
	bmp_frag_hdr->chain_hdr.type = cpu_to_le8(SSDFS_BLK_BMAP_CHAIN_HDR);
	bmp_frag_hdr->chain_hdr.flags = 0;
	bmp_frag_hdr->chain_hdr.desc_size =
		cpu_to_le16(sizeof(struct ssdfs_fragment_desc));

	bmp_frag_hdr->chain_hdr.fragments_count =
				cpu_to_le16((u16)fragments_count);

	/* TODO: temporary compression doesn't supported */
	bmp_frag_hdr->chain_hdr.compr_bytes = cpu_to_le32(bmap_bytes);
	bmp_frag_hdr->chain_hdr.uncompr_bytes = cpu_to_le32(bmap_bytes);

	fragment_offset = bmp_hdr_size;
	fragment_offset += bmp_frag_hdr_size;
	fragment_offset += fragments_count * frag_desc_size;

	written_bmap_bytes = 0;

	for (i = 0; i < fragments_count; i++) {
		u8 *fragment;
		struct ssdfs_fragment_desc *cur_desc;
		u32 desc_offset;
		u32 fragment_size;

		desc_offset = bmp_hdr_size;
		desc_offset += bmp_frag_hdr_size;
		desc_offset += i * frag_desc_size;

		cur_desc = (struct ssdfs_fragment_desc *)((u8 *)extent->buf +
								desc_offset);

		fragment = bmap + (i * PAGE_CACHE_SIZE);
		fragment_offset += i * PAGE_CACHE_SIZE;

		BUG_ON(bmap_bytes <= written_bmap_bytes);
		fragment_size = min_t(u32, bmap_bytes - written_bmap_bytes,
					(u32)PAGE_CACHE_SIZE);
		BUG_ON(fragment_size >= U16_MAX);

		BUG_ON(i >= U8_MAX);

		err = ssdfs_fragment_descriptor_init(cur_desc, fragment,
						    fragment_offset,
						    (u16)fragment_size,
						    (u16)fragment_size,
						    (u8)i,
						    SSDFS_FRAGMENT_UNCOMPR_BLOB,
						    SSDFS_FRAGMENT_HAS_CSUM);
		if (err) {
			SSDFS_ERR("fail to init fragment descriptor: "
				  "fragment_index %u, err %d\n",
				  i, err);
			goto fail_pre_commit_blk_bmap;
		}

		written_bmap_bytes += fragment_size;
	}

	set_blkbmap_compression_flag(layout);

	extent->bytes_count = allocation_size;

	return 0;

fail_pre_commit_blk_bmap:
	free(extent->buf);
	extent->buf = NULL;

	return err;
}

int pre_commit_block_bitmap(struct ssdfs_volume_layout *layout,
			    int seg_index, int peb_index,
			    u16 valid_blks)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, peb_index %d, "
		  "valid_blks %u\n",
		  seg_index, peb_index, valid_blks);

	if (seg_index >= layout->segs_capacity) {
		SSDFS_ERR("seg_index %d >= segs_capacity %d\n",
			  seg_index, layout->segs_capacity);
		return -EINVAL;
	}

	seg_desc = &layout->segs[seg_index];

	if (peb_index >= seg_desc->pebs_capacity) {
		SSDFS_ERR("peb_index %d >= pebs_capacity %d\n",
			  peb_index, seg_desc->pebs_capacity);
		return -EINVAL;
	}

	peb_desc = &seg_desc->pebs[peb_index];
	extent = &peb_desc->extents[SSDFS_BLOCK_BITMAP];

	return __pre_commit_block_bitmap(layout, extent,
					 peb_index, valid_blks);
}

int pre_commit_block_bitmap_backup(struct ssdfs_volume_layout *layout,
				   int seg_index, int peb_index,
				   u16 valid_blks)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, peb_index %d, "
		  "valid_blks %u\n",
		  seg_index, peb_index, valid_blks);

	if (seg_index >= layout->segs_capacity) {
		SSDFS_ERR("seg_index %d >= segs_capacity %d\n",
			  seg_index, layout->segs_capacity);
		return -EINVAL;
	}

	seg_desc = &layout->segs[seg_index];

	if (peb_index >= seg_desc->pebs_capacity) {
		SSDFS_ERR("peb_index %d >= pebs_capacity %d\n",
			  peb_index, seg_desc->pebs_capacity);
		return -EINVAL;
	}

	peb_desc = &seg_desc->pebs[peb_index];
	extent = &peb_desc->extents[SSDFS_BLOCK_BITMAP_BACKUP];

	return __pre_commit_block_bitmap(layout, extent,
					 peb_index, valid_blks);
}

static void __commit_block_bitmap(struct ssdfs_volume_layout *layout,
				  struct ssdfs_extent_desc *extent,
				  u16 metadata_blks)
{
	struct ssdfs_block_bitmap_fragment *bmp_frag_hdr;
	size_t bmp_hdr_size = sizeof(struct ssdfs_block_bitmap_header);

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, extent %p, "
		  "metadata_blks %u\n",
		  layout, extent, metadata_blks);

	BUG_ON(!layout || !extent);
	BUG_ON(!extent->buf);

	bmp_frag_hdr =
		(struct ssdfs_block_bitmap_fragment *)((u8 *)extent->buf +
							bmp_hdr_size);

	BUG_ON(metadata_blks >= (layout->env.erase_size / layout->page_size));

	bmp_frag_hdr->metadata_blks = cpu_to_le16(metadata_blks);
}

void commit_block_bitmap(struct ssdfs_volume_layout *layout,
			 int seg_index, int peb_index,
			 u16 metadata_blks)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, peb_index %d, "
		  "metadata_blks %u\n",
		  seg_index, peb_index, metadata_blks);

	seg_desc = &layout->segs[seg_index];
	peb_desc = &seg_desc->pebs[peb_index];
	extent = &peb_desc->extents[SSDFS_BLOCK_BITMAP];

	__commit_block_bitmap(layout, extent, metadata_blks);
}

void commit_block_bitmap_backup(struct ssdfs_volume_layout *layout,
				int seg_index, int peb_index,
				u16 metadata_blks)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, peb_index %d, "
		  "metadata_blks %u\n",
		  seg_index, peb_index, metadata_blks);

	seg_desc = &layout->segs[seg_index];
	peb_desc = &seg_desc->pebs[peb_index];
	extent = &peb_desc->extents[SSDFS_BLOCK_BITMAP_BACKUP];

	__commit_block_bitmap(layout, extent, metadata_blks);
}

static u16 calculate_offset_table_fragments(u16 valid_blks)
{
	u32 offsets_per_fragment;
	u32 fragments_count;

	offsets_per_fragment = BLK2OFF_DESC_PER_FRAGMENT();

	fragments_count = valid_blks + offsets_per_fragment - 1;
	fragments_count /= offsets_per_fragment;
	BUG_ON(fragments_count >= U16_MAX);

	return (u16)fragments_count;
}

static inline u32 define_block_descriptor_offset(u16 blk_id,
						 u16 fragments_count)
{
	size_t blk_desc_size = sizeof(struct ssdfs_block_descriptor);

	return (u32)BLK_DESC_HEADER_SIZE(fragments_count) +
		((u32)blk_id * blk_desc_size);
}

static void prepare_offsets_table_fragment(u8 *fragment, int peb_index,
					   u16 sequence_id, u8 area_type,
					   u32 logical_start_page,
					   u16 start_id, u16 valid_blks,
					   u16 rest_blks, u16 *processed_blks)
{
	struct ssdfs_phys_offset_table_header *hdr;
	size_t hdr_size = sizeof(struct ssdfs_phys_offset_table_header);
	struct ssdfs_phys_offset_descriptor *offsets;
	size_t item_size = sizeof(struct ssdfs_phys_offset_descriptor);
	u16 id_count, free_items;
	u16 blk_desc_fragments;
	u32 byte_size;
	u16 flags = 0;
	int i;

	BUG_ON(!fragment || !processed_blks);
	BUG_ON(valid_blks == 0);
	BUG_ON(area_type >= SSDFS_LOG_AREA_MAX);
	BUG_ON(peb_index >= U16_MAX);

	hdr = (struct ssdfs_phys_offset_table_header *)fragment;
	offsets = (struct ssdfs_phys_offset_descriptor *)(fragment + hdr_size);
	*processed_blks = 0;
	id_count = min_t(u16, rest_blks, BLK2OFF_DESC_PER_FRAGMENT());
	blk_desc_fragments = BLK_DESC_TABLE_FRAGMENTS(valid_blks);

	for (i = 0; i < id_count; ++i, ++*processed_blks) {
		u16 blk_id = start_id + i;
		u32 byte_offset = define_block_descriptor_offset(blk_id,
							blk_desc_fragments);
		u32 peb_page = logical_start_page + i;

		offsets[i].page_desc.logical_offset = cpu_to_le32(peb_page);
		offsets[i].page_desc.peb_index = cpu_to_le16((u16)peb_index);
		offsets[i].page_desc.peb_page = cpu_to_le16(blk_id);

		offsets[i].blk_state.log_start_page = 0;
		offsets[i].blk_state.log_area = cpu_to_le8(area_type);
		offsets[i].blk_state.peb_migration_id =
					SSDFS_PEB_MIGRATION_ID_START;
		offsets[i].blk_state.byte_offset = cpu_to_le32(byte_offset);
	}

	hdr->magic = cpu_to_le32(SSDFS_PHYS_OFF_TABLE_MAGIC);
	hdr->start_id = cpu_to_le16(start_id);
	hdr->id_count = cpu_to_le16(id_count);

	byte_size = hdr_size + (id_count * item_size);
	hdr->byte_size = cpu_to_le32(byte_size);

	hdr->peb_index = cpu_to_le16((u16)peb_index);
	hdr->sequence_id = cpu_to_le16(sequence_id);
	hdr->type = cpu_to_le16(SSDFS_SEG_OFF_TABLE);

	flags |= SSDFS_OFF_TABLE_HAS_CSUM;
	if (*processed_blks < rest_blks)
		flags |= SSDFS_OFF_TABLE_HAS_NEXT_FRAGMENT;
	hdr->flags = cpu_to_le16(flags);

	free_items = BLK2OFF_DESC_PER_FRAGMENT() - id_count;
	hdr->used_logical_blks = cpu_to_le16(id_count);
	hdr->free_logical_blks = cpu_to_le16(free_items);
	hdr->last_allocated_blk = cpu_to_le16(id_count - 1);

	BUG_ON(byte_size >= U16_MAX);

	if (flags & SSDFS_OFF_TABLE_HAS_NEXT_FRAGMENT)
		hdr->next_fragment_off = cpu_to_le16((u16)byte_size);
	else
		hdr->next_fragment_off = cpu_to_le16(U16_MAX);

	hdr->checksum = 0;
	hdr->checksum = ssdfs_crc32_le(fragment, byte_size);
}

static int __pre_commit_offset_table(struct ssdfs_volume_layout *layout,
				     int peb_index,
				     struct ssdfs_extent_desc *extent,
				     u64 logical_byte_offset,
				     u16 valid_blks)
{
	struct ssdfs_blk2off_table_header *tbl_hdr;
	size_t tbl_hdr_size = sizeof(struct ssdfs_blk2off_table_header);
	u32 pages_per_peb;
	u16 fragments_count;
	u32 allocation_size;
	u16 offset;
	u16 start_id;
	u16 rest_blks;
	u64 logical_start_page;
	u16 i;

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, peb_index %d, extent %p, valid_blks %u\n",
		  layout, peb_index, extent, valid_blks);

	BUG_ON(!layout || !extent);
	BUG_ON(extent->buf);

	BUG_ON(layout->page_size == 0);
	BUG_ON(layout->page_size > layout->env.erase_size);
	pages_per_peb = layout->env.erase_size / layout->page_size;

	if (valid_blks > pages_per_peb) {
		SSDFS_ERR("valid_blks %u > pages_per_peb %u\n",
			  valid_blks, pages_per_peb);
		return -EINVAL;
	}

	fragments_count = calculate_offset_table_fragments(valid_blks);
	allocation_size = tbl_hdr_size + (fragments_count << PAGE_CACHE_SHIFT);

	extent->buf = calloc(1, allocation_size);
	if (!extent->buf) {
		SSDFS_ERR("unable to allocate memory of size %u\n",
			  allocation_size);
		return -ENOMEM;
	}

	tbl_hdr = (struct ssdfs_blk2off_table_header *)extent->buf;

	tbl_hdr->magic.common = cpu_to_le32(SSDFS_SUPER_MAGIC);
	tbl_hdr->magic.key = cpu_to_le16(SSDFS_BLK2OFF_TABLE_HDR_MAGIC);
	tbl_hdr->magic.version.major = cpu_to_le8(SSDFS_MAJOR_REVISION);
	tbl_hdr->magic.version.minor = cpu_to_le8(SSDFS_MINOR_REVISION);

	offset = offsetof(struct ssdfs_blk2off_table_header, sequence);
	tbl_hdr->extents_off = cpu_to_le16(offset);
	tbl_hdr->extents_count = cpu_to_le16(1);
	tbl_hdr->offset_table_off = cpu_to_le16(tbl_hdr_size);
	tbl_hdr->fragments_count = cpu_to_le16(fragments_count);

	tbl_hdr->sequence[0].logical_blk = 0;
	tbl_hdr->sequence[0].offset_id = 0;
	tbl_hdr->sequence[0].len = cpu_to_le16(valid_blks);
	tbl_hdr->sequence[0].sequence_id = 0;
	tbl_hdr->sequence[0].state = cpu_to_le8(SSDFS_LOGICAL_BLK_USED);

	start_id = 0;
	rest_blks = valid_blks;
	logical_start_page = logical_byte_offset / layout->page_size;
	BUG_ON(logical_start_page >= U32_MAX);
	for (i = 0; i < fragments_count; i++) {
		u8 *fragment;
		u16 processed_blks;

		fragment = (u8 *)extent->buf + tbl_hdr_size +
				(PAGE_CACHE_SIZE * i);

		prepare_offsets_table_fragment(fragment, peb_index, i,
						SSDFS_LOG_BLK_DESC_AREA,
						(u32)logical_start_page,
						start_id, valid_blks,
						rest_blks, &processed_blks);

		start_id += processed_blks;
		rest_blks -= processed_blks;
		logical_start_page += processed_blks;
	}

	extent->bytes_count = allocation_size;

	return 0;
}

int pre_commit_offset_table(struct ssdfs_volume_layout *layout,
			    int seg_index, int peb_index,
			    u64 logical_byte_offset,
			    u16 valid_blks)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, peb_index %d, "
		  "valid_blks %u\n",
		  seg_index, peb_index, valid_blks);

	if (seg_index >= layout->segs_capacity) {
		SSDFS_ERR("seg_index %d >= segs_capacity %d\n",
			  seg_index, layout->segs_capacity);
		return -EINVAL;
	}

	seg_desc = &layout->segs[seg_index];

	if (peb_index >= seg_desc->pebs_capacity) {
		SSDFS_ERR("peb_index %d >= pebs_capacity %d\n",
			  peb_index, seg_desc->pebs_capacity);
		return -EINVAL;
	}

	peb_desc = &seg_desc->pebs[peb_index];
	extent = &peb_desc->extents[SSDFS_OFFSET_TABLE];

	return __pre_commit_offset_table(layout, peb_index, extent,
					 logical_byte_offset, valid_blks);
}

int pre_commit_offset_table_backup(struct ssdfs_volume_layout *layout,
				   int seg_index, int peb_index,
				   u64 logical_byte_offset,
				   u16 valid_blks)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, peb_index %d, "
		  "valid_blks %u\n",
		  seg_index, peb_index, valid_blks);

	if (seg_index >= layout->segs_capacity) {
		SSDFS_ERR("seg_index %d >= segs_capacity %d\n",
			  seg_index, layout->segs_capacity);
		return -EINVAL;
	}

	seg_desc = &layout->segs[seg_index];

	if (peb_index >= seg_desc->pebs_capacity) {
		SSDFS_ERR("peb_index %d >= pebs_capacity %d\n",
			  peb_index, seg_desc->pebs_capacity);
		return -EINVAL;
	}

	peb_desc = &seg_desc->pebs[peb_index];
	extent = &peb_desc->extents[SSDFS_OFFSET_TABLE_BACKUP];

	return __pre_commit_offset_table(layout, peb_index, extent,
					 logical_byte_offset, valid_blks);
}

static void __commit_offset_table(struct ssdfs_volume_layout *layout,
				  struct ssdfs_extent_desc *extent)
{
	struct ssdfs_blk2off_table_header *tbl_hdr;
	u16 bytes_count;

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, extent %p\n",
		  layout, extent);

	BUG_ON(!layout || !extent);
	BUG_ON(!extent->buf);

	tbl_hdr = (struct ssdfs_blk2off_table_header *)extent->buf;

	tbl_hdr->check.bytes = tbl_hdr->offset_table_off;
	tbl_hdr->check.flags = cpu_to_le16(SSDFS_CRC32);

	bytes_count = le16_to_cpu(tbl_hdr->offset_table_off);
	tbl_hdr->check.csum = 0;
	tbl_hdr->check.csum = ssdfs_crc32_le(extent->buf, bytes_count);
}

void commit_offset_table(struct ssdfs_volume_layout *layout,
			 int seg_index, int peb_index)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, peb_index %d\n",
		  seg_index, peb_index);

	seg_desc = &layout->segs[seg_index];
	peb_desc = &seg_desc->pebs[peb_index];
	extent = &peb_desc->extents[SSDFS_OFFSET_TABLE];

	__commit_offset_table(layout, extent);
}

void commit_offset_table_backup(struct ssdfs_volume_layout *layout,
				int seg_index, int peb_index)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, peb_index %d\n",
		  seg_index, peb_index);

	seg_desc = &layout->segs[seg_index];
	peb_desc = &seg_desc->pebs[peb_index];
	extent = &peb_desc->extents[SSDFS_OFFSET_TABLE_BACKUP];

	__commit_offset_table(layout, extent);
}

int pre_commit_block_descriptors(struct ssdfs_volume_layout *layout,
				 int seg_index, int peb_index,
				 u16 valid_blks, u64 inode_id,
				 u32 payload_offset_in_bytes,
				 u32 item_size)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;
	struct ssdfs_block_state_descriptor *blk_state_hdr;
	u32 pages_per_peb;
	u16 fragments_count;
	u32 allocation_size;
	u32 bytes_count;
	u32 offset;
	u16 rest_items;
	u8 area_type = SSDFS_LOG_MAIN_AREA;
	u32 cur_byte_offset = 0;
	u16 i, j;

	BUG_ON(!layout);

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, peb_index %d, "
		  "valid_blks %u, inode_id %llu, "
		  "payload_offset_in_bytes %u, item_size %u\n",
		  seg_index, peb_index, valid_blks, inode_id,
		  payload_offset_in_bytes, item_size);

	if (seg_index >= layout->segs_capacity) {
		SSDFS_ERR("seg_index %d >= segs_capacity %d\n",
			  seg_index, layout->segs_capacity);
		return -EINVAL;
	}

	seg_desc = &layout->segs[seg_index];

	if (peb_index >= seg_desc->pebs_capacity) {
		SSDFS_ERR("peb_index %d >= pebs_capacity %d\n",
			  peb_index, seg_desc->pebs_capacity);
		return -EINVAL;
	}

	peb_desc = &seg_desc->pebs[peb_index];
	extent = &peb_desc->extents[SSDFS_BLOCK_DESCRIPTORS];

	BUG_ON(extent->buf);

	BUG_ON(layout->page_size == 0);
	BUG_ON(layout->page_size > layout->env.erase_size);
	pages_per_peb = layout->env.erase_size / layout->page_size;

	if (valid_blks > pages_per_peb) {
		SSDFS_ERR("valid_blks %u > pages_per_peb %u\n",
			  valid_blks, pages_per_peb);
		return -EINVAL;
	}

	fragments_count = BLK_DESC_TABLE_FRAGMENTS(valid_blks);
	allocation_size = BLK_DESC_HEADER_SIZE(fragments_count) +
				(fragments_count << PAGE_CACHE_SHIFT);

	extent->buf = calloc(1, allocation_size);
	if (!extent->buf) {
		SSDFS_ERR("unable to allocate memory of size %u\n",
			  allocation_size);
		return -ENOMEM;
	}

	blk_state_hdr = (struct ssdfs_block_state_descriptor *)extent->buf;

	blk_state_hdr->cno = cpu_to_le64(layout->create_cno);
	blk_state_hdr->parent_snapshot = 0;

	blk_state_hdr->chain_hdr.magic = cpu_to_le8(SSDFS_CHAIN_HDR_MAGIC);
	blk_state_hdr->chain_hdr.type = cpu_to_le8(SSDFS_BLK_STATE_CHAIN_HDR);
	blk_state_hdr->chain_hdr.flags = 0;
	blk_state_hdr->chain_hdr.fragments_count = cpu_to_le16(fragments_count);
	blk_state_hdr->chain_hdr.desc_size =
				cpu_to_le16(sizeof(struct ssdfs_fragment_desc));

	bytes_count = BLK_DESC_HEADER_SIZE(fragments_count);
	bytes_count += valid_blks * sizeof(struct ssdfs_block_descriptor);

	blk_state_hdr->chain_hdr.compr_bytes = cpu_to_le32(bytes_count);
	blk_state_hdr->chain_hdr.uncompr_bytes = cpu_to_le32(bytes_count);

	rest_items = valid_blks;

	if (item_size >= layout->page_size)
		area_type = SSDFS_LOG_MAIN_AREA;
	else
		area_type = SSDFS_LOG_JOURNAL_AREA;

	for (i = 0; i < fragments_count; i++) {
		struct ssdfs_block_descriptor *array;
		struct ssdfs_fragment_desc *frg_desc;
		u16 items_count;
		u32 fragment_offset;

		offset = BLK_DESC_HEADER_SIZE(fragments_count);
		offset += PAGE_CACHE_SIZE * i;
		fragment_offset = offset;

		array = (struct ssdfs_block_descriptor *)(extent->buf + offset);

		items_count = min_t(u16, BLK_STATE_PER_FRAGMENT(), rest_items);

		for (j = 0; j < items_count; j++) {
			struct ssdfs_block_descriptor *blk_desc = &array[j];
			u32 logical_page;
			u32 peb_page;

			blk_desc->ino = cpu_to_le64(inode_id);
			BUG_ON(peb_index >= U16_MAX);
			blk_desc->peb_index = cpu_to_le16(peb_index);

			logical_page = payload_offset_in_bytes;
			logical_page += cur_byte_offset;
			logical_page /= layout->page_size;
			blk_desc->logical_offset = cpu_to_le32(logical_page);

			peb_page = cur_byte_offset / layout->page_size;
			BUG_ON(peb_page >= U16_MAX);
			blk_desc->peb_page = cpu_to_le16((u16)peb_page);

			blk_desc->state[0].log_start_page = 0;
			blk_desc->state[0].log_area = cpu_to_le8(area_type);
			blk_desc->state[0].peb_migration_id =
					SSDFS_PEB_MIGRATION_ID_START;
			blk_desc->state[0].byte_offset =
					cpu_to_le32(cur_byte_offset);

			cur_byte_offset += item_size;
		}

		offset = sizeof(struct ssdfs_block_state_descriptor);
		offset += i * sizeof(struct ssdfs_fragment_desc);

		frg_desc = (struct ssdfs_fragment_desc *)(extent->buf + offset);

		frg_desc->magic = cpu_to_le8(SSDFS_FRAGMENT_DESC_MAGIC);
		frg_desc->type = cpu_to_le8(SSDFS_DATA_BLK_STATE_DESC);
		frg_desc->flags = cpu_to_le8(SSDFS_FRAGMENT_HAS_CSUM);
		BUG_ON(i >= U8_MAX);
		frg_desc->sequence_id = cpu_to_le8((u8)i);

		frg_desc->offset = cpu_to_le32(fragment_offset);

		bytes_count = sizeof(struct ssdfs_block_descriptor);
		bytes_count *= items_count;
		BUG_ON(bytes_count >= U16_MAX);

		frg_desc->compr_size = cpu_to_le16((u16)bytes_count);
		frg_desc->uncompr_size = cpu_to_le16((u16)bytes_count);

		frg_desc->checksum = 0;
		frg_desc->checksum = ssdfs_crc32_le(array, bytes_count);

		rest_items -= items_count;
	}

	bytes_count = BLK_DESC_HEADER_SIZE(fragments_count);
	bytes_count += valid_blks * sizeof(struct ssdfs_block_descriptor);

	extent->bytes_count = bytes_count;

	return 0;
}

void commit_block_descriptors(struct ssdfs_volume_layout *layout,
			      int seg_index, int peb_index)
{
	/* do nothing */
}

int define_log_footer_layout(struct ssdfs_volume_layout *layout,
			     int seg_index, int peb_index)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;
	size_t footer_len = sizeof(struct ssdfs_log_footer);

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, peb_index %d\n",
		  seg_index, peb_index);

	if (seg_index >= layout->segs_capacity) {
		SSDFS_ERR("seg_index %d >= segs_capacity %d\n",
			  seg_index, layout->segs_capacity);
		return -EINVAL;
	}

	seg_desc = &layout->segs[seg_index];

	if (peb_index >= seg_desc->pebs_capacity) {
		SSDFS_ERR("peb_index %d >= pebs_capacity %d\n",
			  peb_index, seg_desc->pebs_capacity);
		return -EINVAL;
	}

	peb_desc = &seg_desc->pebs[peb_index];
	extent = &peb_desc->extents[SSDFS_LOG_FOOTER];

	BUG_ON(extent->buf);
	BUG_ON(footer_len > layout->page_size);

	extent->buf = calloc(1, footer_len);
	if (!extent->buf) {
		SSDFS_ERR("unable to allocate memory of size %zu\n",
			  footer_len);
		return -ENOMEM;
	}

	extent->bytes_count = footer_len;

	return 0;
}

int pre_commit_log_footer(struct ssdfs_volume_layout *layout,
			  int seg_index, int peb_index)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;
	struct ssdfs_log_footer *footer;

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, peb_index %d\n",
		  seg_index, peb_index);

	if (seg_index >= layout->segs_capacity) {
		SSDFS_ERR("seg_index %d >= segs_capacity %d\n",
			  seg_index, layout->segs_capacity);
		return -EINVAL;
	}

	seg_desc = &layout->segs[seg_index];

	if (peb_index >= seg_desc->pebs_capacity) {
		SSDFS_ERR("peb_index %d >= pebs_capacity %d\n",
			  peb_index, seg_desc->pebs_capacity);
		return -EINVAL;
	}

	peb_desc = &seg_desc->pebs[peb_index];
	extent = &peb_desc->extents[SSDFS_LOG_FOOTER];

	BUG_ON(!extent->buf);
	memcpy(extent->buf, (char *)&layout->sb.vs, sizeof(layout->sb.vs));

	footer = (struct ssdfs_log_footer *)extent->buf;
	footer->volume_state.magic.key = cpu_to_le16(SSDFS_LOG_FOOTER_MAGIC);
	footer->timestamp = cpu_to_le64(layout->create_timestamp);
	footer->cno = cpu_to_le64(layout->create_cno);

	return 0;
}

void commit_log_footer(struct ssdfs_volume_layout *layout,
		       int seg_index, int peb_index,
		       u32 blks_count)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_metadata_descriptor *meta_desc;
	struct ssdfs_extent_desc *lf_extent, *extent;
	struct ssdfs_log_footer *footer;
	size_t footer_len = sizeof(struct ssdfs_log_footer);
	u32 log_flags = 0;

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, blks_count %u\n",
		  seg_index, blks_count);

	seg_desc = &layout->segs[seg_index];
	peb_desc = &seg_desc->pebs[peb_index];
	lf_extent = &peb_desc->extents[SSDFS_LOG_FOOTER];

	BUG_ON(!lf_extent->buf);
	BUG_ON(blks_count > (UINT_MAX / layout->page_size));

	footer = (struct ssdfs_log_footer *)lf_extent->buf;

	footer->log_bytes = cpu_to_le32(blks_count * layout->page_size);

	switch (seg_desc->seg_type) {
	case SSDFS_INITIAL_SNAPSHOT:
		/* do nothing */
		break;

	case SSDFS_SUPERBLOCK:
		/* do nothing */
		break;

	case SSDFS_SEGBMAP:
		if (layout->segbmap.log_pages != blks_count)
			log_flags |= SSDFS_PARTIAL_LOG_FOOTER;
		break;

	case SSDFS_PEB_MAPPING_TABLE:
		if (layout->maptbl.log_pages != blks_count)
			log_flags |= SSDFS_PARTIAL_LOG_FOOTER;
		break;

	default:
		SSDFS_WARN("unprocessed type of segment: %#x\n",
			   seg_index);
	}

	if (layout->blkbmap.has_backup_copy &&
	    peb_desc->extents[SSDFS_BLOCK_BITMAP_BACKUP].bytes_count > 0) {
		extent = &peb_desc->extents[SSDFS_BLOCK_BITMAP_BACKUP];
		meta_desc = &footer->desc_array[SSDFS_BLK_BMAP_INDEX];
		prepare_blkbmap_metadata_descriptor(layout, extent, meta_desc);
		log_flags |= SSDFS_LOG_FOOTER_HAS_BLK_BMAP;
	}

	if (layout->blk2off_tbl.has_backup_copy &&
	    peb_desc->extents[SSDFS_OFFSET_TABLE_BACKUP].bytes_count > 0) {
		extent = &peb_desc->extents[SSDFS_OFFSET_TABLE_BACKUP];
		meta_desc = &footer->desc_array[SSDFS_OFF_TABLE_INDEX];
		prepare_offset_table_metadata_descriptor(layout, extent,
							 meta_desc);
		log_flags |= SSDFS_LOG_FOOTER_HAS_OFFSET_TABLE;
	}

	footer->log_flags = cpu_to_le32(log_flags);

	footer->volume_state.check.bytes = cpu_to_le16(footer_len);
	footer->volume_state.check.flags = cpu_to_le16(SSDFS_CRC32);
	footer->volume_state.check.csum = 0;
	footer->volume_state.check.csum = ssdfs_crc32_le(footer, footer_len);
}
