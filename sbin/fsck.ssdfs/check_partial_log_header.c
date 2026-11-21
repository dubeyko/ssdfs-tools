/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/fsck.ssdfs/check_partial_log_header.c - check partial log header.
 *
 * Copyright (c) 2026 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#include "fsck.h"
#include "check_file_system.h"

static
int ssdfs_fsck_partial_header_absent_item(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_UNKNOWN_ITEM_ID;
	item->state = SSDFS_FSCK_METADATA_ITEM_ABSENT;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_partial_header_signature_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_SIGNATURE_ID;

	if (!is_ssdfs_partial_log_header(&log->header.magic)) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_ERR("corrupted log's signature: "
			  "peb_id %llu, start_page %u\n",
			  log->peb_id, log->start_page);
		return SSDFS_FSCK_CHECK_RESULT_FAILURE;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_partial_header_checksum_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;
	struct ssdfs_signature *magic = NULL;
	struct ssdfs_metadata_check *check = NULL;
	size_t plh_size = sizeof(struct ssdfs_partial_log_header);

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_CHECKSUM_ID;

	pl_hdr = &log->header.pl_hdr;
	magic = &pl_hdr->magic;
	check = &pl_hdr->check;

	if (is_ssdfs_partial_log_header(magic)) {
		if (!is_csum_valid(check, pl_hdr, plh_size)) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "incorrect checksum: "
				  "peb_id %llu, start_page %u\n",
				  log->peb_id, log->start_page);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}
	} else {
		item->state = SSDFS_FSCK_METADATA_ITEM_ABSENT;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted log's signature: "
			  "peb_id %llu, start_page %u\n",
			  log->peb_id, log->start_page);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_partial_header_pagesize_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;

	pl_hdr = &log->header.pl_hdr;

	return ssdfs_fsck_pagesize_check(state, item, metadata_id, log,
					 pl_hdr->log_pagesize);
}

static
int ssdfs_fsck_partial_header_erasesize_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;

	pl_hdr = &log->header.pl_hdr;

	return ssdfs_fsck_erasesize_check(state, item, metadata_id, log,
					  pl_hdr->log_pagesize,
					  pl_hdr->log_erasesize);
}

static
int ssdfs_fsck_partial_header_segsize_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;

	pl_hdr = &log->header.pl_hdr;

	return ssdfs_fsck_segsize_check(state, item, metadata_id, log,
					pl_hdr->log_pagesize,
					pl_hdr->log_erasesize,
					pl_hdr->log_segsize);
}

static
int ssdfs_fsck_partial_header_pebs_per_seg_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;

	pl_hdr = &log->header.pl_hdr;

	return ssdfs_fsck_pebs_per_seg_check(state, item, metadata_id, log,
					pl_hdr->log_erasesize,
					pl_hdr->log_segsize,
					pl_hdr->log_pebs_per_seg,
					ssdfs_fsck_log2_to_size(
						pl_hdr->log_pebs_per_seg));
}

static
int ssdfs_fsck_partial_header_create_time_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;

	pl_hdr = &log->header.pl_hdr;

	return ssdfs_fsck_create_time_check(state, item, metadata_id, log,
				le64_to_cpu(pl_hdr->volume_create_time),
				le64_to_cpu(pl_hdr->timestamp));
}

static
int ssdfs_fsck_partial_header_flags_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;
	u32 flags;
	u16 seg_type;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_FLAGS_ID;

	pl_hdr = &log->header.pl_hdr;
	flags = le32_to_cpu(pl_hdr->flags);
	seg_type = le16_to_cpu(pl_hdr->seg_type);

	/*
	 * pl_hdr->flags contains volume state flags
	 * (driver copies fsi->fs_flags).
	 */
	if (flags & ~SSDFS_VOLUME_STATE_FLAGS_MASK) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted volume state flags: "
			  "peb_id %llu, start_page %u, "
			  "flags %#x\n",
			  log->peb_id, log->start_page, flags);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	/*
	 * Superblock and base snapshot segments always use
	 * 4KB pages, and driver always sets SSDFS_VS_4KB_PAGE_SIZE_PEB
	 * for these segment types. Other segment types can
	 * have this flag too because fsi->fs_flags is initialized
	 * by volume state of superblock segment. So, the flag
	 * is ignored for other segment types.
	 */
	switch (seg_type) {
	case SSDFS_SB_SEG_TYPE:
	case SSDFS_INITIAL_SNAPSHOT_SEG_TYPE:
		if (!(flags & SSDFS_VS_4KB_PAGE_SIZE_PEB)) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted volume state flags: "
				  "absent 4KB page size flag: "
				  "peb_id %llu, start_page %u, "
				  "seg_type %#x, flags %#x\n",
				  log->peb_id, log->start_page,
				  seg_type, flags);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}
		break;

	default:
		/* do nothing */
		break;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_partial_header_lebs_per_peb_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;
	u8 log_erasesize;
	u16 pebs_per_seg;
	u32 lebs_per_peb_index;

	pl_hdr = &log->header.pl_hdr;
	log_erasesize = pl_hdr->log_erasesize;
	pebs_per_seg = ssdfs_fsck_log2_to_size(pl_hdr->log_pebs_per_seg);
	lebs_per_peb_index = le32_to_cpu(pl_hdr->lebs_per_peb_index);

	return ssdfs_fsck_lebs_per_peb_check(state, item, metadata_id, log,
					     log_erasesize, pebs_per_seg,
					     lebs_per_peb_index);
}

static
int ssdfs_fsck_partial_header_log_pages_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;
	u64 erase_size;
	u64 page_size;
	u64 pages_per_peb;
	u16 log_pages;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_LOG_PAGES_ID;

	pl_hdr = &log->header.pl_hdr;

	erase_size = ssdfs_fsck_log2_to_size(pl_hdr->log_erasesize);

	if (erase_size == 0) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted size: "
			  "peb_id %llu, start_page %u, "
			  "erase_size %llu\n",
			  log->peb_id, log->start_page,
			  erase_size);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	page_size = ssdfs_fsck_log2_to_size(pl_hdr->log_pagesize);

	if (page_size == 0) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted page_size: "
			  "peb_id %llu, start_page %u, "
			  "page_size %llu\n",
			  log->peb_id, log->start_page, page_size);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (erase_size <= page_size) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect erase_size: "
			  "peb_id %llu, start_page %u, "
			  "erase_size %llu, page_size %llu\n",
			  log->peb_id, log->start_page,
			  erase_size, page_size);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	pages_per_peb = erase_size / page_size;

	if (pages_per_peb < SSDFS_MIN_PAGES_PER_PEB ||
	    pages_per_peb > SSDFS_MAX_PAGES_PER_PEB) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect pages_per_peb: "
			  "peb_id %llu, start_page %u, "
			  "erase_size %llu, page_size %llu, "
			  "pages_per_peb %llu, min_pages_per_peb %u, "
			  "max_pages_per_peb %u\n",
			  log->peb_id, log->start_page,
			  erase_size, page_size,
			  pages_per_peb, SSDFS_MIN_PAGES_PER_PEB,
			  SSDFS_MAX_PAGES_PER_PEB);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	log_pages = le16_to_cpu(pl_hdr->log_pages);

	switch (ssdfs_fsck_check_log_pages(state, item, log,
					   pages_per_peb, log_pages)) {
	case SSDFS_FSCK_CHECK_RESULT_SUCCESS:
		/* continue logic */
		break;

	case SSDFS_FSCK_CHECK_RESULT_CORRUPTION:
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;

	case SSDFS_FSCK_CHECK_RESULT_FAILURE:
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		return SSDFS_FSCK_CHECK_RESULT_FAILURE;

	default:
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		return SSDFS_FSCK_CHECK_RESULT_FAILURE;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static int
ssdfs_fsck_partial_header_create_threads_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;
	u16 pebs_per_seg;
	u16 create_threads_per_seg;

	pl_hdr = &log->header.pl_hdr;

	pebs_per_seg = ssdfs_fsck_log2_to_size(pl_hdr->log_pebs_per_seg);
	create_threads_per_seg = le16_to_cpu(pl_hdr->create_threads_per_seg);

	return ssdfs_fsck_create_threads_check(state, item, metadata_id, log,
						pebs_per_seg,
						pl_hdr->log_segsize,
						pl_hdr->log_erasesize,
						create_threads_per_seg);
}

static
int ssdfs_fsck_partial_header_invextree_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;
	struct ssdfs_btree_descriptor *desc;

	pl_hdr = &log->header.pl_hdr;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_INVEXTREE_ID;

	desc = &pl_hdr->invextree.desc;

	switch (ssdfs_fsck_btree_desc_check(state, item, log,
					    SSDFS_INVALIDATED_EXTENTS_BTREE,
					    desc)) {
	case SSDFS_FSCK_CHECK_RESULT_SUCCESS:
		/* continue logic */
		break;

	case SSDFS_FSCK_CHECK_RESULT_CORRUPTION:
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;

	case SSDFS_FSCK_CHECK_RESULT_FAILURE:
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		return SSDFS_FSCK_CHECK_RESULT_FAILURE;

	default:
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		return SSDFS_FSCK_CHECK_RESULT_FAILURE;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_partial_header_uuid_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;
	const char *uuid_str;

	pl_hdr = &log->header.pl_hdr;
	uuid_str = uuid_string(pl_hdr->uuid);

	return ssdfs_fsck_uuid_check(state, item, metadata_id, log, uuid_str);
}

static int
ssdfs_fsck_partial_header_log_create_time_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;
	u64 volume_create_time;
	u64 timestamp;

	pl_hdr = &log->header.pl_hdr;
	volume_create_time = le64_to_cpu(pl_hdr->volume_create_time);
	timestamp = le64_to_cpu(pl_hdr->timestamp);

	return ssdfs_fsck_log_create_time_check(state, item, metadata_id, log,
						volume_create_time,
						timestamp);
}

static int
ssdfs_fsck_partial_header_log_create_cno_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;
	u64 cno;

	pl_hdr = &log->header.pl_hdr;
	cno = le64_to_cpu(pl_hdr->cno);

	return ssdfs_fsck_log_create_cno_check(state, item, metadata_id, log,
						cno);
}

static
int ssdfs_fsck_partial_header_seg_type_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;
	u16 seg_type;

	pl_hdr = &log->header.pl_hdr;
	seg_type = le16_to_cpu(pl_hdr->seg_type);

	return ssdfs_fsck_seg_type_check(state, item, metadata_id, log,
					 seg_type);
}

static
int ssdfs_fsck_partial_header_desc_array_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;

	pl_hdr = &log->header.pl_hdr;
	return ssdfs_fsck_desc_array_check(state, item,
					   metadata_id, log,
					   pl_hdr->desc_array,
					   SSDFS_SEG_HDR_DESC_MAX);
}

static
int ssdfs_fsck_partial_header_pl_flags_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;
	u32 pl_flags;

	pl_hdr = &log->header.pl_hdr;
	pl_flags = le32_to_cpu(pl_hdr->pl_flags);

	return ssdfs_fsck_seg_flags_check(state, item, metadata_id, log,
					  pl_flags);
}

int ssdfs_fsck_log_bytes_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u16 log_pages, u8 log_pagesize,
				u8 log_erasesize, u32 log_bytes)
{
	u32 page_size;
	u32 erase_size;
	u32 calculated_pages;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_LOG_BYTES_ID;

	page_size = ssdfs_fsck_log2_to_size(log_pagesize);
	erase_size = ssdfs_fsck_log2_to_size(log_erasesize);

	if (log_bytes == 0 || log_bytes > erase_size) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect log_bytes: "
			  "peb_id %llu, start_page %u, "
			  "log_bytes %u, erase_size %u\n",
			  log->peb_id, log->start_page,
			  log_bytes, erase_size);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (log_bytes % page_size) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect log_bytes: "
			  "peb_id %llu, start_page %u, "
			  "log_bytes %u, page_size %u\n",
			  log->peb_id, log->start_page,
			  log_bytes, page_size);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	calculated_pages = log_bytes / page_size;

	if (calculated_pages == 0 || calculated_pages > log_pages) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect log_bytes: "
			  "peb_id %llu, start_page %u, "
			  "log_bytes %u, page_size %u, "
			  "calculated_pages %u, log_pages %u\n",
			  log->peb_id, log->start_page,
			  log_bytes, page_size,
			  calculated_pages, log_pages);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_partial_header_log_bytes_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;
	u16 log_pages;
	u32 log_bytes;

	pl_hdr = &log->header.pl_hdr;
	log_pages = le16_to_cpu(pl_hdr->log_pages);
	log_bytes = le32_to_cpu(pl_hdr->log_bytes);

	return ssdfs_fsck_log_bytes_check(state, item, metadata_id, log,
					  log_pages, pl_hdr->log_pagesize,
					  pl_hdr->log_erasesize, log_bytes);
}

int ssdfs_fsck_nsegs_check(struct ssdfs_thread_state *state,
			   struct ssdfs_fsck_metadata_item_state *item,
			   int metadata_id,
			   struct ssdfs_fsck_found_log *log,
			   u8 log_segsize, u64 nsegs)
{
	u32 segment_size;
	u64 calculated;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_NSEGS_ID;

	segment_size = ssdfs_fsck_log2_to_size(log_segsize);
	calculated = state->base.fs_size / segment_size;

	if (nsegs != calculated) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect segment number: "
			  "peb_id %llu, start_page %u, "
			  "segment_size %u, calculated %llu, "
			  "nsegs %llu\n",
			  log->peb_id, log->start_page,
			  segment_size, calculated, nsegs);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_partial_header_nsegs_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;
	u64 nsegs;

	pl_hdr = &log->header.pl_hdr;
	nsegs = le64_to_cpu(pl_hdr->nsegs);

	return ssdfs_fsck_nsegs_check(state, item, metadata_id, log,
					pl_hdr->log_segsize, nsegs);
}

int ssdfs_fsck_free_pages_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u8 log_pagesize, u8 log_segsize,
				u64 nsegs, u64 free_pages)
{
	u32 page_size;
	u32 segment_size;
	u32 pages_per_segment;
	u64 calculated;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_FREE_PAGES_ID;

	page_size = ssdfs_fsck_log2_to_size(log_pagesize);
	segment_size = ssdfs_fsck_log2_to_size(log_segsize);

	if (page_size == 0 || segment_size == 0) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect free pages: "
			  "peb_id %llu, start_page %u, "
			  "segment_size %u, page_size %u, "
			  "nsegs %llu, free_pages %llu\n",
			  log->peb_id, log->start_page,
			  segment_size, page_size, nsegs,
			  free_pages);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	pages_per_segment = segment_size / page_size;

	if (pages_per_segment == 0) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect free pages: "
			  "peb_id %llu, start_page %u, "
			  "segment_size %u, page_size %u, "
			  "nsegs %llu, free_pages %llu\n",
			  log->peb_id, log->start_page,
			  segment_size, page_size, nsegs,
			  free_pages);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	calculated = nsegs * pages_per_segment;

	if (free_pages > calculated) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect free pages: "
			  "peb_id %llu, start_page %u, "
			  "segment_size %u, page_size %u, "
			  "nsegs %llu, free_pages %llu\n",
			  log->peb_id, log->start_page,
			  segment_size, page_size, nsegs,
			  free_pages);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_partial_header_free_pages_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;
	u64 nsegs;
	u64 free_pages;

	pl_hdr = &log->header.pl_hdr;
	nsegs = le64_to_cpu(pl_hdr->nsegs);
	free_pages = le64_to_cpu(pl_hdr->free_pages);

	return ssdfs_fsck_free_pages_check(state, item, metadata_id, log,
					   pl_hdr->log_pagesize,
					   pl_hdr->log_segsize,
					   nsegs, free_pages);
}

static
int ssdfs_fsck_partial_header_peb_create_time_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;
	u64 peb_create_time;
	u64 timestamp;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_PEB_CREATE_TIME_ID;

	pl_hdr = &log->header.pl_hdr;
	peb_create_time = le64_to_cpu(pl_hdr->peb_create_time);
	timestamp = le64_to_cpu(pl_hdr->timestamp);

	if (peb_create_time == 0 || peb_create_time >= U64_MAX) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted create_time: "
			  "peb_id %llu, start_page %u, "
			  "peb_create_time %llx\n",
			  log->peb_id, log->start_page,
			  peb_create_time);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (timestamp == 0 || timestamp >= U64_MAX) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted create_time: "
			  "peb_id %llu, start_page %u, "
			  "timestamp %llx\n",
			  log->peb_id, log->start_page,
			  timestamp);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (peb_create_time > timestamp) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted create_time: "
			  "peb_id %llu, start_page %u, "
			  "peb_create_time %llx, timestamp %llx\n",
			  log->peb_id, log->start_page,
			  peb_create_time, timestamp);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

int ssdfs_fsck_root_folder_check(struct ssdfs_thread_state *state,
				 struct ssdfs_fsck_metadata_item_state *item,
				 int metadata_id,
				 struct ssdfs_fsck_found_log *log,
				 struct ssdfs_inode *root_folder)
{
	u16 magic;
	u16 mode;
	u64 ino;
	u64 parent_ino;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_ROOT_FOLDER_ID;

	magic = le16_to_cpu(root_folder->magic);
	mode = le16_to_cpu(root_folder->mode);
	ino = le64_to_cpu(root_folder->ino);
	parent_ino = le64_to_cpu(root_folder->parent_ino);

	if (magic != SSDFS_INODE_MAGIC ||
	    !LINUX_S_ISDIR(mode) ||
	    ino != SSDFS_ROOT_INO ||
	    parent_ino != SSDFS_ROOT_INO) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted root folder: "
			  "peb_id %llu, start_page %u, "
			  "magic %#x, mode %#o, ino %llu, "
			  "parent_ino %llu\n",
			  log->peb_id, log->start_page,
			  magic, mode, ino, parent_ino);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_partial_header_root_folder_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;

	pl_hdr = &log->header.pl_hdr;

	return ssdfs_fsck_root_folder_check(state, item, metadata_id, log,
					    &pl_hdr->root_folder);
}

static
int ssdfs_fsck_partial_header_inodes_btree_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;
	struct ssdfs_btree_descriptor *desc;

	pl_hdr = &log->header.pl_hdr;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_INODES_BTREE_ID;

	desc = &pl_hdr->inodes_btree.desc;

	switch (ssdfs_fsck_btree_desc_check(state, item, log,
					    SSDFS_INODES_BTREE,
					    desc)) {
	case SSDFS_FSCK_CHECK_RESULT_SUCCESS:
		/* continue logic */
		break;

	case SSDFS_FSCK_CHECK_RESULT_CORRUPTION:
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;

	case SSDFS_FSCK_CHECK_RESULT_FAILURE:
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		return SSDFS_FSCK_CHECK_RESULT_FAILURE;

	default:
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		return SSDFS_FSCK_CHECK_RESULT_FAILURE;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_partial_header_shextree_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;
	struct ssdfs_btree_descriptor *desc;

	pl_hdr = &log->header.pl_hdr;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_SHEXTREE_ID;

	desc = &pl_hdr->shared_extents_btree.desc;

	switch (ssdfs_fsck_btree_desc_check(state, item, log,
					    SSDFS_SHARED_EXTENTS_BTREE,
					    desc)) {
	case SSDFS_FSCK_CHECK_RESULT_SUCCESS:
		/* continue logic */
		break;

	case SSDFS_FSCK_CHECK_RESULT_CORRUPTION:
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;

	case SSDFS_FSCK_CHECK_RESULT_FAILURE:
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		return SSDFS_FSCK_CHECK_RESULT_FAILURE;

	default:
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		return SSDFS_FSCK_CHECK_RESULT_FAILURE;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_partial_header_shdict_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;
	struct ssdfs_btree_descriptor *desc;

	pl_hdr = &log->header.pl_hdr;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_SHDICT_BTREE_ID;

	desc = &pl_hdr->shared_dict_btree.desc;

	switch (ssdfs_fsck_btree_desc_check(state, item, log,
					    SSDFS_SHARED_DICTIONARY_BTREE,
					    desc)) {
	case SSDFS_FSCK_CHECK_RESULT_SUCCESS:
		/* continue logic */
		break;

	case SSDFS_FSCK_CHECK_RESULT_CORRUPTION:
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;

	case SSDFS_FSCK_CHECK_RESULT_FAILURE:
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		return SSDFS_FSCK_CHECK_RESULT_FAILURE;

	default:
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		return SSDFS_FSCK_CHECK_RESULT_FAILURE;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_partial_header_snap_btree_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;
	struct ssdfs_btree_descriptor *desc;

	pl_hdr = &log->header.pl_hdr;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_SNAP_BTREE_ID;

	desc = &pl_hdr->snapshots_btree.desc;

	switch (ssdfs_fsck_btree_desc_check(state, item, log,
					    SSDFS_SNAPSHOTS_BTREE,
					    desc)) {
	case SSDFS_FSCK_CHECK_RESULT_SUCCESS:
		/* continue logic */
		break;

	case SSDFS_FSCK_CHECK_RESULT_CORRUPTION:
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;

	case SSDFS_FSCK_CHECK_RESULT_FAILURE:
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		return SSDFS_FSCK_CHECK_RESULT_FAILURE;

	default:
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		return SSDFS_FSCK_CHECK_RESULT_FAILURE;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

int ssdfs_fsck_open_zones_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u32 open_zones, u64 nsegs)
{
	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_OPEN_ZONES_ID;

	/*
	 * The driver stores the counter of open zones
	 * (zero for conventional devices). Every open zone
	 * belongs to a segment, so the counter cannot be
	 * bigger than the number of segments.
	 */
	if (open_zones > nsegs) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted open zones: "
			  "peb_id %llu, start_page %u, "
			  "open_zones %u, nsegs %llu\n",
			  log->peb_id, log->start_page,
			  open_zones, nsegs);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_partial_header_open_zones_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_partial_log_header *pl_hdr;

	pl_hdr = &log->header.pl_hdr;

	return ssdfs_fsck_open_zones_check(state, item, metadata_id, log,
					   le32_to_cpu(pl_hdr->open_zones),
					   le64_to_cpu(pl_hdr->nsegs));
}

static check_metadata_item_func partial_header_check[SSDFS_FSCK_HDR_ITEMS_MAX] = {
	ssdfs_fsck_partial_header_signature_check,	/* SSDFS_FSCK_HDR_SIGNATURE_ID */
	ssdfs_fsck_seg_header_version_check,		/* SSDFS_FSCK_HDR_VERSION_ID */
	ssdfs_fsck_partial_header_checksum_check,	/* SSDFS_FSCK_HDR_CHECKSUM_ID */
	ssdfs_fsck_partial_header_pagesize_check,	/* SSDFS_FSCK_HDR_PAGESIZE_ID */
	ssdfs_fsck_partial_header_erasesize_check,	/* SSDFS_FSCK_HDR_ERASESIZE_ID */
	ssdfs_fsck_partial_header_segsize_check,	/* SSDFS_FSCK_HDR_SEGSIZE_ID */
	ssdfs_fsck_partial_header_absent_item,		/* SSDFS_FSCK_HDR_MEGS_PER_PEB_ID */
	ssdfs_fsck_partial_header_pebs_per_seg_check,	/* SSDFS_FSCK_HDR_PEBS_PER_SEG_ID */
	ssdfs_fsck_partial_header_create_time_check,	/* SSDFS_FSCK_HDR_CREATE_TIME_ID */
	ssdfs_fsck_partial_header_absent_item,		/* SSDFS_FSCK_HDR_CREATE_CNO_ID */
	ssdfs_fsck_partial_header_flags_check,		/* SSDFS_FSCK_HDR_FLAGS_ID */
	ssdfs_fsck_partial_header_lebs_per_peb_check,	/* SSDFS_FSCK_HDR_LEBS_PER_PEB_INDEX_ID */
	ssdfs_fsck_partial_header_absent_item,		/* SSDFS_FSCK_HDR_SB_PEBS_ID */
	ssdfs_fsck_partial_header_absent_item,		/* SSDFS_FSCK_HDR_SEGBMAP_ID */
	ssdfs_fsck_partial_header_absent_item,		/* SSDFS_FSCK_HDR_MAPTBL_ID */
	ssdfs_fsck_partial_header_log_pages_check,	/* SSDFS_FSCK_HDR_LOG_PAGES_ID */
	ssdfs_fsck_partial_header_create_threads_check,	/* SSDFS_FSCK_HDR_CREATE_THREADS_ID */
	ssdfs_fsck_partial_header_absent_item,		/* SSDFS_FSCK_HDR_DENTRIES_BTREE_ID */
	ssdfs_fsck_partial_header_absent_item,		/* SSDFS_FSCK_HDR_EXTENTS_BTREE_ID */
	ssdfs_fsck_partial_header_absent_item,		/* SSDFS_FSCK_HDR_XATTR_BTREE_ID */
	ssdfs_fsck_partial_header_invextree_check,	/* SSDFS_FSCK_HDR_INVEXTREE_ID */
	ssdfs_fsck_partial_header_uuid_check,		/* SSDFS_FSCK_HDR_UUID_ID */
	ssdfs_fsck_partial_header_log_create_time_check,/* SSDFS_FSCK_HDR_LOG_TIMESTAMP_ID */
	ssdfs_fsck_partial_header_log_create_cno_check,	/* SSDFS_FSCK_HDR_LOG_CNO_ID */
	ssdfs_fsck_partial_header_seg_type_check,	/* SSDFS_FSCK_HDR_SEG_TYPE_ID */
	ssdfs_fsck_partial_header_absent_item,		/* SSDFS_FSCK_HDR_SEG_FLAGS_ID */
	ssdfs_fsck_partial_header_desc_array_check,	/* SSDFS_FSCK_HDR_DESC_ARRAY_ID */
	ssdfs_fsck_partial_header_absent_item,		/* SSDFS_FSCK_HDR_PEB_MIGRATION_ID */
	ssdfs_fsck_partial_header_peb_create_time_check,/* SSDFS_FSCK_HDR_PEB_CREATE_TIME_ID */
	ssdfs_fsck_partial_header_absent_item,		/* SSDFS_FSCK_HDR_RELATION_PEB_ID */
	ssdfs_fsck_partial_header_pl_flags_check,	/* SSDFS_FSCK_HDR_PL_FLAGS_ID */
	ssdfs_fsck_partial_header_log_bytes_check,	/* SSDFS_FSCK_HDR_LOG_BYTES_ID */
	ssdfs_fsck_partial_header_nsegs_check,		/* SSDFS_FSCK_HDR_NSEGS_ID */
	ssdfs_fsck_partial_header_free_pages_check,	/* SSDFS_FSCK_HDR_FREE_PAGES_ID */
	ssdfs_fsck_partial_header_root_folder_check,	/* SSDFS_FSCK_HDR_ROOT_FOLDER_ID */
	ssdfs_fsck_partial_header_inodes_btree_check,	/* SSDFS_FSCK_HDR_INODES_BTREE_ID */
	ssdfs_fsck_partial_header_shextree_check,	/* SSDFS_FSCK_HDR_SHEXTREE_ID */
	ssdfs_fsck_partial_header_shdict_check,		/* SSDFS_FSCK_HDR_SHDICT_BTREE_ID */
	ssdfs_fsck_partial_header_absent_item,		/* SSDFS_FSCK_HDR_SEQUENCE_ID */
	ssdfs_fsck_partial_header_snap_btree_check,	/* SSDFS_FSCK_HDR_SNAP_BTREE_ID */
	ssdfs_fsck_partial_header_open_zones_check,	/* SSDFS_FSCK_HDR_OPEN_ZONES_ID */
};

int ssdfs_fsck_check_partial_log_header(struct ssdfs_thread_state *state,
		struct ssdfs_fsck_metadata_items_array *metadata,
		struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_fsck_metadata_items_array *items;
	struct ssdfs_fsck_metadata_item_state *cur;
	int metadata_id = SSDFS_FSCK_LOG_HDR_METADATA_ID;
	int i;
	int err;
	int res = SSDFS_FSCK_CHECK_RESULT_SUCCESS;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u\n",
		  log->peb_id, log->start_page);

	items = &metadata[metadata_id];

	err = ssdfs_fsck_init_metadata_items(items, SSDFS_FSCK_HDR_ITEMS_MAX);
	if (err) {
		SSDFS_ERR("fail to allocate partial log header's items: err %d\n",
			  err);
		return SSDFS_FSCK_CHECK_RESULT_FAILURE;
	}

	for (i = 0; i < SSDFS_FSCK_HDR_ITEMS_MAX; i++) {
		cur = &items->states[i];

		items->count++;
		items->checked++;

		res = partial_header_check[i](state, cur, metadata_id, log);
		switch (res) {
		case SSDFS_FSCK_CHECK_RESULT_SUCCESS:
			/* continue logic */
			break;

		case SSDFS_FSCK_CHECK_RESULT_CORRUPTION:
			items->corrupted++;
			break;

		default:
			items->corrupted++;
			SSDFS_ERR("fail to check metadata item: "
				  "id %d\n", i);
			return SSDFS_FSCK_CHECK_RESULT_FAILURE;
		}
	}

	if (items->corrupted > 0)
		res = SSDFS_FSCK_CHECK_RESULT_CORRUPTION;

	return res;
}
