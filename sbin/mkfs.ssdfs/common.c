/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/mkfs.ssdfs/common.c - common creation functionality.
 *
 * Copyright (c) 2014-2019 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 * Copyright (c) 2014-2026 Viacheslav Dubeyko <slava@dubeyko.com>
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

#include <zlib.h>

#include "mkfs.h"
#include "blkbmap.h"

/************************************************************************
 *                      Common creation functionality                   *
 ************************************************************************/

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

/*
 * The following areas follow the block descriptors (exactly in this order):
 * segment bitmap's extents, PEB mapping table's extents, PEB mapping
 * table cache. An area is placed inline (right after the previous one)
 * while it fits into the page of the segment header. Otherwise, the area
 * starts from the page boundary. After the first area that is placed
 * outside of the header's page, all the next areas start from
 * the page boundary too.
 *
 * The function returns the offset of the @target area. If @target is
 * SSDFS_LOG_PAYLOAD then the function returns the (page aligned) offset
 * that follows the last area.
 */
static
u32 define_areas_offset(struct ssdfs_peb_content *desc,
			u32 page_size, u32 inline_end,
			u32 start, int target)
{
	static const int areas[] = {
		SSDFS_SEGBMAP_META_EXTENTS,
		SSDFS_MAPTBL_META_EXTENTS,
		SSDFS_MAPTBL_CACHE,
	};
	u32 cursor = start;
	int is_inline = SSDFS_TRUE;
	int i;

	for (i = 0; i < ARRAY_SIZE(areas); i++) {
		u32 bytes = desc->extents[areas[i]].bytes_count;
		u32 offset = cursor;

		if (bytes > 0) {
			if (!is_inline || (cursor + bytes) > inline_end) {
				is_inline = SSDFS_FALSE;
				offset = ALIGN(cursor, page_size);
			}

			cursor = offset + bytes;
		}

		if (areas[i] == target)
			return offset;
	}

	BUG_ON(target != SSDFS_LOG_PAYLOAD);

	return ALIGN(cursor, page_size);
}

int set_extent_start_offset(struct ssdfs_volume_layout *layout,
			    int seg_type,
			    struct ssdfs_peb_content *desc,
			    int extent_index)
{
	u32 offset = desc->extents[SSDFS_SEG_HEADER].offset;
	u32 page_size = layout->page_size;
	u32 inline_end;

	SSDFS_DBG(layout->env.show_debug,
		  "extent_index %d\n", extent_index);

	switch (seg_type) {
	case SSDFS_SB_SEG_TYPE:
	case SSDFS_INITIAL_SNAPSHOT_SEG_TYPE:
		page_size = PAGE_CACHE_SIZE;
		break;

	default:
		/* do nothing */
		break;
	}

	switch (extent_index) {
	case SSDFS_SEGBMAP_META_EXTENTS:
	case SSDFS_MAPTBL_META_EXTENTS:
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

	SSDFS_DBG(layout->env.show_debug,
		  "STEP 1: offset %u\n", offset);

	if (extent_index < SSDFS_SEGBMAP_META_EXTENTS)
		goto set_extent_offset;

	inline_end = ((desc->extents[SSDFS_SEG_HEADER].offset / page_size) + 1) *
			page_size;

	switch (extent_index) {
	case SSDFS_SEGBMAP_META_EXTENTS:
	case SSDFS_MAPTBL_META_EXTENTS:
	case SSDFS_MAPTBL_CACHE:
		offset = define_areas_offset(desc, page_size, inline_end,
					     offset, extent_index);
		goto set_extent_offset;

	case SSDFS_LOG_PAYLOAD:
	case SSDFS_LOG_FOOTER:
	case SSDFS_BLOCK_BITMAP_BACKUP:
	case SSDFS_OFFSET_TABLE_BACKUP:
		offset = define_areas_offset(desc, page_size, inline_end,
					     offset, SSDFS_LOG_PAYLOAD);
		break;

	default:
		BUG();
	}

	SSDFS_DBG(layout->env.show_debug,
		  "STEP 2: offset %u\n", offset);

	if (extent_index == SSDFS_LOG_PAYLOAD)
		goto set_extent_offset;

	offset += desc->extents[SSDFS_LOG_PAYLOAD].bytes_count;
	offset += page_size - 1;
	offset = (offset / page_size) * page_size;

	SSDFS_DBG(layout->env.show_debug,
		  "STEP 3: offset %u\n", offset);

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

	offset += page_size - 1;
	offset = (offset / page_size) * page_size;

set_extent_offset:
	SSDFS_DBG(layout->env.show_debug,
		  "FINALLY: offset %u\n", offset);

	desc->extents[extent_index].offset = offset;

	return 0;
}

/*
 * define_inline_area_capacity() - free space of inline area for the extent
 *
 * The function defines how many bytes the @extent_index area can occupy
 * in the page of the segment header (right after the previous areas).
 * The size of @extent_index area has to be undefined (zero) yet.
 * The function defines the extent's offset as a side effect.
 */
u32 define_inline_area_capacity(struct ssdfs_volume_layout *layout,
				int seg_type,
				struct ssdfs_peb_content *desc,
				int extent_index)
{
	u32 page_size = layout->page_size;
	u32 inline_end;
	u32 offset;
	int err;

	switch (seg_type) {
	case SSDFS_SB_SEG_TYPE:
	case SSDFS_INITIAL_SNAPSHOT_SEG_TYPE:
		page_size = PAGE_CACHE_SIZE;
		break;

	default:
		/* do nothing */
		break;
	}

	BUG_ON(extent_index != SSDFS_SEGBMAP_META_EXTENTS &&
		extent_index != SSDFS_MAPTBL_META_EXTENTS &&
		extent_index != SSDFS_MAPTBL_CACHE);
	BUG_ON(desc->extents[extent_index].bytes_count != 0);

	err = set_extent_start_offset(layout, seg_type, desc, extent_index);
	if (err)
		return 0;

	inline_end = ((desc->extents[SSDFS_SEG_HEADER].offset / page_size) + 1) *
			page_size;
	offset = desc->extents[extent_index].offset;

	return offset < inline_end ? inline_end - offset : 0;
}

u32 calculate_log_pages(struct ssdfs_volume_layout *layout,
			int seg_type,
			struct ssdfs_peb_content *desc)
{
	u32 page_size = layout->page_size;
	u32 bytes_count = 0;
	u32 pages_count;

	switch (seg_type) {
	case SSDFS_SB_SEG_TYPE:
	case SSDFS_INITIAL_SNAPSHOT_SEG_TYPE:
		page_size = PAGE_CACHE_SIZE;
		break;

	default:
		/* do nothing */
		break;
	}

	bytes_count += desc->extents[SSDFS_SEG_HEADER].bytes_count;
	bytes_count += desc->extents[SSDFS_BLOCK_BITMAP].bytes_count;
	bytes_count += desc->extents[SSDFS_OFFSET_TABLE].bytes_count;
	bytes_count += desc->extents[SSDFS_BLOCK_DESCRIPTORS].bytes_count;

	bytes_count = define_areas_offset(desc, page_size, page_size,
					  bytes_count, SSDFS_LOG_PAYLOAD);

	SSDFS_DBG(layout->env.show_debug,
		  "bytes_count: SEG_HDR %u, BLK_BMAP %u, "
		  "OFFSET_TBL %u, BLK_DESC %u, "
		  "SEGBMAP_EXTENTS %u, MAPTBL_EXTENTS %u, "
		  "MAPTBL_CACHE %u, total %u\n",
		  desc->extents[SSDFS_SEG_HEADER].bytes_count,
		  desc->extents[SSDFS_BLOCK_BITMAP].bytes_count,
		  desc->extents[SSDFS_OFFSET_TABLE].bytes_count,
		  desc->extents[SSDFS_BLOCK_DESCRIPTORS].bytes_count,
		  desc->extents[SSDFS_SEGBMAP_META_EXTENTS].bytes_count,
		  desc->extents[SSDFS_MAPTBL_META_EXTENTS].bytes_count,
		  desc->extents[SSDFS_MAPTBL_CACHE].bytes_count,
		  bytes_count);

	bytes_count += desc->extents[SSDFS_LOG_PAYLOAD].bytes_count;

	bytes_count += page_size - 1;
	bytes_count = (bytes_count / page_size) * page_size;

	SSDFS_DBG(layout->env.show_debug,
		  "bytes_count: PAYLOAD %u, total %u\n",
		  desc->extents[SSDFS_LOG_PAYLOAD].bytes_count,
		  bytes_count);

	bytes_count += desc->extents[SSDFS_LOG_FOOTER].bytes_count;
	bytes_count += desc->extents[SSDFS_BLOCK_BITMAP_BACKUP].bytes_count;
	bytes_count += desc->extents[SSDFS_OFFSET_TABLE_BACKUP].bytes_count;

	bytes_count += page_size - 1;
	bytes_count = (bytes_count / page_size) * page_size;
	BUG_ON(bytes_count > layout->env.erase_size);

	SSDFS_DBG(layout->env.show_debug,
		  "bytes_count: LOG_FOOTER %u, BLK_BMAP_BACKUP %u, "
		  "OFFSET_TBL_BACKUP %u, total %u\n",
		  desc->extents[SSDFS_LOG_FOOTER].bytes_count,
		  desc->extents[SSDFS_BLOCK_BITMAP_BACKUP].bytes_count,
		  desc->extents[SSDFS_OFFSET_TABLE_BACKUP].bytes_count,
		  bytes_count);

	pages_count = bytes_count / page_size;

	SSDFS_DBG(layout->env.show_debug,
		  "bytes_count %u, pages_count %u\n",
		  bytes_count, pages_count);

	return pages_count;
}

u32 calculate_metadata_blks(struct ssdfs_volume_layout *layout,
			    int seg_type,
			    struct ssdfs_peb_content *desc)
{
	u32 page_size = layout->page_size;
	u32 bytes_count = 0;
	u32 pages_count;

	switch (seg_type) {
	case SSDFS_SB_SEG_TYPE:
	case SSDFS_INITIAL_SNAPSHOT_SEG_TYPE:
		page_size = PAGE_CACHE_SIZE;
		break;

	default:
		/* do nothing */
		break;
	}

	bytes_count += desc->extents[SSDFS_SEG_HEADER].bytes_count;
	bytes_count += desc->extents[SSDFS_BLOCK_BITMAP].bytes_count;
	bytes_count += desc->extents[SSDFS_OFFSET_TABLE].bytes_count;

	bytes_count = define_areas_offset(desc, page_size, page_size,
					  bytes_count, SSDFS_LOG_PAYLOAD);

	bytes_count += desc->extents[SSDFS_LOG_FOOTER].bytes_count;
	bytes_count += desc->extents[SSDFS_BLOCK_BITMAP_BACKUP].bytes_count;
	bytes_count += desc->extents[SSDFS_OFFSET_TABLE_BACKUP].bytes_count;

	bytes_count += page_size - 1;
	BUG_ON(bytes_count > layout->env.erase_size);

	pages_count = bytes_count / page_size;

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
	u32 flags;

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

	hdr->peb_create_time = cpu_to_le64(layout->create_timestamp);

	hdr->seg_id = cpu_to_le64(seg_desc->seg_id);
	hdr->leb_id = cpu_to_le64(peb_desc->leb_id);
	hdr->peb_id = cpu_to_le64(peb_desc->peb_id);
	hdr->relation_peb_id = cpu_to_le64(U64_MAX);

	switch (seg_type) {
	case SSDFS_SB_SEG_TYPE:
	case SSDFS_INITIAL_SNAPSHOT_SEG_TYPE:
		flags = le32_to_cpu(hdr->volume_hdr.flags);
		flags |= SSDFS_VH_4KB_PAGE_SIZE_PEB;
		hdr->volume_hdr.flags = cpu_to_le32(flags);
		break;

	default:
		/* do nothing */
		break;
	}

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
	BUG_ON(!layout || !extent || !desc);
	BUG_ON(extent->bytes_count == 0 ||
		extent->bytes_count >= layout->env.erase_size);
	BUG_ON(extent->offset == 0 ||
		extent->offset >= layout->env.erase_size);

	desc->offset = cpu_to_le32(extent->offset);
	desc->size = cpu_to_le32(extent->bytes_count);

	desc->check.bytes = cpu_to_le16(extent->bytes_count);
	desc->check.flags = cpu_to_le16(SSDFS_CRC32);
	desc->check.csum = 0;
	desc->check.csum = ssdfs_crc32_le(extent->buf, extent->bytes_count);
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

	if (extent->bytes_count >= PAGE_CACHE_SIZE)
		desc->check.bytes = cpu_to_le16((u16)PAGE_CACHE_SIZE);
	else
		desc->check.bytes = cpu_to_le16((u16)extent->bytes_count);

	desc->check.flags = cpu_to_le16(SSDFS_CRC32);
	desc->check.csum = 0;
	desc->check.csum = ssdfs_crc32_le(extent->buf,
					  le16_to_cpu(desc->check.bytes));
}

/*
 * The flags of the area's metadata check show whether the mapping table
 * cache area is compressed (the sequence of struct ssdfs_maptbl_cache_table
 * portions) or uncompressed (the sequence of fragments).
 */
static void
prepare_maptbl_cache_metadata_descriptor(struct ssdfs_volume_layout *layout,
					 struct ssdfs_extent_desc *extent,
					 struct ssdfs_metadata_descriptor *desc)
{
	size_t tbl_size = sizeof(struct ssdfs_maptbl_cache_table);
	u16 bytes_count;
	u16 flags = SSDFS_CRC32;

	BUG_ON(!layout || !extent || !desc);
	BUG_ON(extent->bytes_count == 0 ||
		extent->bytes_count >= layout->env.erase_size);
	BUG_ON(extent->offset == 0 ||
		extent->offset >= layout->env.erase_size);
	BUG_ON(!extent->buf);

	desc->offset = cpu_to_le32(extent->offset);
	desc->size = cpu_to_le32(extent->bytes_count);

	switch (extent->state) {
	case SSDFS_UNCOMPRESSED_BLOB:
		/* the whole area */
		BUG_ON(extent->bytes_count >= U16_MAX);
		bytes_count = (u16)extent->bytes_count;
		break;

	case SSDFS_ZLIB_BLOB:
		flags |= SSDFS_ZLIB_COMPRESSED;
		goto compressed_area;

	case SSDFS_LZO_BLOB:
		flags |= SSDFS_LZO_COMPRESSED;
		goto compressed_area;

	case SSDFS_LZ4_BLOB:
		flags |= SSDFS_LZ4_COMPRESSED;
		goto compressed_area;

	case SSDFS_ZSTD_BLOB:
		flags |= SSDFS_ZSTD_COMPRESSED;

compressed_area:
		/* the first table's checksum; fragments have their own ones */
		BUG_ON(extent->bytes_count < tbl_size);
		bytes_count = (u16)tbl_size;
		break;

	default:
		BUG();
	}

	desc->check.bytes = cpu_to_le16(bytes_count);
	desc->check.flags = cpu_to_le16(flags);
	desc->check.csum = 0;
	desc->check.csum = ssdfs_crc32_le(extent->buf, bytes_count);
}

static void
prepare_meta_extents_metadata_descriptor(struct ssdfs_volume_layout *layout,
					 struct ssdfs_extent_desc *extent,
					 struct ssdfs_metadata_descriptor *desc)
{
	size_t hdr_size = sizeof(struct ssdfs_metadata_extents_table);

	BUG_ON(!layout || !extent || !desc);
	BUG_ON(extent->bytes_count < hdr_size ||
		extent->bytes_count >= layout->env.erase_size);
	BUG_ON(extent->offset == 0 ||
		extent->offset >= layout->env.erase_size);
	BUG_ON(!extent->buf);

	desc->offset = cpu_to_le32(extent->offset);
	desc->size = cpu_to_le32(extent->bytes_count);

	/* the first table's checksum; fragments have their own ones */
	desc->check.bytes = cpu_to_le16(hdr_size);
	desc->check.flags = cpu_to_le16(SSDFS_CRC32);
	desc->check.csum = 0;
	desc->check.csum = ssdfs_crc32_le(extent->buf, hdr_size);
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
	u32 pages_per_peb;
	u32 log_pages;
	u32 seg_flags = 0;

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, blks_count %u\n",
		  seg_index, blks_count);

	seg_desc = &layout->segs[seg_index];
	peb_desc = &seg_desc->pebs[peb_index];
	hdr_ext = &peb_desc->extents[SSDFS_SEG_HEADER];
	pages_per_peb = layout->env.erase_size / layout->page_size;
	log_pages = pages_per_peb;
	log_pages = min_t(u32, log_pages, (u32)SSDFS_LOG_MAX_PAGES);

	BUG_ON(!hdr_ext->buf);
	BUG_ON(blks_count >= USHRT_MAX);
	BUG_ON(log_pages >= U16_MAX);

	begin_ptr = hdr_ext->buf;
	hdr = (struct ssdfs_segment_header *)begin_ptr;

	switch (seg_desc->seg_type) {
	case SSDFS_INITIAL_SNAPSHOT:
		log_pages = blks_count;
		log_pages = min_t(u32, blks_count, (u32)SSDFS_LOG_MAX_PAGES);
		BUG_ON(log_pages == 0);
		BUG_ON(log_pages >= U16_MAX);
		break;

	case SSDFS_SUPERBLOCK:
		log_pages = layout->sb.log_pages;
		log_pages = min_t(u32, log_pages, (u32)SSDFS_LOG_MAX_PAGES);
		BUG_ON(log_pages == 0);
		BUG_ON(log_pages >= U16_MAX);
		break;

	case SSDFS_SEGBMAP:
		log_pages = layout->segbmap.log_pages;
		log_pages = min_t(u32, log_pages, (u32)SSDFS_LOG_MAX_PAGES);
		BUG_ON(log_pages == 0);
		BUG_ON(log_pages >= U16_MAX);
		if (log_pages != blks_count)
			seg_flags |= SSDFS_LOG_IS_PARTIAL;
		break;

	case SSDFS_PEB_MAPPING_TABLE:
		log_pages = layout->maptbl.log_pages;
		log_pages = min_t(u32, log_pages, (u32)SSDFS_LOG_MAX_PAGES);
		BUG_ON(log_pages == 0);
		BUG_ON(log_pages >= U16_MAX);
		if (log_pages != blks_count)
			seg_flags |= SSDFS_LOG_IS_PARTIAL;
		break;

	default:
		SSDFS_WARN("unprocessed type of segment: %#x\n",
			   seg_index);
	}

	hdr->log_pages = cpu_to_le16((u16)log_pages);

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

	if (peb_desc->extents[SSDFS_MAPTBL_META_EXTENTS].bytes_count > 0) {

		if (seg_desc->seg_type != SSDFS_SUPERBLOCK) {
			SSDFS_ERR("sb segment should have maptbl extents\n");
			BUG();
		}

		extent = &peb_desc->extents[SSDFS_MAPTBL_META_EXTENTS];
		meta_desc = &hdr->desc_array[SSDFS_MAPTBL_META_EXTENTS_INDEX];
		prepare_meta_extents_metadata_descriptor(layout, extent,
							 meta_desc);
		seg_flags |= SSDFS_LOG_HAS_MAPTBL_EXT_CHAIN;
	}

	if (peb_desc->extents[SSDFS_SEGBMAP_META_EXTENTS].bytes_count > 0) {

		if (seg_desc->seg_type != SSDFS_SUPERBLOCK) {
			SSDFS_ERR("sb segment should have segbmap extents\n");
			BUG();
		}

		extent = &peb_desc->extents[SSDFS_SEGBMAP_META_EXTENTS];
		meta_desc = &hdr->desc_array[SSDFS_SEGBMAP_META_EXTENTS_INDEX];
		prepare_meta_extents_metadata_descriptor(layout, extent,
							 meta_desc);
		seg_flags |= SSDFS_LOG_HAS_SEGBMAP_EXT_CHAIN;
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
		if (seg_flags & SSDFS_LOG_IS_PARTIAL)
			seg_flags |= SSDFS_PARTIAL_HEADER_INSTEAD_FOOTER;
		else
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
	/* do nothing */
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
	    type > SSDFS_BLK2OFF_DESC_ZSTD) {
		SSDFS_ERR("invalid type %#x\n", type);
		return -EINVAL;
	}

	if (flags & ~SSDFS_FRAGMENT_DESC_FLAGS_MASK) {
		SSDFS_ERR("unknown flags %#x\n", flags);
		return -EINVAL;
	}

	desc->magic = SSDFS_FRAGMENT_DESC_MAGIC;
	desc->type = cpu_to_le8((u8)type);

	desc->flags = cpu_to_le8((u8)flags);
	desc->sequence_id = cpu_to_le8(sequence_id);

	desc->offset = cpu_to_le32(offset);
	desc->compr_size = cpu_to_le16(compr_size);
	desc->uncompr_size = cpu_to_le16(uncompr_size);

	if (flags & SSDFS_FRAGMENT_HAS_CSUM)
		desc->checksum = ssdfs_crc32_le(fragment, uncompr_size);

	return 0;
}

static int __pre_commit_block_bitmap(struct ssdfs_volume_layout *layout,
				     struct ssdfs_extent_desc *extent,
				     int peb_index,
				     size_t bytes_count,
				     u32 start_logical_blk,
				     u16 blks_count)
{
	struct ssdfs_block_bitmap_header *bmp_hdr;
	struct ssdfs_block_bitmap_fragment *bmp_frag_hdr;
	u8 *bmap;
	u8 *compr_bmap;
	size_t bmp_hdr_size = sizeof(struct ssdfs_block_bitmap_header);
	size_t bmp_frag_hdr_size = sizeof(struct ssdfs_block_bitmap_fragment);
	size_t frag_desc_size = sizeof(struct ssdfs_fragment_desc);
	u32 pages_per_peb;
	u32 pages_per_seg;
	u32 allocation_size;
	u32 compr_allocation_size;
	u32 bmap_bytes, written_bmap_bytes;
	u32 written_compr_bytes;
	u32 fragments_count;
	u32 bmap_offset;
	u32 fragment_offset;
	u8 flags = 0;
	u8 type = SSDFS_BLK_BMAP_UNCOMPRESSED_BLOB;
	u32 valid_blks, pre_allocated_blks;
	u32 i;
	int err;

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, extent %p, "
		  "bytes_count %zu, start_logical_blk %u "
		  "blks_count %u\n",
		  layout, extent, bytes_count,
		  start_logical_blk, blks_count);

	BUG_ON(!layout || !extent);
	BUG_ON(extent->buf);
	BUG_ON(extent->compr_buf);

	BUG_ON(layout->page_size == 0);
	BUG_ON(layout->page_size > layout->env.erase_size);
	pages_per_peb = layout->env.erase_size / layout->page_size;

	pages_per_seg = (u32)(layout->seg_size / layout->page_size);

	if (blks_count > pages_per_peb) {
		SSDFS_ERR("blks_count %u > pages_per_peb %u\n",
			  blks_count, pages_per_peb);
		return -EINVAL;
	}

	if (((u64)start_logical_blk + blks_count) > pages_per_seg) {
		SSDFS_ERR("start_logical_blk %u + blks_count %u > "
			  "pages_per_seg %u\n",
			  start_logical_blk, blks_count, pages_per_seg);
		return -EINVAL;
	}

	bmap_bytes = BLK_BMAP_BYTES(pages_per_seg);

	fragments_count = bmap_bytes + SSDFS_4KB - 1;
	fragments_count /= SSDFS_4KB;

	if (fragments_count > SSDFS_BLK_BMAP_FRAGMENTS_CHAIN_MAX) {
		SSDFS_ERR("block bitmap is too big: "
			  "pages_per_seg %u, fragments_count %u > max %u\n",
			  pages_per_seg, fragments_count,
			  SSDFS_BLK_BMAP_FRAGMENTS_CHAIN_MAX);
		return -E2BIG;
	}

	allocation_size = bmap_bytes + bmp_hdr_size + bmp_frag_hdr_size;
	allocation_size += fragments_count * frag_desc_size;

	extent->buf = calloc(1, allocation_size);
	if (!extent->buf) {
		SSDFS_ERR("unable to allocate memory of size %u\n",
			  allocation_size);
		return -ENOMEM;
	}

	/* LZO needs room for the worst case of the last fragment */
	compr_allocation_size = allocation_size;
	compr_allocation_size += SSDFS_LZO_WORST_COMPRESS(SSDFS_4KB) -
								SSDFS_4KB;

	extent->compr_buf = calloc(1, compr_allocation_size);
	if (!extent->compr_buf) {
		err = -ENOMEM;
		SSDFS_ERR("unable to allocate memory of size %u\n",
			  compr_allocation_size);
		goto fail_pre_commit_blk_bmap;
	}

	bmp_hdr = (struct ssdfs_block_bitmap_header *)extent->compr_buf;

	bmp_hdr->magic.common = cpu_to_le32(SSDFS_SUPER_MAGIC);
	bmp_hdr->magic.key = cpu_to_le16(SSDFS_BLK_BMAP_MAGIC);
	bmp_hdr->magic.version.major = cpu_to_le8(SSDFS_MAJOR_REVISION);
	bmp_hdr->magic.version.minor = cpu_to_le8(SSDFS_MINOR_REVISION);
	bmp_hdr->fragments_count = cpu_to_le16(1);

	if (layout->blkbmap.has_backup_copy)
		flags |= SSDFS_BLK_BMAP_BACKUP;

	switch (layout->blkbmap.compression) {
	case SSDFS_UNCOMPRESSED_BLOB:
		type = SSDFS_BLK_BMAP_UNCOMPRESSED_BLOB;
		break;

	case SSDFS_ZLIB_BLOB:
		flags |= SSDFS_BLK_BMAP_COMPRESSED;
		type = SSDFS_BLK_BMAP_ZLIB_BLOB;
		break;

	case SSDFS_LZO_BLOB:
		flags |= SSDFS_BLK_BMAP_COMPRESSED;
		type = SSDFS_BLK_BMAP_LZO_BLOB;
		break;

	case SSDFS_LZ4_BLOB:
		flags |= SSDFS_BLK_BMAP_COMPRESSED;
		type = SSDFS_BLK_BMAP_LZ4_BLOB;
		break;

	case SSDFS_ZSTD_BLOB:
		flags |= SSDFS_BLK_BMAP_COMPRESSED;
		type = SSDFS_BLK_BMAP_ZSTD_BLOB;
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
	compr_bmap = (u8 *)extent->compr_buf + bmap_offset;

	valid_blks = bytes_count / layout->page_size;
	pre_allocated_blks = blks_count - valid_blks;

	SSDFS_DBG(layout->env.show_debug,
		  "bytes_count %zu, blks_count %u, "
		  "page_size %u, valid_blks %u, "
		  "pre_allocated_blks %u\n",
		  bytes_count, blks_count,
		  layout->page_size,
		  valid_blks,
		  pre_allocated_blks);

	if (pre_allocated_blks > 1) {
		err = -ERANGE;
		SSDFS_ERR("pre_allocated_blks %u is too many\n",
			  pre_allocated_blks);
		goto fail_pre_commit_blk_bmap;
	}

	if (valid_blks > 0) {
		err = ssdfs_blkbmap_set_area(bmap, start_logical_blk,
					     valid_blks,
					     SSDFS_BLK_VALID);
		if (err) {
			SSDFS_ERR("fail to set block bitmap: "
				  "start_logical_blk %u, valid_blks %u, "
				  "err %d\n",
				  start_logical_blk, valid_blks, err);
			goto fail_pre_commit_blk_bmap;
		}
	}

	if (pre_allocated_blks > 0) {
		err = ssdfs_blkbmap_set_area(bmap,
					     start_logical_blk + valid_blks,
					     pre_allocated_blks,
					     SSDFS_BLK_PRE_ALLOCATED);
		if (err) {
			SSDFS_ERR("fail to set block bitmap: "
				  "pre_allocated_blks %u, err %d\n",
				  pre_allocated_blks, err);
			goto fail_pre_commit_blk_bmap;
		}
	}

	bmp_frag_hdr =
		(struct ssdfs_block_bitmap_fragment *)((u8 *)extent->compr_buf +
							bmp_hdr_size);

	bmp_frag_hdr->peb_index = cpu_to_le16(peb_index);
	bmp_frag_hdr->sequence_id = 0;
	bmp_frag_hdr->flags = 0;
	bmp_frag_hdr->type = SSDFS_SRC_BLK_BMAP;
	bmp_frag_hdr->last_free_blk =
			cpu_to_le32(start_logical_blk + blks_count);
	bmp_frag_hdr->invalid_blks = 0;

	bmp_frag_hdr->chain_hdr.magic = cpu_to_le8(SSDFS_CHAIN_HDR_MAGIC);
	bmp_frag_hdr->chain_hdr.type = cpu_to_le8(SSDFS_BLK_BMAP_CHAIN_HDR);
	bmp_frag_hdr->chain_hdr.flags = 0;
	bmp_frag_hdr->chain_hdr.desc_size =
		cpu_to_le16(sizeof(struct ssdfs_fragment_desc));

	bmp_frag_hdr->chain_hdr.fragments_count =
				cpu_to_le16((u16)fragments_count);
	bmp_frag_hdr->chain_hdr.uncompr_bytes = cpu_to_le32(bmap_bytes);

	fragment_offset = bmp_hdr_size;
	fragment_offset += bmp_frag_hdr_size;
	fragment_offset += fragments_count * frag_desc_size;

	written_bmap_bytes = 0;
	written_compr_bytes = 0;

	flags = SSDFS_FRAGMENT_HAS_CSUM;

	for (i = 0; i < fragments_count; i++) {
		u8 *fragment;
		u8 *compr_fragment;
		struct ssdfs_fragment_desc *cur_desc;
		u32 desc_offset;
		u32 fragment_size;
		u32 compr_size = compr_allocation_size - bmap_offset -
							written_compr_bytes;

		desc_offset = bmp_hdr_size;
		desc_offset += bmp_frag_hdr_size;
		desc_offset += i * frag_desc_size;

		cur_desc =
		    (struct ssdfs_fragment_desc *)((u8 *)extent->compr_buf +
								desc_offset);

		fragment = bmap + (i * SSDFS_4KB);
		compr_fragment = compr_bmap + written_compr_bytes;

		BUG_ON(bmap_bytes <= written_bmap_bytes);
		fragment_size = min_t(u32, bmap_bytes - written_bmap_bytes,
					(u32)SSDFS_4KB);
		BUG_ON(fragment_size >= U16_MAX);

		BUG_ON(i >= U8_MAX);

		if (fragment_size < SSDFS_MIN_SIZE_COMPRESSION_THRESHOLD)
			goto make_uncompressed_blob;

		switch (layout->blkbmap.compression) {
		case SSDFS_UNCOMPRESSED_BLOB:
make_uncompressed_blob:
			type = SSDFS_FRAGMENT_UNCOMPR_BLOB;
			compr_size = fragment_size;
			memcpy(compr_fragment, fragment, fragment_size);
			break;

		case SSDFS_ZLIB_BLOB:
			type = SSDFS_FRAGMENT_ZLIB_BLOB;
			err = ssdfs_zlib_compress(fragment,
						  compr_fragment,
						  &fragment_size,
						  &compr_size,
						  layout->env.show_debug);
			if (err) {
				SSDFS_ERR("fail to compress: "
					  "fragment_size %u, err %d\n",
					  fragment_size, err);
				goto fail_pre_commit_blk_bmap;
			}
			break;

		case SSDFS_LZO_BLOB:
			type = SSDFS_FRAGMENT_LZO_BLOB;
			err = ssdfs_lzo_compress(fragment,
						 compr_fragment,
						 &fragment_size,
						 &compr_size,
						 layout->env.show_debug);
			if (err) {
				SSDFS_ERR("fail to compress: "
					  "fragment_size %u, err %d\n",
					  fragment_size, err);
				goto fail_pre_commit_blk_bmap;
			}
			break;

		case SSDFS_LZ4_BLOB:
			type = SSDFS_FRAGMENT_LZ4_BLOB;
			err = ssdfs_lz4_compress(fragment,
						 compr_fragment,
						 &fragment_size,
						 &compr_size,
						 layout->env.show_debug);
			if (err) {
				SSDFS_ERR("fail to compress: "
					  "fragment_size %u, err %d\n",
					  fragment_size, err);
				goto fail_pre_commit_blk_bmap;
			}
			break;

		case SSDFS_ZSTD_BLOB:
			type = SSDFS_FRAGMENT_ZSTD_BLOB;
			err = ssdfs_zstd_compress(fragment,
						  compr_fragment,
						  &fragment_size,
						  &compr_size,
						  layout->env.show_debug);
			if (err) {
				SSDFS_ERR("fail to compress: "
					  "fragment_size %u, err %d\n",
					  fragment_size, err);
				goto fail_pre_commit_blk_bmap;
			}
			break;

		default:
			BUG();
		}

		err = ssdfs_fragment_descriptor_init(cur_desc, fragment,
						     fragment_offset,
						     (u16)compr_size,
						     (u16)fragment_size,
						     (u8)i, type, flags);
		if (err) {
			SSDFS_ERR("fail to init fragment descriptor: "
				  "fragment_index %u, err %d\n",
				  i, err);
			goto fail_pre_commit_blk_bmap;
		}

		fragment_offset += compr_size;
		written_bmap_bytes += fragment_size;
		written_compr_bytes += compr_size;
	}

	set_blkbmap_compression_flag(layout);

	bmp_frag_hdr->chain_hdr.compr_bytes = cpu_to_le32(written_compr_bytes);

	written_bmap_bytes = bmp_hdr_size + bmp_frag_hdr_size;
	written_bmap_bytes += fragments_count * frag_desc_size;
	written_bmap_bytes += written_compr_bytes;

	bmp_hdr->bytes_count = cpu_to_le32(written_bmap_bytes);
	extent->bytes_count = written_bmap_bytes;

	switch (layout->blkbmap.compression) {
	case SSDFS_UNCOMPRESSED_BLOB:
		extent->state = SSDFS_UNCOMPRESSED_BLOB;
		break;

	case SSDFS_ZLIB_BLOB:
		extent->state = SSDFS_ZLIB_BLOB;
		break;

	case SSDFS_LZO_BLOB:
		extent->state = SSDFS_LZO_BLOB;
		break;

	case SSDFS_LZ4_BLOB:
		extent->state = SSDFS_LZ4_BLOB;
		break;

	case SSDFS_ZSTD_BLOB:
		extent->state = SSDFS_ZSTD_BLOB;
		break;

	default:
		BUG();
	}

	free(extent->buf);
	extent->buf = extent->compr_buf;
	extent->compr_buf = NULL;

	return 0;

fail_pre_commit_blk_bmap:
	if (extent->buf) {
		free(extent->buf);
		extent->buf = NULL;
	}

	if (extent->compr_buf) {
		free(extent->compr_buf);
		extent->compr_buf = NULL;
	}

	return err;
}

int pre_commit_block_bitmap(struct ssdfs_volume_layout *layout,
			    int seg_index, int peb_index,
			    size_t bytes_count,
			    u32 start_logical_blk, u16 blks_count)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, peb_index %d, "
		  "bytes_count %zu, start_logical_blk %u, "
		  "blks_count %u\n",
		  seg_index, peb_index, bytes_count,
		  start_logical_blk, blks_count);

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

	return __pre_commit_block_bitmap(layout, extent, peb_index,
					 bytes_count,
					 start_logical_blk, blks_count);
}

int pre_commit_block_bitmap_backup(struct ssdfs_volume_layout *layout,
				   int seg_index, int peb_index,
				   size_t bytes_count,
				   u32 start_logical_blk, u16 blks_count)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, peb_index %d, "
		  "bytes_count %zu, start_logical_blk %u, "
		  "blks_count %u\n",
		  seg_index, peb_index, bytes_count,
		  start_logical_blk, blks_count);

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

	return __pre_commit_block_bitmap(layout, extent, peb_index,
					 bytes_count,
					 start_logical_blk, blks_count);
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

	bmp_frag_hdr->metadata_blks = cpu_to_le32(metadata_blks);
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
	u16 portions_count;

	offsets_per_fragment = OFF_DESC_PER_FRAGMENT();

	fragments_count = valid_blks + offsets_per_fragment - 1;
	fragments_count /= offsets_per_fragment;
	BUG_ON(fragments_count >= U16_MAX);

	portions_count = fragments_count + SSDFS_BLK2OFF_TBL_MAX - 1;
	portions_count /= SSDFS_BLK2OFF_TBL_MAX;

	/* one extent fragment per portion */
	fragments_count += portions_count;

	return (u16)fragments_count;
}

static u64 calculate_offset_table_size(u16 fragments, u16 valid_blks)
{
	size_t tbl_hdr_size = sizeof(struct ssdfs_blk2off_table_header);
	size_t hdr_size = sizeof(struct ssdfs_phys_offset_table_header);
	size_t extent_desc_size = sizeof(struct ssdfs_translation_extent);
	size_t item_size = sizeof(struct ssdfs_phys_offset_descriptor);
	u32 offsets_per_fragment;
	u32 blks_in_last_fragment;
	u16 portions_count;
	u32 item_fragments;
	u64 allocation_size = 0;

	offsets_per_fragment = OFF_DESC_PER_FRAGMENT();
	blks_in_last_fragment = (u32)valid_blks % offsets_per_fragment;

	portions_count = fragments + SSDFS_BLK2OFF_TBL_MAX - 1;
	portions_count /= SSDFS_BLK2OFF_TBL_MAX;

	item_fragments = fragments - portions_count;

	/* account table header */
	allocation_size = tbl_hdr_size * portions_count;

	/* account translation extents area */
	allocation_size += extent_desc_size * portions_count;

	/* account fragments' header size */
	allocation_size += hdr_size * item_fragments;

	/* items size */
	allocation_size +=
		(item_fragments - 1) * (offsets_per_fragment * item_size);
	allocation_size += blks_in_last_fragment * item_size;

	return allocation_size;
}

static inline u32 define_block_descriptor_offset(u16 blk_id,
						 u16 fragments)
{
	u32 blk_desc_per_fragment;
	u32 blk_desc_per_area;
	size_t hdr_size = sizeof(struct ssdfs_area_block_table);
	size_t blk_desc_size = sizeof(struct ssdfs_block_descriptor);
	u32 area_index;
	u32 offset;

	blk_desc_per_fragment = BLK_DESC_PER_FRAGMENT();
	blk_desc_per_area = blk_desc_per_fragment * SSDFS_FRAGMENTS_CHAIN_MAX;

	area_index = blk_id / blk_desc_per_area;

	offset = (area_index + 1) * hdr_size;
	offset += (u32)blk_id * blk_desc_size;

	return offset;
}

static void prepare_offsets_table_fragment(struct ssdfs_volume_layout *layout,
					   u8 *fragment, u32 pages_per_seg,
					   int peb_index,
					   u16 sequence_id, u8 area_type,
					   u32 logical_start_page,
					   u16 logical_blk,
					   u16 start_peb_page,
					   u16 start_id, u16 valid_blks,
					   u16 rest_blks,
					   u32 used_logical_blks,
					   u32 last_allocated_blk,
					   u16 *processed_blks)
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
	BUG_ON((last_allocated_blk + 1) != used_logical_blks);
	BUG_ON(used_logical_blks >= U16_MAX);
	BUG_ON(last_allocated_blk >= U16_MAX);

	hdr = (struct ssdfs_phys_offset_table_header *)fragment;
	offsets = (struct ssdfs_phys_offset_descriptor *)(fragment + hdr_size);
	*processed_blks = 0;
	id_count = min_t(u16, rest_blks, OFF_DESC_PER_FRAGMENT());
	blk_desc_fragments = BLK_DESC_TABLE_FRAGMENTS(valid_blks);

	for (i = 0; i < id_count; ++i, ++*processed_blks) {
		u32 logical_offset = logical_start_page + i;
		u32 peb_page = start_peb_page + i;
		u32 byte_offset = define_block_descriptor_offset(peb_page,
							blk_desc_fragments);
		u16 blk = logical_blk + i;

		SSDFS_DBG(layout->env.show_debug,
			  "ID %d, logical_offset %u, "
			  "logical_blk %u, peb_page %u\n",
			  i, logical_offset, blk, blk);

		offsets[i].page_desc.logical_offset = cpu_to_le32(logical_offset);
		offsets[i].page_desc.logical_blk = cpu_to_le16(blk);
		offsets[i].page_desc.peb_page = cpu_to_le16(blk);

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

	/*
	 * The used_logical_blks and free_logical_blks describe the whole
	 * translation table of the segment (every fragment keeps the same
	 * values). The driver creates the table with the capacity of
	 * the segment's logical blocks. So, the capacity is not limited by
	 * the number of descriptors in one fragment.
	 */
	free_items = min_t(u32, pages_per_seg, (u32)U16_MAX);
	BUG_ON(used_logical_blks > free_items);
	free_items -= used_logical_blks;

	hdr->used_logical_blks = cpu_to_le16((u16)used_logical_blks);
	hdr->free_logical_blks = cpu_to_le16(free_items);
	hdr->last_allocated_blk = cpu_to_le16((u16)last_allocated_blk);

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
				     u32 start_logical_blk,
				     u16 valid_blks,
				     u32 used_logical_blks,
				     u32 last_allocated_blk)
{
	struct ssdfs_blk2off_table_header *tbl_hdr;
	struct ssdfs_translation_extent *trans_extent;
	struct ssdfs_fragment_desc *frag_desc;
	size_t tbl_hdr_size = sizeof(struct ssdfs_blk2off_table_header);
	size_t extent_desc_size = sizeof(struct ssdfs_translation_extent);
	size_t phys_off_hdr_size = sizeof(struct ssdfs_phys_offset_table_header);
	size_t item_size = sizeof(struct ssdfs_phys_offset_descriptor);
	u32 pages_per_seg;
	u32 pages_per_peb;
	int fragments_count;
	u32 cur_fragment = 0;
	u32 cur_phys_off_tbl_frag = 0;
	u32 allocation_size;
	u32 offset = 0;
	u16 start_id;
	u16 rest_blks;
	u64 logical_start_page;
	u16 processed_blks = 0;
	u32 logical_blk;
	u16 portions_count;
	u8 type;
	u16 flags;
	u32 start_peb_page = 0;
	u16 i, j;
	int err;

	BUG_ON(!layout || !extent);
	BUG_ON(extent->buf);

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, peb_index %d, extent %p, "
		  "valid_blks %u, used_logical_blks %u, "
		  "last_allocated_blk %u\n",
		  layout, peb_index, extent, valid_blks,
		  used_logical_blks, last_allocated_blk);

	BUG_ON(start_logical_blk >= U16_MAX);
	BUG_ON(layout->page_size == 0);
	BUG_ON(layout->page_size > layout->env.erase_size);
	pages_per_seg = layout->blk2off_tbl.pages_per_seg;
	pages_per_peb = layout->env.erase_size / layout->page_size;

	if (valid_blks > pages_per_peb) {
		SSDFS_ERR("valid_blks %u > pages_per_peb %u\n",
			  valid_blks, pages_per_peb);
		return -EINVAL;
	}

	fragments_count = calculate_offset_table_fragments(valid_blks);

	SSDFS_DBG(layout->env.show_debug,
		  "valid_blks %u, fragments_count %d\n",
		  valid_blks, fragments_count);

	allocation_size = calculate_offset_table_size(fragments_count,
							valid_blks);
	portions_count = fragments_count + SSDFS_BLK2OFF_TBL_MAX - 1;
	portions_count /= SSDFS_BLK2OFF_TBL_MAX;

	SSDFS_DBG(layout->env.show_debug,
		  "allocation_size %u, portions_count %u\n",
		  allocation_size, portions_count);

	extent->buf = calloc(1, allocation_size);
	if (!extent->buf) {
		SSDFS_ERR("unable to allocate memory of size %u\n",
			  allocation_size);
		return -ENOMEM;
	}

	start_id = 0;
	rest_blks = valid_blks;
	logical_start_page = logical_byte_offset / layout->page_size;
	BUG_ON(logical_start_page >= U32_MAX);
	logical_blk = start_logical_blk;

	for (i = 0; i < portions_count; i++) {
		u32 portion_offset = offset;
		u32 portion_size;

		tbl_hdr = BLK2OFF_TBL_HDR((u8 *)extent->buf + offset);

		tbl_hdr->magic.common = cpu_to_le32(SSDFS_SUPER_MAGIC);
		tbl_hdr->magic.key = cpu_to_le16(SSDFS_BLK2OFF_TABLE_HDR_MAGIC);
		tbl_hdr->magic.version.major = cpu_to_le8(SSDFS_MAJOR_REVISION);
		tbl_hdr->magic.version.minor = cpu_to_le8(SSDFS_MINOR_REVISION);

		offset += tbl_hdr_size;
		j = 0;

		if (i == 0) {
			trans_extent = TRANS_EXTENT((u8 *)extent->buf + offset);

			trans_extent->logical_blk =
				cpu_to_le16((u16)start_logical_blk);
			trans_extent->offset_id = 0;
			trans_extent->len = cpu_to_le16(valid_blks);
			trans_extent->sequence_id = 0;
			trans_extent->state = cpu_to_le8(SSDFS_LOGICAL_BLK_USED);

			frag_desc = &tbl_hdr->blk[j];
			type = SSDFS_BLK2OFF_EXTENT_DESC;
			flags = SSDFS_FRAGMENT_HAS_CSUM;

			err = ssdfs_fragment_descriptor_init(frag_desc,
							(u8 *)trans_extent,
							offset,
							(u16)extent_desc_size,
							(u16)extent_desc_size,
							(u8)cur_fragment,
							type,
							flags);
			if (err) {
				SSDFS_ERR("fail to init fragment descriptor: "
					  "fragment_index %u, err %d\n",
					  cur_fragment, err);
				return err;
			}

			cur_fragment++;
			fragments_count--;

			SSDFS_DBG(layout->env.show_debug,
				  "cur_fragment %u, fragments_count %d\n",
				  cur_fragment, fragments_count);

			offset += extent_desc_size;
			j++;
		}

		for (/* initialized above */; j < SSDFS_BLK2OFF_TBL_MAX; j++) {
			u8 *fragment;
			u32 frag_size;

			if (fragments_count < 0) {
				SSDFS_ERR("invalid fragments_count %d\n",
					  fragments_count);
				return -ERANGE;
			} else if (fragments_count == 0)
				break;

			fragment = (u8 *)extent->buf + offset;

			prepare_offsets_table_fragment(layout,
							fragment, pages_per_seg,
							peb_index,
							cur_phys_off_tbl_frag,
							SSDFS_LOG_BLK_DESC_AREA,
							(u32)logical_start_page,
							(u16)logical_blk,
							(u16)start_peb_page,
							start_id, valid_blks,
							rest_blks,
							used_logical_blks,
							last_allocated_blk,
							&processed_blks);

			frag_desc = &tbl_hdr->blk[j];
			frag_size = phys_off_hdr_size +
					(processed_blks * item_size);
			type = SSDFS_BLK2OFF_DESC;
			flags = SSDFS_FRAGMENT_HAS_CSUM;

			err = ssdfs_fragment_descriptor_init(frag_desc,
							     fragment,
							     offset,
							     (u16)frag_size,
							     (u16)frag_size,
							     (u8)cur_fragment,
							     type, flags);
			if (err) {
				SSDFS_ERR("fail to init fragment descriptor: "
					  "fragment_index %u, err %d\n",
					  cur_fragment, err);
				return err;
			}

			start_id += processed_blks;
			rest_blks -= processed_blks;
			logical_start_page += processed_blks;
			logical_blk += processed_blks;
			start_peb_page += processed_blks;
			BUG_ON(start_logical_blk >= U16_MAX);

			offset += frag_size;

			cur_fragment++;
			cur_phys_off_tbl_frag++;
			fragments_count--;

			SSDFS_DBG(layout->env.show_debug,
				  "cur_fragment %u, fragments_count %d\n",
				  cur_fragment, fragments_count);
		}

		tbl_hdr->chain_hdr.magic = cpu_to_le8(SSDFS_CHAIN_HDR_MAGIC);
		tbl_hdr->chain_hdr.type = cpu_to_le8(SSDFS_BLK2OFF_CHAIN_HDR);

		if ((i + 1) < portions_count) {
			tbl_hdr->chain_hdr.flags =
				cpu_to_le16(SSDFS_MULTIPLE_HDR_CHAIN);
		}

		tbl_hdr->chain_hdr.desc_size =
			cpu_to_le16(sizeof(struct ssdfs_fragment_desc));
		tbl_hdr->chain_hdr.fragments_count = cpu_to_le16((u16)j);

		portion_size = offset - portion_offset;

		SSDFS_DBG(layout->env.show_debug,
			  "portion_offset %u, offset %u, portion_size %u\n",
			  portion_offset, offset, portion_size);

		tbl_hdr->chain_hdr.compr_bytes = cpu_to_le32(portion_size);
		tbl_hdr->chain_hdr.uncompr_bytes = cpu_to_le32(portion_size);
	}

	extent->bytes_count = allocation_size;

	return 0;
}

int pre_commit_offset_table(struct ssdfs_volume_layout *layout,
			    int seg_index, int peb_index,
			    u64 logical_byte_offset,
			    u32 start_logical_blk,
			    u16 valid_blks,
			    u32 used_logical_blks,
			    u32 last_allocated_blk)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, peb_index %d, "
		  "valid_blks %u, used_logical_blks %u, "
		  "last_allocated_blk %u\n",
		  seg_index, peb_index, valid_blks,
		  used_logical_blks, last_allocated_blk);

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
					 logical_byte_offset,
					 start_logical_blk,
					 valid_blks,
					 used_logical_blks,
					 last_allocated_blk);
}

int pre_commit_offset_table_backup(struct ssdfs_volume_layout *layout,
				   int seg_index, int peb_index,
				   u64 logical_byte_offset,
				   u32 start_logical_blk,
				   u16 valid_blks,
				   u32 used_logical_blks,
				   u32 last_allocated_blk)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, peb_index %d, "
		  "valid_blks %u, used_logical_blks %u, "
		  "last_allocated_blk %u\n",
		  seg_index, peb_index, valid_blks,
		  used_logical_blks, last_allocated_blk);

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
					 logical_byte_offset,
					 start_logical_blk,
					 valid_blks,
					 used_logical_blks,
					 last_allocated_blk);
}

static void __commit_offset_table(struct ssdfs_volume_layout *layout,
				  struct ssdfs_extent_desc *extent)
{
	struct ssdfs_blk2off_table_header *tbl_hdr;
	size_t hdr_size = sizeof(struct ssdfs_blk2off_table_header);
	u16 bytes_count;

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, extent %p\n",
		  layout, extent);

	BUG_ON(!layout || !extent);
	BUG_ON(!extent->buf);

	tbl_hdr = (struct ssdfs_blk2off_table_header *)extent->buf;

	tbl_hdr->check.bytes = le16_to_cpu(hdr_size);
	tbl_hdr->check.flags = cpu_to_le16(SSDFS_CRC32);

	bytes_count = hdr_size;
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

static u64 calculate_blk_desc_table_size(u16 fragments, u16 valid_blks)
{
	size_t hdr_size = sizeof(struct ssdfs_area_block_table);
	size_t item_size = sizeof(struct ssdfs_block_descriptor);
	size_t hdrs_count;
	u32 blk_desc_per_fragment;
	u32 blk_desc_in_last_fragment;
	u64 allocation_size = 0;

	blk_desc_per_fragment = BLK_DESC_PER_FRAGMENT();
	blk_desc_in_last_fragment = (u32)valid_blks % blk_desc_per_fragment;

	hdrs_count = fragments + SSDFS_FRAGMENTS_CHAIN_MAX - 1;
	hdrs_count /= SSDFS_FRAGMENTS_CHAIN_MAX;

	allocation_size = hdrs_count * hdr_size;

	allocation_size += (fragments - 1) * (blk_desc_per_fragment * item_size);
	allocation_size += blk_desc_in_last_fragment * item_size;

	return allocation_size;
}

static inline u32 define_area_blk_tbl_hdr_offset(u16 area_index)
{
	u32 blk_desc_per_fragment;
	u32 blk_desc_per_area;
	size_t hdr_size = sizeof(struct ssdfs_area_block_table);
	size_t blk_desc_size = sizeof(struct ssdfs_block_descriptor);
	u32 offset;

	blk_desc_per_fragment = BLK_DESC_PER_FRAGMENT();
	blk_desc_per_area = blk_desc_per_fragment * SSDFS_FRAGMENTS_CHAIN_MAX;

	offset = (u32)area_index * hdr_size;
	offset += (u32)area_index * (blk_desc_per_area * blk_desc_size);

	return offset;
}

static inline u16 FRAGMENTS_PER_AREA(u16 area_index, u16 fragments_count)
{
	size_t area_count;

	area_count = fragments_count + SSDFS_FRAGMENTS_CHAIN_MAX - 1;
	area_count /= SSDFS_FRAGMENTS_CHAIN_MAX;

	if (area_index >= area_count)
		return U16_MAX;
	else if ((area_index + 1) < area_count)
		return SSDFS_FRAGMENTS_CHAIN_MAX;
	else
		return fragments_count % SSDFS_FRAGMENTS_CHAIN_MAX;
}

static inline u16 BLKS_PER_AREA(u16 area_index, u16 fragments_count,
				u16 valid_blks)
{
	size_t area_count;
	u16 blk_desc_per_fragment;
	u16 blk_desc_per_area;

	area_count = fragments_count + SSDFS_FRAGMENTS_CHAIN_MAX - 1;
	area_count /= SSDFS_FRAGMENTS_CHAIN_MAX;

	if (area_index >= area_count)
		return U16_MAX;

	blk_desc_per_fragment = BLK_DESC_PER_FRAGMENT();
	blk_desc_per_area = blk_desc_per_fragment * SSDFS_FRAGMENTS_CHAIN_MAX;

	if ((area_index + 1) < area_count)
		return blk_desc_per_area;
	else
		return valid_blks % blk_desc_per_area;
}

static
int prepare_block_descriptor_fragment(struct ssdfs_volume_layout *layout,
				      int fragment_index,
				      struct ssdfs_fragment_desc *fdesc,
				      struct ssdfs_block_descriptor *array,
				      u32 fragment_offset,
				      u32 start_logical_blk,
				      u16 valid_blks,
				      int peb_index,
				      u64 inode_id,
				      u32 item_size,
				      u32 page_size,
				      u32 payload_offset_in_bytes,
				      u32 *cur_byte_offset)
{
	u8 area_type = SSDFS_LOG_MAIN_AREA;
	u32 bytes_count;
	int i;

	if (fragment_index >= SSDFS_FRAGMENTS_CHAIN_MAX) {
		SSDFS_ERR("invalid fragments_index %u\n",
			  fragment_index);
		return -ERANGE;
	}

	if (item_size >= page_size)
		area_type = SSDFS_LOG_MAIN_AREA;
	else
		area_type = SSDFS_LOG_JOURNAL_AREA;

	for (i = 0; i < valid_blks; i++) {
		struct ssdfs_block_descriptor *blk_desc = &array[i];
		u32 logical_page;
		u32 peb_page;

		memset(blk_desc, 0xFF, sizeof(struct ssdfs_block_descriptor));

		blk_desc->ino = cpu_to_le64(inode_id);
		BUG_ON(peb_index >= U16_MAX);
		blk_desc->peb_index = cpu_to_le16(peb_index);

		logical_page = payload_offset_in_bytes;
		logical_page += *cur_byte_offset;
		logical_page /= page_size;
		blk_desc->logical_offset = cpu_to_le32(logical_page);

		peb_page = start_logical_blk + i;
		BUG_ON(peb_page >= U16_MAX);
		blk_desc->peb_page = cpu_to_le16((u16)peb_page);

		SSDFS_DBG(layout->env.show_debug,
			  "ID %d, logical_offset %u, "
			  "ino %llu, peb_page %u\n",
			  i, logical_page, inode_id, peb_page);

		blk_desc->state[0].log_start_page = 0;
		blk_desc->state[0].log_area = cpu_to_le8(area_type);
		blk_desc->state[0].peb_migration_id =
				SSDFS_PEB_MIGRATION_ID_START;
		blk_desc->state[0].byte_offset =
				cpu_to_le32(*cur_byte_offset);

		*cur_byte_offset += item_size;
	}

	fdesc->magic = cpu_to_le8(SSDFS_FRAGMENT_DESC_MAGIC);
	fdesc->type = cpu_to_le8(SSDFS_DATA_BLK_DESC);
	fdesc->flags = cpu_to_le8(SSDFS_FRAGMENT_HAS_CSUM);
	BUG_ON(fragment_index >= U8_MAX);
	fdesc->sequence_id = cpu_to_le8((u8)fragment_index);

	fdesc->offset = cpu_to_le32(fragment_offset);

	bytes_count = sizeof(struct ssdfs_block_descriptor);
	bytes_count *= valid_blks;
	BUG_ON(bytes_count >= U16_MAX);

	fdesc->compr_size = cpu_to_le16((u16)bytes_count);
	fdesc->uncompr_size = cpu_to_le16((u16)bytes_count);

	fdesc->checksum = 0;
	fdesc->checksum = ssdfs_crc32_le(array, bytes_count);

	return 0;
}

static
int prepare_area_block_table(struct ssdfs_volume_layout *layout,
			     u16 area_index, u32 area_offset,
			     struct ssdfs_area_block_table *ptr,
			     u16 fragments_count, int has_next_area,
			     u32 start_logical_blk,
			     u16 valid_blks, int peb_index,
			     u64 inode_id,
			     u32 item_size,
			     u32 page_size,
			     u32 payload_offset_in_bytes,
			     u32 *cur_byte_offset)
{
	struct ssdfs_fragment_desc *fdesc;
	u32 bytes_count;
	size_t hdr_size = sizeof(struct ssdfs_area_block_table);
	size_t blk_desc_size = sizeof(struct ssdfs_block_descriptor);
	u32 blk_desc_per_fragment;
	u16 sequence_id = area_index * SSDFS_BLK_TABLE_MAX;
	u32 logical_blk;
	int i;
	int err;

	if (fragments_count > SSDFS_FRAGMENTS_CHAIN_MAX) {
		SSDFS_ERR("invalid fragments_count %u\n",
			  fragments_count);
		return -ERANGE;
	}

	ptr->chain_hdr.magic = cpu_to_le8(SSDFS_CHAIN_HDR_MAGIC);
	ptr->chain_hdr.type = cpu_to_le8(SSDFS_BLK_DESC_CHAIN_HDR);

	if (has_next_area)
		ptr->chain_hdr.flags = cpu_to_le16(SSDFS_MULTIPLE_HDR_CHAIN);
	else
		ptr->chain_hdr.flags = cpu_to_le16(0);

	ptr->chain_hdr.fragments_count = cpu_to_le16(fragments_count);
	ptr->chain_hdr.desc_size =
			cpu_to_le16(sizeof(struct ssdfs_fragment_desc));

	bytes_count = hdr_size;
	bytes_count += valid_blks * blk_desc_size;
	ptr->chain_hdr.compr_bytes = cpu_to_le32(bytes_count);
	ptr->chain_hdr.uncompr_bytes = cpu_to_le32(bytes_count);

	blk_desc_per_fragment = BLK_DESC_PER_FRAGMENT();
	logical_blk = start_logical_blk;

	for (i = 0; i < fragments_count; i++) {
		struct ssdfs_block_descriptor *frag;
		u32 offset;
		u16 blk_desc_count;

		fdesc = &ptr->blk[i];

		offset = hdr_size;
		offset += i * (blk_desc_per_fragment * blk_desc_size);
		frag = (struct ssdfs_block_descriptor *)((u8 *)ptr + offset);

		if ((i + 1) < fragments_count)
			blk_desc_count = blk_desc_per_fragment;
		else
			blk_desc_count = valid_blks % blk_desc_per_fragment;

		err = prepare_block_descriptor_fragment(layout,
							i, fdesc, frag,
							offset,
							logical_blk,
							blk_desc_count,
							peb_index,
							inode_id,
							item_size,
							page_size,
							payload_offset_in_bytes,
							cur_byte_offset);
		if (err) {
			SSDFS_ERR("fail to prepare fragment: "
				  "index %d, err %d\n", i, err);
			return err;
		}

		logical_blk += blk_desc_count;
	}

	if (has_next_area) {
		fdesc = &ptr->blk[SSDFS_NEXT_BLK_TABLE_INDEX];

		fdesc->magic = cpu_to_le8(SSDFS_FRAGMENT_DESC_MAGIC);
		fdesc->type = cpu_to_le8(SSDFS_NEXT_TABLE_DESC);
		fdesc->flags = cpu_to_le8(0);
		fdesc->sequence_id = cpu_to_le8((u8)(sequence_id +
						SSDFS_FRAGMENTS_CHAIN_MAX));
		fdesc->offset = cpu_to_le32(area_offset + bytes_count);
		fdesc->compr_size = cpu_to_le16((u16)hdr_size);
		fdesc->uncompr_size = cpu_to_le16((u16)hdr_size);
		fdesc->checksum = cpu_to_le32(0);
	}

	return 0;
}

int pre_commit_block_descriptors(struct ssdfs_volume_layout *layout,
				 int seg_index, int peb_index,
				 u32 start_logical_blk,
				 u16 valid_blks, u64 inode_id,
				 u32 payload_offset_in_bytes,
				 u32 item_size)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;
	struct ssdfs_area_block_table *blk_desc_tbl_hdr;
	u32 pages_per_peb;
	u16 fragments_count;
	u32 allocation_size;
	u32 cur_byte_offset = 0;
	size_t hdrs_count;
	u32 area_offset;
	u32 logical_blk;
	u16 i;
	int err;

	BUG_ON(!layout);

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, peb_index %d, "
		  "start_logical_blk %u, "
		  "valid_blks %u, inode_id %llu, "
		  "payload_offset_in_bytes %u, item_size %u\n",
		  seg_index, peb_index, start_logical_blk,
		  valid_blks, inode_id,
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
	allocation_size = calculate_blk_desc_table_size(fragments_count,
							valid_blks);

	extent->buf = calloc(1, allocation_size);
	if (!extent->buf) {
		SSDFS_ERR("unable to allocate memory of size %u\n",
			  allocation_size);
		return -ENOMEM;
	}

	hdrs_count = fragments_count + SSDFS_FRAGMENTS_CHAIN_MAX - 1;
	hdrs_count /= SSDFS_FRAGMENTS_CHAIN_MAX;

	logical_blk = start_logical_blk;

	for (i = 0; i < hdrs_count; i++) {
		u16 fragments_per_area;
		u16 blks_per_area;
		int has_next_area = SSDFS_FALSE;

		if ((i + 1) < hdrs_count)
			has_next_area = SSDFS_TRUE;
		else
			has_next_area = SSDFS_FALSE;

		area_offset = define_area_blk_tbl_hdr_offset((u16)i);
		blk_desc_tbl_hdr =
		    (struct ssdfs_area_block_table *)((u8 *)extent->buf +
							area_offset);

		fragments_per_area = FRAGMENTS_PER_AREA(i, fragments_count);
		if (fragments_per_area >= U16_MAX) {
			SSDFS_ERR("invalid fragments_per_area\n");
			return -ERANGE;
		}

		blks_per_area = BLKS_PER_AREA(i, fragments_count, valid_blks);
		if (blks_per_area >= U16_MAX) {
			SSDFS_ERR("invalid blks_per_area\n");
			return -ERANGE;
		}

		err = prepare_area_block_table(layout, i, area_offset,
						blk_desc_tbl_hdr,
						fragments_per_area,
						has_next_area,
						logical_blk,
						blks_per_area,
						peb_index,
						inode_id,
						item_size,
						layout->page_size,
						payload_offset_in_bytes,
						&cur_byte_offset);
		if (err) {
			SSDFS_ERR("fail to prepare area block table: "
				  "index %d, fragments_count %u, "
				  "valid_blks %u, err %d\n",
				  i, fragments_count, valid_blks, err);
			return err;
		}

		logical_blk += blks_per_area;
	}

	extent->bytes_count = allocation_size;

	return 0;
}

void commit_block_descriptors(struct ssdfs_volume_layout *layout,
			      int seg_index, int peb_index)
{
	/* do nothing */
}

/************************************************************************
 *              Metadata extents area creation functionality            *
 ************************************************************************/

/*
 * The volume header keeps only SSDFS_SEGBMAP_RESERVED_EXTENTS (or
 * SSDFS_MAPTBL_RESERVED_EXTENTS) rows of extents for every chain
 * (main and copy). If a metadata structure needs more extents then
 * the rest of them is stored in the superblock segment as a separate
 * area. The area continues the volume header's array: it is the sequence
 * of pairs (main extent, copy extent) for the rows that follow
 * the last row of the volume header. An absent extent has
 * SSDFS_EMPTY_EXTENT_TYPE type.
 *
 * The area has the structure that is described in ssdfs_abi.h:
 * the sequence of struct ssdfs_metadata_extents_table tables. Every table
 * describes up to SSDFS_FRAGMENTS_CHAIN_MAX fragments. Every fragment is
 * an array of extents (up to PAGE_CACHE_SIZE bytes). The last fragment
 * descriptor of the table (if the chain has SSDFS_MULTIPLE_HDR_CHAIN flag)
 * has SSDFS_NEXT_TABLE_DESC type and keeps the offset (from the area's
 * beginning) of the next table. The offset of fragment descriptor is
 * counted from the beginning of the area too. The sequence ID of
 * a fragment descriptor is its index in the table. The compr_bytes and
 * uncompr_bytes fields of the chain header account only the fragments'
 * bytes of the table (without the table itself).
 */

#define SSDFS_META_EXTENTS_PER_FRAGMENT \
	(PAGE_CACHE_SIZE / sizeof(struct ssdfs_meta_area_extent))

/* fragment's sequence ID is 8 bits wide */
#define SSDFS_META_EXTENTS_MAX_FRAGMENTS	(U8_MAX)

#define SSDFS_META_EXTENTS_MAX_COUNT \
	(SSDFS_META_EXTENTS_MAX_FRAGMENTS * SSDFS_META_EXTENTS_PER_FRAGMENT)

int set_meta_area_extent(struct ssdfs_meta_area_extent *embedded,
			 u32 embedded_rows, u32 chains_count,
			 struct ssdfs_meta_extents_layout *overflow,
			 u32 row, u32 chain, u64 start_id, u32 len)
{
	struct ssdfs_meta_area_extent *extent;
	size_t extent_size = sizeof(struct ssdfs_meta_area_extent);
	u32 needed;
	u32 i;

	BUG_ON(!embedded || !overflow);
	BUG_ON(chain >= chains_count);

	if (row < embedded_rows) {
		extent = &embedded[(row * chains_count) + chain];
		goto init_extent;
	}

	needed = (row - embedded_rows + 1) * chains_count;

	if (needed > SSDFS_META_EXTENTS_MAX_COUNT) {
		SSDFS_ERR("too many metadata extents: "
			  "row %u, embedded_rows %u, max_count %lu\n",
			  row, embedded_rows,
			  (unsigned long)SSDFS_META_EXTENTS_MAX_COUNT);
		return -E2BIG;
	}

	if (needed > overflow->capacity) {
		struct ssdfs_meta_area_extent *ptr;
		u32 capacity = max_t(u32, needed, overflow->capacity * 2);

		ptr = realloc(overflow->extents, capacity * extent_size);
		if (!ptr) {
			SSDFS_ERR("fail to allocate memory: "
				  "capacity %u, item size %zu\n",
				  capacity, extent_size);
			return -ENOMEM;
		}

		overflow->extents = ptr;
		overflow->capacity = capacity;
	}

	if (needed > overflow->count) {
		memset(&overflow->extents[overflow->count], 0xFF,
			(needed - overflow->count) * extent_size);

		for (i = overflow->count; i < needed; i++) {
			overflow->extents[i].type =
				cpu_to_le16(SSDFS_EMPTY_EXTENT_TYPE);
		}

		overflow->count = needed;
	}

	extent = &overflow->extents[((row - embedded_rows) * chains_count) +
				    chain];

init_extent:
	extent->start_id = cpu_to_le64(start_id);
	extent->len = cpu_to_le32(len);
	extent->type = cpu_to_le16(SSDFS_SEG_EXTENT_TYPE);
	extent->flags = 0;

	return 0;
}

void destroy_meta_extents_layout(struct ssdfs_meta_extents_layout *ptr)
{
	free(ptr->extents);
	ptr->extents = NULL;
	ptr->count = 0;
	ptr->capacity = 0;
}

/*
 * The function prepares the buffer of the metadata extents area and
 * defines its size (extent->bytes_count). If there is nothing to store
 * then the area stays empty (bytes_count is zero).
 */
int define_meta_extents_area(struct ssdfs_volume_layout *layout,
			     struct ssdfs_peb_content *peb_desc,
			     int extent_index,
			     struct ssdfs_meta_extents_layout *src)
{
	struct ssdfs_extent_desc *extent;
	struct ssdfs_metadata_extents_table *table;
	struct ssdfs_fragment_desc *fdesc;
	size_t hdr_size = sizeof(struct ssdfs_metadata_extents_table);
	size_t desc_size = sizeof(struct ssdfs_fragment_desc);
	size_t item_size = sizeof(struct ssdfs_meta_area_extent);
	u32 per_fragment = SSDFS_META_EXTENTS_PER_FRAGMENT;
	u32 fragments_count;
	u32 tables_count;
	u32 allocation_size;
	u32 fragment_index = 0;
	u32 items_left;
	u32 table_offset;
	u32 cur_offset = 0;
	u8 *src_ptr;
	u32 i, j;

	SSDFS_DBG(layout->env.show_debug,
		  "extent_index %d, count %u\n",
		  extent_index, src->count);

	BUG_ON(extent_index != SSDFS_SEGBMAP_META_EXTENTS &&
		extent_index != SSDFS_MAPTBL_META_EXTENTS);

	extent = &peb_desc->extents[extent_index];
	BUG_ON(extent->buf);

	if (src->count == 0) {
		/* the volume header keeps all extents */
		return 0;
	}

	if (src->count > SSDFS_META_EXTENTS_MAX_COUNT) {
		SSDFS_ERR("too many metadata extents: count %u\n",
			  src->count);
		return -E2BIG;
	}

	fragments_count = (src->count + per_fragment - 1) / per_fragment;
	tables_count = (fragments_count + SSDFS_FRAGMENTS_CHAIN_MAX - 1) /
				SSDFS_FRAGMENTS_CHAIN_MAX;
	allocation_size = (tables_count * hdr_size) + (src->count * item_size);

	extent->buf = calloc(1, allocation_size);
	if (!extent->buf) {
		SSDFS_ERR("unable to allocate memory of size %u\n",
			  allocation_size);
		return -ENOMEM;
	}

	items_left = src->count;
	src_ptr = (u8 *)src->extents;

	for (i = 0; i < tables_count; i++) {
		u32 fragments_in_table = min_t(u32,
					fragments_count - fragment_index,
					SSDFS_FRAGMENTS_CHAIN_MAX);
		int has_next_table = (i + 1) < tables_count;

		table_offset = cur_offset;
		table = (struct ssdfs_metadata_extents_table *)
					((u8 *)extent->buf + table_offset);
		cur_offset += hdr_size;

		for (j = 0; j < fragments_in_table; j++, fragment_index++) {
			u32 items = min_t(u32, items_left, per_fragment);
			u32 bytes = items * item_size;
			u8 *fragment = (u8 *)extent->buf + cur_offset;

			memcpy(fragment, src_ptr, bytes);

			fdesc = &table->blk[j];
			fdesc->magic = cpu_to_le8(SSDFS_FRAGMENT_DESC_MAGIC);
			fdesc->type = cpu_to_le8(SSDFS_META_EXT_BLOB);
			fdesc->flags = cpu_to_le8(SSDFS_FRAGMENT_HAS_CSUM);
			fdesc->sequence_id = cpu_to_le8((u8)j);
			fdesc->offset = cpu_to_le32(cur_offset);
			fdesc->compr_size = cpu_to_le16((u16)bytes);
			fdesc->uncompr_size = cpu_to_le16((u16)bytes);
			fdesc->checksum = ssdfs_crc32_le(fragment, bytes);

			src_ptr += bytes;
			items_left -= items;
			cur_offset += bytes;
		}

		table->chain_hdr.compr_bytes =
			cpu_to_le32(cur_offset - table_offset - hdr_size);
		table->chain_hdr.uncompr_bytes =
			cpu_to_le32(cur_offset - table_offset - hdr_size);
		table->chain_hdr.fragments_count =
				cpu_to_le16((u16)fragments_in_table);
		table->chain_hdr.desc_size = cpu_to_le16((u16)desc_size);
		table->chain_hdr.magic = cpu_to_le8(SSDFS_CHAIN_HDR_MAGIC);
		table->chain_hdr.type = cpu_to_le8(SSDFS_META_EXT_CHAIN_HDR);

		if (has_next_table) {
			table->chain_hdr.flags =
				cpu_to_le16(SSDFS_MULTIPLE_HDR_CHAIN);

			fdesc = &table->blk[SSDFS_NEXT_META_EXT_TABLE_INDEX];
			fdesc->magic = cpu_to_le8(SSDFS_FRAGMENT_DESC_MAGIC);
			fdesc->type = cpu_to_le8(SSDFS_NEXT_TABLE_DESC);
			fdesc->flags = cpu_to_le8(0);
			fdesc->sequence_id =
				cpu_to_le8(SSDFS_NEXT_META_EXT_TABLE_INDEX);
			fdesc->offset = cpu_to_le32(cur_offset);
			fdesc->compr_size = cpu_to_le16((u16)hdr_size);
			fdesc->uncompr_size = cpu_to_le16((u16)hdr_size);
			fdesc->checksum = cpu_to_le32(0);
		} else {
			table->chain_hdr.flags = cpu_to_le16(0);
		}
	}

	BUG_ON(items_left != 0);
	BUG_ON(cur_offset != allocation_size);

	extent->bytes_count = allocation_size;

	return 0;
}

int define_log_footer_layout(struct ssdfs_volume_layout *layout,
			     int seg_index, int peb_index)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;
	size_t footer_len = max_t(size_t,
				sizeof(struct ssdfs_log_footer),
				sizeof(struct ssdfs_partial_log_header));

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
			  int seg_type, int seg_index, int peb_index)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;
	struct ssdfs_log_footer *footer;
	u32 flags;

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

	footer = (struct ssdfs_log_footer *)extent->buf;

	switch (seg_type) {
	case SSDFS_SUPERBLOCK:
	case SSDFS_INITIAL_SNAPSHOT:
		flags = le32_to_cpu(footer->volume_state.flags);
		flags |= SSDFS_VS_4KB_PAGE_SIZE_PEB;
		footer->volume_state.flags = cpu_to_le32(flags);
		break;

	default:
		/* do nothing */
		break;
	}

	return 0;
}

static
void __commit_log_footer(struct ssdfs_volume_layout *layout,
			 int seg_type, int seg_index,
			 int peb_index, u32 blks_count)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;
	struct ssdfs_log_footer *footer;
	size_t footer_len = sizeof(struct ssdfs_log_footer);
	struct ssdfs_metadata_descriptor *meta_desc;
	u32 log_flags = 0;
	u32 page_size = layout->page_size;

	SSDFS_DBG(layout->env.show_debug,
		  "seg_type %#x, seg_index %d, "
		  "peb_index %d, blks_count %u\n",
		  seg_type, seg_index, peb_index, blks_count);

	if (seg_index >= layout->segs_capacity) {
		SSDFS_WARN("seg_index %d >= segs_capacity %d\n",
			   seg_index, layout->segs_capacity);
	}

	switch (seg_type) {
	case SSDFS_SUPERBLOCK:
	case SSDFS_INITIAL_SNAPSHOT:
		page_size = SSDFS_4KB;
		break;

	default:
		/* do nothing */
		break;
	}

	seg_desc = &layout->segs[seg_index];

	if (peb_index >= seg_desc->pebs_capacity) {
		SSDFS_WARN("peb_index %d >= pebs_capacity %d\n",
			   peb_index, seg_desc->pebs_capacity);
	}

	peb_desc = &seg_desc->pebs[peb_index];
	extent = &peb_desc->extents[SSDFS_LOG_FOOTER];

	BUG_ON(!extent->buf);
	memcpy(extent->buf, (char *)&layout->sb.vs, sizeof(layout->sb.vs));

	footer = (struct ssdfs_log_footer *)extent->buf;
	footer->volume_state.magic.key = cpu_to_le16(SSDFS_LOG_FOOTER_MAGIC);
	footer->timestamp = cpu_to_le64(layout->create_timestamp);
	footer->peb_create_time = cpu_to_le64(layout->create_timestamp);
	footer->cno = cpu_to_le64(layout->create_cno);

	footer->log_bytes = cpu_to_le32(blks_count * page_size);

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

	footer->volume_state.open_zones =
		cpu_to_le32(layout->calculated_open_zones);

	footer->log_flags = cpu_to_le32(log_flags);

	footer->volume_state.check.bytes = cpu_to_le16(footer_len);
	footer->volume_state.check.flags = cpu_to_le16(SSDFS_CRC32);
	footer->volume_state.check.csum = 0;
	footer->volume_state.check.csum = ssdfs_crc32_le(footer, footer_len);
}

static
void __commit_partial_log_header(struct ssdfs_volume_layout *layout,
				 int seg_index, int peb_index,
				 u32 blks_count)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;
	struct ssdfs_partial_log_header *pl_footer;
	size_t footer_len = sizeof(struct ssdfs_partial_log_header);
	struct ssdfs_metadata_descriptor *meta_desc;
	u32 pages_per_peb;
	u32 log_pages;
	u32 log_flags = 0;

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, peb_index %d, blks_count %u\n",
		  seg_index, peb_index, blks_count);

	pages_per_peb = layout->env.erase_size / layout->page_size;

	log_pages = pages_per_peb;
	log_pages = min_t(u32, log_pages, (u32)SSDFS_LOG_MAX_PAGES);

	BUG_ON(log_pages >= U16_MAX);

	if (seg_index >= layout->segs_capacity) {
		SSDFS_WARN("seg_index %d >= segs_capacity %d\n",
			   seg_index, layout->segs_capacity);
	}

	seg_desc = &layout->segs[seg_index];

	if (peb_index >= seg_desc->pebs_capacity) {
		SSDFS_WARN("peb_index %d >= pebs_capacity %d\n",
			   peb_index, seg_desc->pebs_capacity);
	}

	peb_desc = &seg_desc->pebs[peb_index];
	extent = &peb_desc->extents[SSDFS_LOG_FOOTER];

	BUG_ON(!extent->buf);
	pl_footer = (struct ssdfs_partial_log_header *)extent->buf;

	memset(pl_footer, 0, footer_len);

	pl_footer->magic.common = cpu_to_le32(SSDFS_SUPER_MAGIC);
	pl_footer->magic.key = cpu_to_le16(SSDFS_PARTIAL_LOG_HDR_MAGIC);
	pl_footer->magic.version.major = cpu_to_le8(SSDFS_MAJOR_REVISION);
	pl_footer->magic.version.minor = cpu_to_le8(SSDFS_MINOR_REVISION);

	pl_footer->timestamp = cpu_to_le64(layout->create_timestamp);
	pl_footer->peb_create_time = cpu_to_le64(layout->create_timestamp);
	pl_footer->cno = cpu_to_le64(layout->create_cno);

	BUG_ON(blks_count >= USHRT_MAX);

	switch (seg_desc->seg_type) {
	case SSDFS_INITIAL_SNAPSHOT:
		log_pages = blks_count;
		log_pages = min_t(u32, log_pages, (u32)SSDFS_LOG_MAX_PAGES);
		BUG_ON(log_pages == 0);
		BUG_ON(log_pages >= U16_MAX);
		break;

	case SSDFS_SUPERBLOCK:
		log_pages = layout->sb.log_pages;
		log_pages = min_t(u32, log_pages, (u32)SSDFS_LOG_MAX_PAGES);
		BUG_ON(log_pages == 0);
		BUG_ON(log_pages >= U16_MAX);
		break;

	case SSDFS_SEGBMAP:
		log_pages = layout->segbmap.log_pages;
		log_pages = min_t(u32, log_pages, (u32)SSDFS_LOG_MAX_PAGES);
		BUG_ON(log_pages == 0);
		BUG_ON(log_pages >= U16_MAX);
		if (log_pages != blks_count) {
			log_flags |= SSDFS_LOG_IS_PARTIAL |
					SSDFS_LOG_HAS_PARTIAL_HEADER |
					SSDFS_PARTIAL_HEADER_INSTEAD_FOOTER;
		}
		break;

	case SSDFS_PEB_MAPPING_TABLE:
		log_pages = layout->maptbl.log_pages;
		log_pages = min_t(u32, log_pages, (u32)SSDFS_LOG_MAX_PAGES);
		BUG_ON(log_pages == 0);
		BUG_ON(log_pages >= U16_MAX);
		if (log_pages != blks_count) {
			log_flags |= SSDFS_LOG_IS_PARTIAL |
					SSDFS_LOG_HAS_PARTIAL_HEADER |
					SSDFS_PARTIAL_HEADER_INSTEAD_FOOTER;
		}
		break;

	default:
		SSDFS_WARN("unprocessed type of segment: %#x\n",
			   seg_index);
	}

	pl_footer->log_pages = cpu_to_le16((u16)log_pages);
	pl_footer->seg_type =
		cpu_to_le16((u16)META2SEG_TYPE(seg_desc->seg_type));

	pl_footer->log_bytes = cpu_to_le32(blks_count * layout->page_size);
	memcpy(&pl_footer->flags, &layout->sb.vs.flags,
		sizeof(pl_footer->flags));

	if (layout->blkbmap.has_backup_copy &&
	    peb_desc->extents[SSDFS_BLOCK_BITMAP_BACKUP].bytes_count > 0) {
		extent = &peb_desc->extents[SSDFS_BLOCK_BITMAP_BACKUP];
		meta_desc = &pl_footer->desc_array[SSDFS_BLK_BMAP_INDEX];
		prepare_blkbmap_metadata_descriptor(layout, extent, meta_desc);
		log_flags |= SSDFS_SEG_HDR_HAS_BLK_BMAP;
	}

	if (layout->blk2off_tbl.has_backup_copy &&
	    peb_desc->extents[SSDFS_OFFSET_TABLE_BACKUP].bytes_count > 0) {
		extent = &peb_desc->extents[SSDFS_OFFSET_TABLE_BACKUP];
		meta_desc = &pl_footer->desc_array[SSDFS_OFF_TABLE_INDEX];
		prepare_offset_table_metadata_descriptor(layout, extent,
							 meta_desc);
		log_flags |= SSDFS_SEG_HDR_HAS_OFFSET_TABLE;
	}

	memcpy(&pl_footer->nsegs, &layout->sb.vs.nsegs,
		sizeof(pl_footer->nsegs));
	memcpy(&pl_footer->free_pages, &layout->sb.vs.free_pages,
		sizeof(pl_footer->free_pages));

	memcpy(&pl_footer->root_folder, &layout->sb.vs.root_folder,
		sizeof(pl_footer->root_folder));
	memcpy(&pl_footer->inodes_btree, &layout->sb.vs.inodes_btree,
		sizeof(pl_footer->inodes_btree));
	memcpy(&pl_footer->shared_extents_btree,
		&layout->sb.vs.shared_extents_btree,
		sizeof(pl_footer->shared_extents_btree));
	memcpy(&pl_footer->shared_dict_btree,
		&layout->sb.vs.shared_dict_btree,
		sizeof(pl_footer->shared_dict_btree));

	pl_footer->sequence_id = cpu_to_le32(0);

	pl_footer->log_pagesize = layout->sb.vh.log_pagesize;
	pl_footer->log_erasesize = layout->sb.vh.log_erasesize;
	pl_footer->log_segsize = layout->sb.vh.log_segsize;
	pl_footer->log_pebs_per_seg = layout->sb.vh.log_pebs_per_seg;
	pl_footer->lebs_per_peb_index = layout->sb.vh.lebs_per_peb_index;
	pl_footer->create_threads_per_seg = layout->sb.vh.create_threads_per_seg;
	pl_footer->open_zones = cpu_to_le32(layout->calculated_open_zones);

	pl_footer->seg_id = cpu_to_le64(seg_desc->seg_id);
	pl_footer->leb_id = cpu_to_le64(peb_desc->leb_id);
	pl_footer->peb_id = cpu_to_le64(peb_desc->peb_id);
	pl_footer->relation_peb_id = cpu_to_le64(U64_MAX);

	pl_footer->volume_create_time = cpu_to_le64(layout->create_timestamp);
	memcpy(pl_footer->uuid, layout->uuid, sizeof(layout->uuid));

	pl_footer->pl_flags = cpu_to_le32(log_flags);

	pl_footer->check.bytes = cpu_to_le16(footer_len);
	pl_footer->check.flags = cpu_to_le16(SSDFS_CRC32);
	pl_footer->check.csum = 0;
	pl_footer->check.csum = ssdfs_crc32_le(pl_footer, footer_len);
}

void commit_log_footer(struct ssdfs_volume_layout *layout,
		       int seg_index, int peb_index,
		       u32 blks_count)
{
	struct ssdfs_segment_desc *seg_desc;

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, blks_count %u\n",
		  seg_index, blks_count);

	BUG_ON(blks_count > (UINT_MAX / layout->page_size));

	if (seg_index >= layout->segs_capacity) {
		SSDFS_WARN("seg_index %d >= segs_capacity %d\n",
			   seg_index, layout->segs_capacity);
	}

	seg_desc = &layout->segs[seg_index];

	switch (seg_desc->seg_type) {
	case SSDFS_INITIAL_SNAPSHOT:
		__commit_log_footer(layout, seg_desc->seg_type,
				    seg_index, peb_index,
				    blks_count);
		break;

	case SSDFS_SUPERBLOCK:
		__commit_log_footer(layout, seg_desc->seg_type,
				    seg_index, peb_index,
				    blks_count);
		break;

	case SSDFS_SEGBMAP:
		SSDFS_DBG(layout->env.show_debug,
			  "log_pages %u, blks_count %u\n",
			  layout->segbmap.log_pages,
			  blks_count);
		if (layout->segbmap.log_pages != blks_count) {
			__commit_partial_log_header(layout, seg_index,
						    peb_index, blks_count);
		} else {
			__commit_log_footer(layout,
					    seg_desc->seg_type,
					    seg_index,
					    peb_index,
					    blks_count);
		}
		break;

	case SSDFS_PEB_MAPPING_TABLE:
		SSDFS_DBG(layout->env.show_debug,
			  "log_pages %u, blks_count %u\n",
			  layout->maptbl.log_pages,
			  blks_count);
		if (layout->maptbl.log_pages != blks_count) {
			__commit_partial_log_header(layout, seg_index,
						    peb_index, blks_count);
		} else {
			__commit_log_footer(layout,
					    seg_desc->seg_type,
					    seg_index,
					    peb_index,
					    blks_count);
		}
		break;

	default:
		SSDFS_WARN("unprocessed type of segment: %#x\n",
			   seg_index);
	}
}
