/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/fsck.ssdfs/check_segment_header.c - check segment header.
 *
 * Copyright (c) 2026 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#include <ctype.h>

#include "fsck.h"
#include "check_file_system.h"
#include "segbmap.h"

static inline
u8 ssdfs_fsck_log_header_log_pagesize(struct ssdfs_fsck_found_log *log)
{
	if (is_ssdfs_partial_log_header(&log->header.magic))
		return log->header.pl_hdr.log_pagesize;

	return log->header.seg_hdr.volume_hdr.log_pagesize;
}

static
int ssdfs_fsck_seg_header_absent_item(struct ssdfs_thread_state *state,
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
int ssdfs_fsck_seg_header_signature_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_SIGNATURE_ID;

	if (!is_ssdfs_segment_header(&log->header.magic)) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_ERR("corrupted log's signature: "
			  "peb_id %llu, start_page %u\n",
			  log->peb_id, log->start_page);
		return SSDFS_FSCK_CHECK_RESULT_FAILURE;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

int ssdfs_fsck_seg_header_version_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_VERSION_ID;

	if (log->header.magic.version.major != SSDFS_MAJOR_REVISION ||
	    log->header.magic.version.minor != SSDFS_MINOR_REVISION) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_ERR("unexpected on-disk layout version: "
			  "peb_id %llu, start_page %u, "
			  "version (major %u, minor %u), "
			  "expected (major %u, minor %u)\n",
			  log->peb_id, log->start_page,
			  log->header.magic.version.major,
			  log->header.magic.version.minor,
			  SSDFS_MAJOR_REVISION,
			  SSDFS_MINOR_REVISION);
		return SSDFS_FSCK_CHECK_RESULT_FAILURE;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_seg_header_checksum_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;
	struct ssdfs_signature *magic;
	struct ssdfs_metadata_check *check;
	size_t sg_size = sizeof(struct ssdfs_segment_header);

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_CHECKSUM_ID;

	seg_hdr = &log->header.seg_hdr;
	magic = &seg_hdr->volume_hdr.magic;
	check = &seg_hdr->volume_hdr.check;

	if (is_ssdfs_segment_header(magic)) {
		if (!is_csum_valid(check, seg_hdr, sg_size)) {
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

int ssdfs_fsck_pagesize_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u8 log_pagesize)
{
	u32 min_log_pagesize = ilog2(SSDFS_4KB);
	u32 max_log_pagesize = ilog2(SSDFS_128KB);

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_PAGESIZE_ID;

	if (log_pagesize < min_log_pagesize || log_pagesize > max_log_pagesize) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect log_pagesize: "
			  "peb_id %llu, start_page %u, "
			  "log_pagesize %u, min_log_pagesize %u, "
			  "max_log_pagesize %u\n",
			  log->peb_id, log->start_page, log_pagesize,
			  min_log_pagesize, max_log_pagesize);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_seg_header_pagesize_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;

	seg_hdr = &log->header.seg_hdr;

	return ssdfs_fsck_pagesize_check(state, item, metadata_id, log,
					 seg_hdr->volume_hdr.log_pagesize);
}

int ssdfs_fsck_erasesize_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u8 log_pagesize, u8 log_erasesize)
{
	u32 min_log_erasesize = ilog2(SSDFS_128KB);
	u32 max_log_erasesize = ilog2(SSDFS_2GB);
	u64 erase_size;
	u64 page_size;
	u64 pages_per_peb;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_ERASESIZE_ID;

	if (log_erasesize < min_log_erasesize ||
	    log_erasesize > max_log_erasesize) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect log_erasesize: "
			  "peb_id %llu, start_page %u, "
			  "log_erasesize %u, min_log_erasesize %u, "
			  "max_log_erasesize %u\n",
			  log->peb_id, log->start_page, log_erasesize,
			  min_log_erasesize, max_log_erasesize);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	erase_size = ssdfs_fsck_log2_to_size(log_erasesize);
	page_size = ssdfs_fsck_log2_to_size(log_pagesize);

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

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_seg_header_erasesize_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;

	seg_hdr = &log->header.seg_hdr;

	return ssdfs_fsck_erasesize_check(state, item, metadata_id, log,
					  seg_hdr->volume_hdr.log_pagesize,
					  seg_hdr->volume_hdr.log_erasesize);
}

int ssdfs_fsck_segsize_check(struct ssdfs_thread_state *state,
			     struct ssdfs_fsck_metadata_item_state *item,
			     int metadata_id,
			     struct ssdfs_fsck_found_log *log,
			     u8 log_pagesize, u8 log_erasesize,
			     u8 log_segsize)
{
	u32 min_log_segsize = ilog2(SSDFS_128KB);
	u64 seg_size;
	u64 erase_size;
	u64 pebs_per_seg;
	u32 log_pebs_per_seg;
	u64 page_size;
	u64 pages_per_seg;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_SEGSIZE_ID;

	if (log_segsize < min_log_segsize) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect log_segsize: "
			  "peb_id %llu, start_page %u, "
			  "log_segsize %u, min_log_segsize %u\n",
			  log->peb_id, log->start_page,
			  log_segsize, min_log_segsize);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	seg_size = ssdfs_fsck_log2_to_size(log_segsize);
	erase_size = ssdfs_fsck_log2_to_size(log_erasesize);

	if (seg_size == 0 || erase_size == 0) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted size: "
			  "peb_id %llu, start_page %u, "
			  "seg_size %llu, erase_size %llu\n",
			  log->peb_id, log->start_page,
			  seg_size, erase_size);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (seg_size < erase_size) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect seg_size: "
			  "peb_id %llu, start_page %u, "
			  "seg_size %llu, erase_size %llu\n",
			  log->peb_id, log->start_page,
			  seg_size, erase_size);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	pebs_per_seg = seg_size / erase_size;

	if (pebs_per_seg == 0) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect pebs_per_seg: "
			  "peb_id %llu, start_page %u, "
			  "seg_size %llu, erase_size %llu, "
			  "pebs_per_seg %llu\n",
			  log->peb_id, log->start_page,
			  seg_size, erase_size,
			  pebs_per_seg);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	log_pebs_per_seg = ilog2(pebs_per_seg);

	if (((u64)1 << log_pebs_per_seg) != pebs_per_seg) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect pebs_per_seg: "
			  "peb_id %llu, start_page %u, "
			  "seg_size %llu, erase_size %llu, "
			  "pebs_per_seg %llu\n",
			  log->peb_id, log->start_page,
			  seg_size, erase_size,
			  pebs_per_seg);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	page_size = ssdfs_fsck_log2_to_size(log_pagesize);

	if (page_size == 0) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted page_size: "
			  "peb_id %llu, start_page %u, "
			  "page_size %llu\n",
			  log->peb_id, log->start_page, page_size);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (seg_size <= page_size) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect erase_size: "
			  "peb_id %llu, start_page %u, "
			  "seg_size %llu, page_size %llu\n",
			  log->peb_id, log->start_page,
			  seg_size, page_size);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	pages_per_seg = seg_size / page_size;

	if (pages_per_seg < SSDFS_MIN_PAGES_PER_PEB ||
	    pages_per_seg > SSDFS_MAX_PAGES_PER_PEB) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect pages_per_seg: "
			  "peb_id %llu, start_page %u, "
			  "seg_size %llu, page_size %llu, "
			  "pages_per_seg %llu, min_pages_per_seg %u, "
			  "max_pages_per_seg %u\n",
			  log->peb_id, log->start_page,
			  seg_size, page_size,
			  pages_per_seg, SSDFS_MIN_PAGES_PER_PEB,
			  SSDFS_MAX_PAGES_PER_PEB);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_seg_header_segsize_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;

	seg_hdr = &log->header.seg_hdr;

	return ssdfs_fsck_segsize_check(state, item, metadata_id, log,
					seg_hdr->volume_hdr.log_pagesize,
					seg_hdr->volume_hdr.log_erasesize,
					seg_hdr->volume_hdr.log_segsize);
}

static
int ssdfs_fsck_seg_header_megs_per_peb_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *hdr = &log->header.seg_hdr;
	u64 erase_size;
	u64 megabytes_per_peb;
	u64 calculated;
	u32 flags;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_MEGS_PER_PEB_ID;

	erase_size = ssdfs_fsck_log2_to_size(hdr->volume_hdr.log_erasesize);

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

	megabytes_per_peb = le16_to_cpu(hdr->volume_hdr.megabytes_per_peb);

	if (megabytes_per_peb >= U16_MAX) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "unsupported megabytes_per_peb: "
			  "peb_id %llu, start_page %u, "
			  "megabytes_per_peb %llu\n",
			  log->peb_id, log->start_page,
			  megabytes_per_peb);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (megabytes_per_peb == 0) {
		switch (erase_size) {
		case SSDFS_128KB:
		case SSDFS_256KB:
		case SSDFS_512KB:
			/* expected value */
			break;

		default:
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "incorrect megabytes_per_peb: "
				  "peb_id %llu, start_page %u, "
				  "megabytes_per_peb %llu, "
				  "erase_size %llu\n",
				  log->peb_id, log->start_page,
				  megabytes_per_peb, erase_size);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}
	}

	calculated = erase_size / SSDFS_1MB;

	if (calculated != megabytes_per_peb) {
		flags = le32_to_cpu(hdr->volume_hdr.flags);

		if (flags & SSDFS_VH_ZNS_BASED_VOLUME &&
		    flags & SSDFS_VH_UNALIGNED_ZONE) {
			/* expected case */
			SSDFS_DBG(state->base.show_debug,
				  "peb_id %llu, start_page %u, "
				  "megabytes_per_peb %llu, "
				  "erase_size %llu, calculated %llu\n",
				  log->peb_id, log->start_page,
				  megabytes_per_peb, erase_size,
				  calculated);
		} else {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "incorrect megabytes_per_peb: "
				  "peb_id %llu, start_page %u, "
				  "megabytes_per_peb %llu, "
				  "erase_size %llu\n",
				  log->peb_id, log->start_page,
				  megabytes_per_peb, erase_size);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

int ssdfs_fsck_pebs_per_seg_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u8 log_erasesize,
				u8 log_segsize,
				u8 log_pebs_per_seg,
				u16 pebs_per_seg)
{
	u64 seg_size;
	u64 erase_size;
	u64 calculated;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_PEBS_PER_SEG_ID;

	seg_size = ssdfs_fsck_log2_to_size(log_segsize);
	erase_size = ssdfs_fsck_log2_to_size(log_erasesize);

	if (seg_size == 0 || erase_size == 0) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted size: "
			  "peb_id %llu, start_page %u, "
			  "seg_size %llu, erase_size %llu\n",
			  log->peb_id, log->start_page,
			  seg_size, erase_size);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (seg_size < erase_size) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect seg_size: "
			  "peb_id %llu, start_page %u, "
			  "seg_size %llu, erase_size %llu\n",
			  log->peb_id, log->start_page,
			  seg_size, erase_size);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	calculated = seg_size / erase_size;

	if (calculated == 0) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect pebs_per_seg: "
			  "peb_id %llu, start_page %u, "
			  "seg_size %llu, erase_size %llu, "
			  "pebs_per_seg %u, calculated %llu\n",
			  log->peb_id, log->start_page,
			  seg_size, erase_size,
			  pebs_per_seg, calculated);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (log_pebs_per_seg != ilog2(pebs_per_seg)) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect pebs_per_seg: "
			  "peb_id %llu, start_page %u, "
			  "seg_size %llu, erase_size %llu, "
			  "pebs_per_seg %u, log_pebs_per_seg %u\n",
			  log->peb_id, log->start_page,
			  seg_size, erase_size,
			  pebs_per_seg, log_pebs_per_seg);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (((u64)1 << log_pebs_per_seg) != calculated) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect pebs_per_seg: "
			  "peb_id %llu, start_page %u, "
			  "seg_size %llu, erase_size %llu, "
			  "pebs_per_seg %u, calculated %llu\n",
			  log->peb_id, log->start_page,
			  seg_size, erase_size,
			  pebs_per_seg, calculated);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_seg_header_pebs_per_seg_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;

	seg_hdr = &log->header.seg_hdr;

	return ssdfs_fsck_pebs_per_seg_check(state, item, metadata_id, log,
				seg_hdr->volume_hdr.log_erasesize,
				seg_hdr->volume_hdr.log_segsize,
				seg_hdr->volume_hdr.log_pebs_per_seg,
				le16_to_cpu(seg_hdr->volume_hdr.pebs_per_seg));
}

int ssdfs_fsck_create_time_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u64 create_time, u64 timestamp)
{
	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_CREATE_TIME_ID;

	if (create_time == 0 || create_time >= U64_MAX) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted create_time: "
			  "peb_id %llu, start_page %u, "
			  "create_time %llx\n",
			  log->peb_id, log->start_page,
			  create_time);
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

	if (create_time > timestamp) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted create_time: "
			  "peb_id %llu, start_page %u, "
			  "create_time %llx, timestamp %llx\n",
			  log->peb_id, log->start_page,
			  create_time, timestamp);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_seg_header_create_time_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;

	seg_hdr = &log->header.seg_hdr;

	return ssdfs_fsck_create_time_check(state, item, metadata_id, log,
				le64_to_cpu(seg_hdr->volume_hdr.create_time),
				le64_to_cpu(seg_hdr->timestamp));
}

static
int ssdfs_fsck_create_cno_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u64 create_cno)
{
	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_CREATE_CNO_ID;

	if (create_cno != 0) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted create_cno: "
			  "peb_id %llu, start_page %u, "
			  "create_cno %llx\n",
			  log->peb_id, log->start_page, create_cno);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_seg_header_create_cno_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;

	seg_hdr = &log->header.seg_hdr;

	return ssdfs_fsck_create_cno_check(state, item, metadata_id, log,
				le64_to_cpu(seg_hdr->volume_hdr.create_cno));
}

static
int ssdfs_fsck_volume_header_flags_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u32 flags)
{
	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_FLAGS_ID;

	if (flags & ~SSDFS_VH_FLAGS_MASK) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted flags: "
			  "peb_id %llu, start_page %u, "
			  "flags %#x\n",
			  log->peb_id, log->start_page,
			  flags);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (is_zoned_device(state->base.fd)) {
		if (!(flags & SSDFS_VH_ZNS_BASED_VOLUME)) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted flags: "
				  "peb_id %llu, start_page %u, "
				  "flags %#x\n",
				  log->peb_id, log->start_page,
				  flags);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}
	} else {
		if (flags & SSDFS_VH_ZNS_BASED_VOLUME ||
		    flags & SSDFS_VH_UNALIGNED_ZONE) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted flags: "
				  "peb_id %llu, start_page %u, "
				  "flags %#x\n",
				  log->peb_id, log->start_page,
				  flags);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_seg_header_flags_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;

	seg_hdr = &log->header.seg_hdr;

	return ssdfs_fsck_volume_header_flags_check(state, item,
					metadata_id, log,
					le32_to_cpu(seg_hdr->volume_hdr.flags));
}

int ssdfs_fsck_lebs_per_peb_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u8 log_erasesize,
				u16 pebs_per_seg,
				u32 lebs_per_peb_index)
{
	u64 erase_size;
	u64 pebs_count;
	u64 calculated;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_LEBS_PER_PEB_INDEX_ID;

	if (lebs_per_peb_index == 0 || lebs_per_peb_index >= U32_MAX) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted lebs_per_peb_index: "
			  "peb_id %llu, start_page %u, "
			  "lebs_per_peb_index %u\n",
			  log->peb_id, log->start_page,
			  lebs_per_peb_index);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (pebs_per_seg == 1 && lebs_per_peb_index != 1) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted lebs_per_peb_index: "
			  "peb_id %llu, start_page %u, "
			  "lebs_per_peb_index %u\n",
			  log->peb_id, log->start_page,
			  lebs_per_peb_index);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	erase_size = ssdfs_fsck_log2_to_size(log_erasesize);

	/*
	 * Erase size is calculated from on-disk log_erasesize.
	 * Corrupted log_erasesize can give zero erase size or erase size
	 * bigger than the volume. It is the corruption of the item but
	 * not the failure of the check.
	 */
	if (erase_size == 0) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted erase_size: "
			  "peb_id %llu, start_page %u, "
			  "log_erasesize %u\n",
			  log->peb_id, log->start_page,
			  log_erasesize);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	pebs_count = state->base.fs_size / erase_size;

	if (pebs_count == 0 || pebs_count >= U64_MAX) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted pebs_count: "
			  "peb_id %llu, start_page %u, "
			  "erase_size %llu, fs_size %llu, "
			  "pebs_count %llu\n",
			  log->peb_id, log->start_page,
			  erase_size, state->base.fs_size,
			  pebs_count);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	calculated = pebs_count / lebs_per_peb_index;

	if (calculated == 0 || calculated >= U32_MAX) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted lebs_per_peb_index: "
			  "peb_id %llu, start_page %u, "
			  "lebs_per_peb_index %u\n",
			  log->peb_id, log->start_page,
			  lebs_per_peb_index);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	calculated = pebs_count % lebs_per_peb_index;

	if (calculated > 0) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted lebs_per_peb_index: "
			  "peb_id %llu, start_page %u, "
			  "lebs_per_peb_index %u\n",
			  log->peb_id, log->start_page,
			  lebs_per_peb_index);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_seg_header_lebs_per_peb_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;
	u8 log_erasesize;
	u16 pebs_per_seg;
	u32 lebs_per_peb_index;

	seg_hdr = &log->header.seg_hdr;
	log_erasesize = seg_hdr->volume_hdr.log_erasesize;
	pebs_per_seg = le16_to_cpu(seg_hdr->volume_hdr.pebs_per_seg);
	lebs_per_peb_index = le32_to_cpu(seg_hdr->volume_hdr.lebs_per_peb_index);

	return ssdfs_fsck_lebs_per_peb_check(state, item, metadata_id, log,
					     log_erasesize, pebs_per_seg,
					     lebs_per_peb_index);
}

static inline
int is_leb2peb_pair_valid(struct ssdfs_leb2peb_pair *pair,
			  u64 pebs_count)
{
	u64 leb_id;
	u64 peb_id;
	int is_leb_valid = SSDFS_FALSE;
	int is_peb_valid = SSDFS_FALSE;

	leb_id = le64_to_cpu(pair->leb_id);
	peb_id = le64_to_cpu(pair->peb_id);

	if (leb_id > SSDFS_INITIAL_SNAPSHOT_SEG_LEB_ID &&
	    leb_id < pebs_count) {
		is_leb_valid = SSDFS_TRUE;
	}

	if (peb_id > SSDFS_INITIAL_SNAPSHOT_SEG_PEB_ID &&
	    peb_id < pebs_count) {
		is_peb_valid = SSDFS_TRUE;
	}

	if (is_leb_valid == SSDFS_TRUE && is_peb_valid == SSDFS_TRUE)
		return SSDFS_TRUE;

	return SSDFS_FALSE;
}

static
int ssdfs_fsck_seg_header_sb_pebs_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;
	u64 erase_size;
	u64 pebs_count;
	int i, j;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	seg_hdr = &log->header.seg_hdr;

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_SB_PEBS_ID;

	erase_size = ssdfs_fsck_log2_to_size(seg_hdr->volume_hdr.log_erasesize);

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

	pebs_count = state->base.fs_size / erase_size;

	if (pebs_count == 0 || pebs_count >= U64_MAX) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted pebs_count: "
			  "peb_id %llu, start_page %u, "
			  "erase_size %llu, fs_size %llu, "
			  "pebs_count %llu\n",
			  log->peb_id, log->start_page,
			  erase_size, state->base.fs_size,
			  pebs_count);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	for (i = 0; i < SSDFS_SB_CHAIN_MAX; i++) {
		int is_peb_valid[SSDFS_SB_SEG_COPY_MAX] = {
			SSDFS_FALSE,
			SSDFS_FALSE
		};

		for (j = 0; j < SSDFS_SB_SEG_COPY_MAX; j++) {
			struct ssdfs_leb2peb_pair *pair;

			pair = &seg_hdr->volume_hdr.sb_pebs[i][j];

			is_peb_valid[j] = is_leb2peb_pair_valid(pair, pebs_count);
		}

		if (is_peb_valid[SSDFS_MAIN_SB_SEG] !=
					is_peb_valid[SSDFS_COPY_SB_SEG]) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted leb2peb pair: "
				  "peb_id %llu, start_page %u, "
				  "index %d\n",
				  log->peb_id, log->start_page, i);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		switch (i) {
		case SSDFS_CUR_SB_SEG:
		case SSDFS_NEXT_SB_SEG:
			if (!is_peb_valid[SSDFS_MAIN_SB_SEG]) {
				item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
				SSDFS_DBG(state->base.show_debug,
					  "corrupted leb2peb pair: "
					  "peb_id %llu, start_page %u, "
					  "index %d\n",
					  log->peb_id, log->start_page, i);
				return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
			}
			break;

		default:
			/* do nothing */
			break;
		}
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

/*
 * ssdfs_fsck_meta_extent_check() - check one metadata extent
 * @state: pointer on thread state
 * @item: metadata item state
 * @log: found log descriptor
 * @extents: metadata extents
 * @row: extent's index in the chain
 * @chain: chain type (main or copy)
 * @total_segs: total segments count of the volume
 */
static
int ssdfs_fsck_meta_extent_check(struct ssdfs_thread_state *state,
				 struct ssdfs_fsck_metadata_item_state *item,
				 struct ssdfs_fsck_found_log *log,
				 struct ssdfs_fsck_meta_extents *extents,
				 u32 row, u32 chain, u64 total_segs)
{
	struct ssdfs_meta_area_extent *extent;
	u64 start_id;
	u32 len;
	u16 type;
	u16 flags;

	extent = ssdfs_fsck_meta_extent(extents, row, chain);
	start_id = le64_to_cpu(extent->start_id);
	len = le32_to_cpu(extent->len);
	type = le16_to_cpu(extent->type);
	flags = le16_to_cpu(extent->flags);

	if (type != SSDFS_SEG_EXTENT_TYPE || flags != 0 ||
	    start_id == 0 || start_id >= total_segs ||
	    len == 0 || len > (total_segs - start_id)) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted %s extent: "
			  "peb_id %llu, start_page %u, "
			  "row %u, chain %u, "
			  "extent (start %llu, len %u, type %#x, flags %#x), "
			  "total_segs %llu\n",
			  extents->name,
			  log->peb_id, log->start_page,
			  row, chain,
			  start_id, len, type, flags,
			  total_segs);
		return -EIO;
	}

	return 0;
}

/*
 * ssdfs_fsck_meta_extents_intersect() - do metadata extents intersect?
 */
static inline
int ssdfs_fsck_meta_extents_intersect(struct ssdfs_meta_area_extent *extent1,
				      struct ssdfs_meta_area_extent *extent2)
{
	u64 start1 = le64_to_cpu(extent1->start_id);
	u64 end1 = start1 + le32_to_cpu(extent1->len);
	u64 start2 = le64_to_cpu(extent2->start_id);
	u64 end2 = start2 + le32_to_cpu(extent2->len);

	return start1 < end2 && start2 < end1;
}

/*
 * ssdfs_fsck_meta_extents_check() - check metadata extents of volume header
 * @state: pointer on thread state
 * @item: metadata item state
 * @log: found log descriptor
 * @extents: metadata extents
 * @total_segs: total segments count of the volume
 * @segs_count: segments count of main chain in volume header [out]
 * @may_overflow: can the chain continue in overflow extents? [out]
 *
 * The method follows the logic of the driver: the main chain ends
 * with the first empty extent; the copy chain (if any) has to
 * contain the extent of the same length in every row. Additionally,
 * the method checks that the chain has no holes and that extents
 * don't intersect each other. The volume header keeps only the first
 * @rows extents, the rest of extents is stored in the superblock
 * segment. If all rows are used, then the chain may continue.
 *
 * RETURN:
 * [success] - extents are valid.
 * [failure] - extents are corrupted.
 */
static
int ssdfs_fsck_meta_extents_check(struct ssdfs_thread_state *state,
				  struct ssdfs_fsck_metadata_item_state *item,
				  struct ssdfs_fsck_found_log *log,
				  struct ssdfs_fsck_meta_extents *extents,
				  u64 total_segs,
				  u64 *segs_count, int *may_overflow)
{
	struct ssdfs_meta_area_extent *extent1, *extent2;
	u32 chains = extents->has_copy ? extents->chains : 1;
	u32 used_rows;
	u32 i, j, k, l;
	int err;

	*segs_count = 0;
	*may_overflow = SSDFS_FALSE;

	for (used_rows = 0; used_rows < extents->rows; used_rows++) {
		extent1 = ssdfs_fsck_meta_extent(extents, used_rows, 0);

		if (le16_to_cpu(extent1->type) == SSDFS_EMPTY_EXTENT_TYPE)
			break;

		for (j = 0; j < chains; j++) {
			err = ssdfs_fsck_meta_extent_check(state, item, log,
							   extents, used_rows,
							   j, total_segs);
			if (err)
				return err;
		}

		extent2 = ssdfs_fsck_meta_extent(extents, used_rows,
						 chains - 1);

		if (extent1->len != extent2->len) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "different %s main and copy extents: "
				  "peb_id %llu, start_page %u, row %u, "
				  "len1 %u, len2 %u\n",
				  extents->name,
				  log->peb_id, log->start_page, used_rows,
				  le32_to_cpu(extent1->len),
				  le32_to_cpu(extent2->len));
			return -EIO;
		}

		*segs_count += le32_to_cpu(extent1->len);
	}

	if (used_rows == 0) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "empty %s extents: "
			  "peb_id %llu, start_page %u\n",
			  extents->name,
			  log->peb_id, log->start_page);
		return -EIO;
	}

	for (i = used_rows; i < extents->rows; i++) {
		for (j = 0; j < chains; j++) {
			extent1 = ssdfs_fsck_meta_extent(extents, i, j);

			if (le16_to_cpu(extent1->type) ==
						SSDFS_EMPTY_EXTENT_TYPE)
				continue;

			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "%s extent after empty one: "
				  "peb_id %llu, start_page %u, "
				  "row %u, chain %u, type %#x\n",
				  extents->name,
				  log->peb_id, log->start_page,
				  i, j, le16_to_cpu(extent1->type));
			return -EIO;
		}
	}

	for (i = 0; i < used_rows; i++) {
		for (j = 0; j < chains; j++) {
			extent1 = ssdfs_fsck_meta_extent(extents, i, j);

			for (k = i; k < used_rows; k++) {
				for (l = (k == i) ? j + 1 : 0;
				     l < chains; l++) {
					extent2 = ssdfs_fsck_meta_extent(extents,
									 k, l);

					if (!ssdfs_fsck_meta_extents_intersect(extent1,
									       extent2))
						continue;

					item->state =
					    SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
					SSDFS_DBG(state->base.show_debug,
						  "intersected %s extents: "
						  "peb_id %llu, start_page %u, "
						  "extent1 (row %u, chain %u, "
						  "start %llu, len %u), "
						  "extent2 (row %u, chain %u, "
						  "start %llu, len %u)\n",
						  extents->name,
						  log->peb_id, log->start_page,
						  i, j,
						  le64_to_cpu(extent1->start_id),
						  le32_to_cpu(extent1->len),
						  k, l,
						  le64_to_cpu(extent2->start_id),
						  le32_to_cpu(extent2->len));
					return -EIO;
				}
			}
		}
	}

	*may_overflow = (used_rows == extents->rows);

	return 0;
}

/*
 * ssdfs_fsck_meta_segs_count_check() - check segments count of extents
 * @state: pointer on thread state
 * @item: metadata item state
 * @log: found log descriptor
 * @name: name of metadata structure
 * @expected: segments count of the chain
 * @found: segments count of the chain in volume header
 * @may_overflow: can the chain continue in overflow extents?
 */
static
int ssdfs_fsck_meta_segs_count_check(struct ssdfs_thread_state *state,
				     struct ssdfs_fsck_metadata_item_state *item,
				     struct ssdfs_fsck_found_log *log,
				     const char *name,
				     u64 expected, u64 found,
				     int may_overflow)
{
	if (found == expected || (may_overflow && found < expected))
		return 0;

	item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
	SSDFS_DBG(state->base.show_debug,
		  "corrupted %s extents: "
		  "peb_id %llu, start_page %u, "
		  "segments in extents %llu, expected %llu\n",
		  name, log->peb_id, log->start_page,
		  found, expected);
	return -EIO;
}

/*
 * ssdfs_fsck_compression_flags_count() - number of compression flags
 */
static inline
int ssdfs_fsck_compression_flags_count(u16 flags, u16 compr_mask)
{
	return __builtin_popcount(flags & compr_mask);
}

#define SSDFS_FSCK_SEGBMAP_COMPR_MASK \
	(SSDFS_SEGBMAP_MAKE_ZLIB_COMPR | SSDFS_SEGBMAP_MAKE_LZO_COMPR | \
	 SSDFS_SEGBMAP_MAKE_LZ4_COMPR | SSDFS_SEGBMAP_MAKE_ZSTD_COMPR)

/*
 * ssdfs_fsck_seg_header_segbmap_check() - check segment bitmap header
 */
static
int ssdfs_fsck_seg_header_segbmap_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segbmap_sb_header *segbmap;
	u32 erase_size = state->base.erase_size;
	u32 seg_size = state->base.seg_size;
	u32 pebs_per_seg = seg_size / erase_size;
	u64 total_segs = state->base.fs_size / seg_size;
	struct ssdfs_fsck_meta_extents extents;
	u16 flags;
	u16 fragments_count;
	u16 fragments_per_seg;
	u16 fragments_per_peb;
	u16 fragment_size;
	u16 segs_count;
	u32 bytes_count;
	u64 found_segs;
	int may_overflow;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_SEGBMAP_ID;

	segbmap = &log->header.seg_hdr.volume_hdr.segbmap;

	flags = le16_to_cpu(segbmap->flags);

	if (flags & ~SSDFS_SEGBMAP_FLAGS_MASK ||
	    flags & SSDFS_SEGBMAP_ERROR ||
	    ssdfs_fsck_compression_flags_count(flags,
				SSDFS_FSCK_SEGBMAP_COMPR_MASK) > 1) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted segbmap's flags: "
			  "peb_id %llu, start_page %u, "
			  "flags %#x\n",
			  log->peb_id, log->start_page,
			  flags);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	bytes_count = le32_to_cpu(segbmap->bytes_count);

	if (bytes_count != SEG_BMAP_BYTES(total_segs)) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted bytes_count: "
			  "peb_id %llu, start_page %u, "
			  "bytes_count %u, total_segs %llu, "
			  "calculated %u\n",
			  log->peb_id, log->start_page,
			  bytes_count, total_segs,
			  SEG_BMAP_BYTES(total_segs));
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	fragment_size = le16_to_cpu(segbmap->fragment_size);

	if (fragment_size != PAGE_CACHE_SIZE) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted fragment_size: "
			  "peb_id %llu, start_page %u, fragment_size %u\n",
			  log->peb_id, log->start_page,
			  fragment_size);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	fragments_count = le16_to_cpu(segbmap->fragments_count);

	if (fragments_count !=
		SEG_BMAP_FRAGMENTS(total_segs, fragment_size)) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted fragments_count: "
			  "peb_id %llu, start_page %u, "
			  "fragments_count %u, calculated %u\n",
			  log->peb_id, log->start_page,
			  fragments_count,
			  SEG_BMAP_FRAGMENTS(total_segs, fragment_size));
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	fragments_per_peb = le16_to_cpu(segbmap->fragments_per_peb);
	fragments_per_seg = le16_to_cpu(segbmap->fragments_per_seg);

	if (fragments_per_peb == 0 ||
	    ((u64)fragments_per_peb * fragment_size) > erase_size ||
	    fragments_per_seg == 0 ||
	    ((u64)fragments_per_seg * fragment_size) > seg_size ||
	    fragments_per_seg % fragments_per_peb ||
	    (fragments_per_seg / fragments_per_peb) > pebs_per_seg) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted segment bitmap: "
			  "peb_id %llu, start_page %u, "
			  "fragments_per_peb %u, fragments_per_seg %u, "
			  "erase_size %u, seg_size %u, pebs_per_seg %u\n",
			  log->peb_id, log->start_page,
			  fragments_per_peb, fragments_per_seg,
			  erase_size, seg_size, pebs_per_seg);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	segs_count = le16_to_cpu(segbmap->segs_count);

	if (segs_count == 0 || segs_count >= U16_MAX ||
	    ((u64)fragments_per_seg * segs_count) < fragments_count) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted segs_count: "
			  "peb_id %llu, start_page %u, "
			  "segs_count %u, fragments_per_seg %u, "
			  "fragments_count %u\n",
			  log->peb_id, log->start_page,
			  segs_count, fragments_per_seg,
			  fragments_count);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	extents.name = "segbmap";
	extents.array = &segbmap->extents[0][0];
	extents.rows = SSDFS_SEGBMAP_RESERVED_EXTENTS;
	extents.chains = SSDFS_SEGBMAP_SEG_COPY_MAX;
	extents.has_copy = flags & SSDFS_SEGBMAP_HAS_COPY;

	if (ssdfs_fsck_meta_extents_check(state, item, log, &extents,
					  total_segs,
					  &found_segs, &may_overflow)) {
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (ssdfs_fsck_meta_segs_count_check(state, item, log,
					     extents.name, segs_count,
					     found_segs, may_overflow)) {
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

#define SSDFS_FSCK_MAPTBL_COMPR_MASK \
	(SSDFS_MAPTBL_MAKE_ZLIB_COMPR | SSDFS_MAPTBL_MAKE_LZO_COMPR | \
	 SSDFS_MAPTBL_MAKE_LZ4_COMPR | SSDFS_MAPTBL_MAKE_ZSTD_COMPR)

/*
 * ssdfs_fsck_maptbl_fragments_check() - check capacity of fragments
 *
 * The fragments have to describe all LEBs (PEBs) of the volume.
 * The last fragment(s) can be partially used (the driver's logic
 * of ssdfs_check_maptbl_sb_header()).
 */
static inline
int ssdfs_fsck_maptbl_fragments_check(u64 count, u32 fragments_count,
				      u16 per_fragment)
{
	u64 calculated = (u64)fragments_count * per_fragment;

	return per_fragment != 0 && count <= calculated &&
		calculated <= (count + (2 * (u64)per_fragment));
}

/*
 * ssdfs_fsck_seg_header_maptbl_check() - check mapping table header
 */
static
int ssdfs_fsck_seg_header_maptbl_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_maptbl_sb_header *maptbl;
	u32 erase_size = state->base.erase_size;
	u32 seg_size = state->base.seg_size;
	u32 pebs_per_seg = seg_size / erase_size;
	u64 total_segs = state->base.fs_size / seg_size;
	struct ssdfs_fsck_meta_extents extents;
	u16 flags;
	u32 fragments_count;
	u32 fragment_bytes;
	u16 fragments_per_seg;
	u16 fragments_per_peb;
	u16 lebs_per_fragment;
	u16 pebs_per_fragment;
	u16 pebs_per_stripe;
	u16 stripes_per_fragment;
	u64 lebs_count;
	u64 pebs_count;
	u64 calculated;
	u64 segs_per_copy;
	u64 found_segs;
	int may_overflow;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_MAPTBL_ID;

	maptbl = &log->header.seg_hdr.volume_hdr.maptbl;

	flags = le16_to_cpu(maptbl->flags);

	/*
	 * The driver clears SSDFS_MAPTBL_UNDER_FLUSH and
	 * SSDFS_MAPTBL_START_MIGRATION flags before storing
	 * the volume header.
	 */
	if (flags & ~SSDFS_MAPTBL_FLAGS_MASK ||
	    flags & SSDFS_MAPTBL_ERROR ||
	    flags & SSDFS_MAPTBL_UNDER_FLUSH ||
	    flags & SSDFS_MAPTBL_START_MIGRATION ||
	    ssdfs_fsck_compression_flags_count(flags,
				SSDFS_FSCK_MAPTBL_COMPR_MASK) > 1) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted maptbl's flags: "
			  "peb_id %llu, start_page %u, "
			  "flags %#x\n",
			  log->peb_id, log->start_page,
			  flags);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	fragment_bytes = le32_to_cpu(maptbl->fragment_bytes);
	fragments_per_peb = le16_to_cpu(maptbl->fragments_per_peb);
	fragments_per_seg = le16_to_cpu(maptbl->fragments_per_seg);

	if (fragment_bytes == 0 || fragments_per_peb == 0 ||
	    ((u64)fragments_per_peb * fragment_bytes) > erase_size ||
	    ((u64)fragments_per_seg * fragment_bytes) > seg_size ||
	    ((u64)fragments_per_peb * pebs_per_seg) != fragments_per_seg) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted mapping table: "
			  "peb_id %llu, start_page %u, "
			  "fragment_bytes %u, fragments_per_peb %u, "
			  "fragments_per_seg %u, erase_size %u, "
			  "seg_size %u, pebs_per_seg %u\n",
			  log->peb_id, log->start_page,
			  fragment_bytes, fragments_per_peb,
			  fragments_per_seg, erase_size,
			  seg_size, pebs_per_seg);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	lebs_count = le64_to_cpu(maptbl->lebs_count);
	pebs_count = le64_to_cpu(maptbl->pebs_count);
	calculated = total_segs * pebs_per_seg;

	if (lebs_count != calculated || pebs_count != calculated) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted mapping table: "
			  "peb_id %llu, start_page %u, "
			  "lebs_count %llu, pebs_count %llu, "
			  "calculated %llu\n",
			  log->peb_id, log->start_page,
			  lebs_count, pebs_count, calculated);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	fragments_count = le32_to_cpu(maptbl->fragments_count);
	lebs_per_fragment = le16_to_cpu(maptbl->lebs_per_fragment);
	pebs_per_fragment = le16_to_cpu(maptbl->pebs_per_fragment);

	if (fragments_count == 0 ||
	    !ssdfs_fsck_maptbl_fragments_check(lebs_count, fragments_count,
					       lebs_per_fragment) ||
	    !ssdfs_fsck_maptbl_fragments_check(pebs_count, fragments_count,
					       pebs_per_fragment)) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted mapping table: "
			  "peb_id %llu, start_page %u, "
			  "fragments_count %u, lebs_per_fragment %u, "
			  "pebs_per_fragment %u, lebs_count %llu, "
			  "pebs_count %llu\n",
			  log->peb_id, log->start_page,
			  fragments_count, lebs_per_fragment,
			  pebs_per_fragment, lebs_count, pebs_count);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	pebs_per_stripe = le16_to_cpu(maptbl->pebs_per_stripe);
	stripes_per_fragment = le16_to_cpu(maptbl->stripes_per_fragment);

	/*
	 * The last stripe of fragment can be partially used:
	 * pebs_per_stripe = DIV_ROUND_UP(pebs_per_fragment,
	 *                                stripes_per_fragment)
	 */
	if (pebs_per_stripe == 0 || stripes_per_fragment == 0 ||
	    pebs_per_stripe != ((pebs_per_fragment +
				stripes_per_fragment - 1) /
					stripes_per_fragment)) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted mapping table: "
			  "peb_id %llu, start_page %u, "
			  "pebs_per_stripe %u, stripes_per_fragment %u, "
			  "pebs_per_fragment %u\n",
			  log->peb_id, log->start_page,
			  pebs_per_stripe, stripes_per_fragment,
			  pebs_per_fragment);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	extents.name = "maptbl";
	extents.array = &maptbl->extents[0][0];
	extents.rows = SSDFS_MAPTBL_RESERVED_EXTENTS;
	extents.chains = SSDFS_MAPTBL_SEG_COPY_MAX;
	extents.has_copy = flags & SSDFS_MAPTBL_HAS_COPY;

	if (ssdfs_fsck_meta_extents_check(state, item, log, &extents,
					  total_segs,
					  &found_segs, &may_overflow)) {
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	segs_per_copy = ((u64)fragments_count + fragments_per_seg - 1) /
							fragments_per_seg;

	if (ssdfs_fsck_meta_segs_count_check(state, item, log,
					     extents.name, segs_per_copy,
					     found_segs, may_overflow)) {
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

int ssdfs_fsck_check_log_pages(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				struct ssdfs_fsck_found_log *log,
				u64 pages_per_peb, u16 log_pages)
{
	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, "
		  "pages_per_peb %llu, log_pages %u\n",
		  log->peb_id, log->start_page,
		  pages_per_peb, log_pages);

	if (pages_per_peb < SSDFS_MIN_PAGES_PER_PEB ||
	    pages_per_peb > SSDFS_MAX_PAGES_PER_PEB) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect pages_per_peb: "
			  "peb_id %llu, start_page %u, "
			  "pages_per_peb %llu, min_pages_per_peb %u, "
			  "max_pages_per_peb %u\n",
			  log->peb_id, log->start_page,
			  pages_per_peb, SSDFS_MIN_PAGES_PER_PEB,
			  SSDFS_MAX_PAGES_PER_PEB);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (log_pages < SSDFS_LOG_MIN_PAGES || log_pages > pages_per_peb) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect log_pages: "
			  "peb_id %llu, start_page %u, "
			  "pages_per_peb %llu, log_pages %u\n",
			  log->peb_id, log->start_page,
			  pages_per_peb, log_pages);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (log_pages % SSDFS_LOG_MIN_PAGES) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect log_pages: "
			  "peb_id %llu, start_page %u, "
			  "pages_per_peb %llu, log_pages %u\n",
			  log->peb_id, log->start_page,
			  pages_per_peb, log_pages);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_seg_header_log_pages_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;
	u64 erase_size;
	u64 page_size;
	u64 pages_per_peb;
	u16 log_pages;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_LOG_PAGES_ID;

	seg_hdr = &log->header.seg_hdr;

	erase_size = ssdfs_fsck_log2_to_size(seg_hdr->volume_hdr.log_erasesize);

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

	page_size = ssdfs_fsck_log2_to_size(seg_hdr->volume_hdr.log_pagesize);

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

	log_pages = le16_to_cpu(seg_hdr->volume_hdr.sb_seg_log_pages);

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

	log_pages = le16_to_cpu(seg_hdr->volume_hdr.segbmap_log_pages);

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

	log_pages = le16_to_cpu(seg_hdr->volume_hdr.maptbl_log_pages);

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

	log_pages = le16_to_cpu(seg_hdr->volume_hdr.lnodes_seg_log_pages);

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

	log_pages = le16_to_cpu(seg_hdr->volume_hdr.hnodes_seg_log_pages);

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

	log_pages = le16_to_cpu(seg_hdr->volume_hdr.inodes_seg_log_pages);

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

	log_pages = le16_to_cpu(seg_hdr->volume_hdr.user_data_log_pages);

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

	log_pages = le16_to_cpu(seg_hdr->log_pages);

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

int ssdfs_fsck_create_threads_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u16 pebs_per_seg,
				u8 log_segsize,
				u8 log_erasesize,
				u16 create_threads_per_seg)
{
	u64 seg_size;
	u64 erase_size;
	u64 calculated;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_CREATE_THREADS_ID;

	if (pebs_per_seg == 0 || pebs_per_seg >= U16_MAX) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted pebs_per_seg: "
			  "peb_id %llu, start_page %u, "
			  "pebs_per_seg %u\n",
			  log->peb_id, log->start_page,
			  pebs_per_seg);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	seg_size = ssdfs_fsck_log2_to_size(log_segsize);
	erase_size = ssdfs_fsck_log2_to_size(log_erasesize);

	if (seg_size == 0 || erase_size == 0) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted size: "
			  "peb_id %llu, start_page %u, "
			  "seg_size %llu, erase_size %llu\n",
			  log->peb_id, log->start_page,
			  seg_size, erase_size);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (seg_size < erase_size) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect seg_size: "
			  "peb_id %llu, start_page %u, "
			  "seg_size %llu, erase_size %llu\n",
			  log->peb_id, log->start_page,
			  seg_size, erase_size);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	calculated = seg_size / erase_size;

	if (calculated == 0) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect pebs_per_seg: "
			  "peb_id %llu, start_page %u, "
			  "seg_size %llu, erase_size %llu, "
			  "pebs_per_seg %llu\n",
			  log->peb_id, log->start_page,
			  seg_size, erase_size,
			  calculated);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (pebs_per_seg != calculated) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect pebs_per_seg: "
			  "peb_id %llu, start_page %u, "
			  "seg_size %llu, erase_size %llu, "
			  "pebs_per_seg %u, calculated %llu\n",
			  log->peb_id, log->start_page,
			  seg_size, erase_size,
			  pebs_per_seg, calculated);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (create_threads_per_seg == 0 ||
	    create_threads_per_seg > pebs_per_seg) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "incorrect create_threads_per_seg: "
			  "peb_id %llu, start_page %u, "
			  "seg_size %llu, erase_size %llu, "
			  "pebs_per_seg %u, create_threads_per_seg %u\n",
			  log->peb_id, log->start_page,
			  seg_size, erase_size,
			  pebs_per_seg,
			  create_threads_per_seg);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_seg_header_create_threads_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;
	u16 pebs_per_seg;
	u16 create_threads_per_seg;

	seg_hdr = &log->header.seg_hdr;

	pebs_per_seg = le16_to_cpu(seg_hdr->volume_hdr.pebs_per_seg);
	create_threads_per_seg =
		le16_to_cpu(seg_hdr->volume_hdr.create_threads_per_seg);

	return ssdfs_fsck_create_threads_check(state, item, metadata_id, log,
						pebs_per_seg,
						seg_hdr->volume_hdr.log_segsize,
						seg_hdr->volume_hdr.log_erasesize,
						create_threads_per_seg);
}

int ssdfs_fsck_btree_desc_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				struct ssdfs_fsck_found_log *log,
				u8 type,
				struct ssdfs_btree_descriptor *desc)
{
	u64 page_size;
	u64 node_size;
	u16 index_area_min_size;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, type %#x\n",
		  log->peb_id, log->start_page, type);

	index_area_min_size = le16_to_cpu(desc->index_area_min_size);

	switch (type) {
	case SSDFS_INODES_BTREE:
		if (desc->type != SSDFS_INODES_BTREE ||
		    le32_to_cpu(desc->magic) != SSDFS_INODES_BTREE_MAGIC) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "magic %#x, type %#x\n",
				  log->peb_id, log->start_page,
				  le32_to_cpu(desc->magic),
				  desc->type);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		if (le16_to_cpu(desc->item_size) != sizeof(struct ssdfs_inode)) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "magic %#x, item_size %u\n",
				  log->peb_id, log->start_page,
				  le32_to_cpu(desc->magic),
				  le16_to_cpu(desc->item_size));
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		if (index_area_min_size < sizeof(struct ssdfs_inode)) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "index_area_min_size %u\n",
				  log->peb_id, log->start_page,
				  index_area_min_size);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}
		break;

	case SSDFS_DENTRIES_BTREE:
		if (desc->type != SSDFS_DENTRIES_BTREE ||
		    le32_to_cpu(desc->magic) != SSDFS_DENTRIES_BTREE_MAGIC) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "magic %#x, type %#x\n",
				  log->peb_id, log->start_page,
				  le32_to_cpu(desc->magic),
				  desc->type);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		if (le16_to_cpu(desc->item_size) !=
					sizeof(struct ssdfs_dir_entry)) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "magic %#x, item_size %u\n",
				  log->peb_id, log->start_page,
				  le32_to_cpu(desc->magic),
				  le16_to_cpu(desc->item_size));
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		if (index_area_min_size <
			    sizeof(struct ssdfs_dentries_btree_node_header)) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "index_area_min_size %u\n",
				  log->peb_id, log->start_page,
				  index_area_min_size);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}
		break;

	case SSDFS_EXTENTS_BTREE:
		if (desc->type != SSDFS_EXTENTS_BTREE ||
		    le32_to_cpu(desc->magic) != SSDFS_EXTENTS_BTREE_MAGIC) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "magic %#x, type %#x\n",
				  log->peb_id, log->start_page,
				  le32_to_cpu(desc->magic),
				  desc->type);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		if (le16_to_cpu(desc->item_size) != sizeof(struct ssdfs_raw_fork)) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "magic %#x, item_size %u\n",
				  log->peb_id, log->start_page,
				  le32_to_cpu(desc->magic),
				  le16_to_cpu(desc->item_size));
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		if (index_area_min_size <
			    sizeof(struct ssdfs_extents_btree_node_header)) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "index_area_min_size %u\n",
				  log->peb_id, log->start_page,
				  index_area_min_size);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}
		break;

	case SSDFS_SHARED_EXTENTS_BTREE:
		if (desc->type != SSDFS_SHARED_EXTENTS_BTREE ||
		    le32_to_cpu(desc->magic) != SSDFS_SHARED_EXTENTS_BTREE_MAGIC) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "magic %#x, type %#x\n",
				  log->peb_id, log->start_page,
				  le32_to_cpu(desc->magic),
				  desc->type);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		if (le16_to_cpu(desc->item_size) != sizeof(struct ssdfs_raw_fork)) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "magic %#x, item_size %u\n",
				  log->peb_id, log->start_page,
				  le32_to_cpu(desc->magic),
				  le16_to_cpu(desc->item_size));
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		if (index_area_min_size <
			    sizeof(struct ssdfs_extents_btree_node_header)) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "index_area_min_size %u\n",
				  log->peb_id, log->start_page,
				  index_area_min_size);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}
		break;

	case SSDFS_XATTR_BTREE:
		if (desc->type != SSDFS_XATTR_BTREE ||
		    le32_to_cpu(desc->magic) != SSDFS_XATTR_BTREE_MAGIC) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "magic %#x, type %#x\n",
				  log->peb_id, log->start_page,
				  le32_to_cpu(desc->magic),
				  desc->type);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		if (le16_to_cpu(desc->item_size) != sizeof(struct ssdfs_xattr_entry)) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "magic %#x, item_size %u\n",
				  log->peb_id, log->start_page,
				  le32_to_cpu(desc->magic),
				  le16_to_cpu(desc->item_size));
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		if (index_area_min_size <
			    sizeof(struct ssdfs_xattrs_btree_node_header)) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "index_area_min_size %u\n",
				  log->peb_id, log->start_page,
				  index_area_min_size);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}
		break;

	case SSDFS_SHARED_XATTR_BTREE:
		if (desc->type != SSDFS_SHARED_XATTR_BTREE ||
		    le32_to_cpu(desc->magic) != SSDFS_SHARED_XATTR_BTREE_MAGIC) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "magic %#x, type %#x\n",
				  log->peb_id, log->start_page,
				  le32_to_cpu(desc->magic),
				  desc->type);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		if (le16_to_cpu(desc->item_size) != sizeof(struct ssdfs_xattr_entry)) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "magic %#x, item_size %u\n",
				  log->peb_id, log->start_page,
				  le32_to_cpu(desc->magic),
				  le16_to_cpu(desc->item_size));
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		if (index_area_min_size <
			    sizeof(struct ssdfs_xattrs_btree_node_header)) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "index_area_min_size %u\n",
				  log->peb_id, log->start_page,
				  index_area_min_size);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}
		break;

	case SSDFS_SHARED_DICTIONARY_BTREE:
		if (desc->type != SSDFS_SHARED_DICTIONARY_BTREE ||
		    le32_to_cpu(desc->magic) != SSDFS_SHARED_DICT_BTREE_MAGIC) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "magic %#x, type %#x\n",
				  log->peb_id, log->start_page,
				  le32_to_cpu(desc->magic),
				  desc->type);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		if (le16_to_cpu(desc->item_size) != SSDFS_MAX_NAME_LEN) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "magic %#x, item_size %u\n",
				  log->peb_id, log->start_page,
				  le32_to_cpu(desc->magic),
				  le16_to_cpu(desc->item_size));
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		if (index_area_min_size <
			    sizeof(struct ssdfs_shared_dictionary_node_header)) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "index_area_min_size %u\n",
				  log->peb_id, log->start_page,
				  index_area_min_size);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}
		break;

	case SSDFS_SNAPSHOTS_BTREE:
		if (desc->type != SSDFS_SNAPSHOTS_BTREE ||
		    le32_to_cpu(desc->magic) != SSDFS_SNAPSHOTS_BTREE_MAGIC) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "magic %#x, type %#x\n",
				  log->peb_id, log->start_page,
				  le32_to_cpu(desc->magic),
				  desc->type);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		if (le16_to_cpu(desc->item_size) != sizeof(struct ssdfs_snapshot)) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "magic %#x, item_size %u\n",
				  log->peb_id, log->start_page,
				  le32_to_cpu(desc->magic),
				  le16_to_cpu(desc->item_size));
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		if (index_area_min_size <
			    sizeof(struct ssdfs_snapshots_btree_node_header)) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "index_area_min_size %u\n",
				  log->peb_id, log->start_page,
				  index_area_min_size);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}
		break;

	case SSDFS_INVALIDATED_EXTENTS_BTREE:
		if (desc->type != SSDFS_INVALIDATED_EXTENTS_BTREE ||
		    le32_to_cpu(desc->magic) != SSDFS_INVEXT_BTREE_MAGIC) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "magic %#x, type %#x\n",
				  log->peb_id, log->start_page,
				  le32_to_cpu(desc->magic),
				  desc->type);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		if (le16_to_cpu(desc->item_size) != sizeof(struct ssdfs_raw_extent)) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "magic %#x, item_size %u\n",
				  log->peb_id, log->start_page,
				  le32_to_cpu(desc->magic),
				  le16_to_cpu(desc->item_size));
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		if (index_area_min_size <
			    sizeof(struct ssdfs_invextree_node_header)) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted b-tree descriptor: "
				  "peb_id %llu, start_page %u, "
				  "index_area_min_size %u\n",
				  log->peb_id, log->start_page,
				  index_area_min_size);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}
		break;

	default:
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "unexpected b-tree type: "
			  "peb_id %llu, start_page %u, "
			  "type %#x\n",
			  log->peb_id, log->start_page,
			  type);
		return SSDFS_FSCK_CHECK_RESULT_FAILURE;
	}

	if (le16_to_cpu(desc->flags) & ~SSDFS_BTREE_DESC_FLAGS_MASK) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted b-tree descriptor: "
			  "peb_id %llu, start_page %u, "
			  "flags %#x\n",
			  log->peb_id, log->start_page,
			  le16_to_cpu(desc->flags));
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	page_size =
		ssdfs_fsck_log2_to_size(ssdfs_fsck_log_header_log_pagesize(log));

	if (page_size == 0) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted page_size: "
			  "peb_id %llu, start_page %u, "
			  "page_size %llu\n",
			  log->peb_id, log->start_page, page_size);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	node_size = ssdfs_fsck_log2_to_size(desc->log_node_size);

	if (node_size == 0 || node_size < page_size) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted b-tree descriptor: "
			  "peb_id %llu, start_page %u, "
			  "node_size %llu, page_size %llu\n",
			  log->peb_id, log->start_page,
			  node_size, page_size);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (desc->pages_per_node != (node_size / page_size)) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted b-tree descriptor: "
			  "peb_id %llu, start_page %u, "
			  "node_size %llu, page_size %llu, "
			  "pages_per_node %u\n",
			  log->peb_id, log->start_page,
			  node_size, page_size,
			  desc->pages_per_node);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (desc->node_ptr_size != sizeof(struct ssdfs_btree_index_key)) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted b-tree descriptor: "
			  "peb_id %llu, start_page %u, "
			  "node_ptr_size %u\n",
			  log->peb_id, log->start_page,
			  desc->node_ptr_size);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (desc->index_size != sizeof(struct ssdfs_btree_index_key)) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted b-tree descriptor: "
			  "peb_id %llu, start_page %u, "
			  "index_size %u\n",
			  log->peb_id, log->start_page,
			  desc->index_size);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (index_area_min_size > (node_size / 2)) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted b-tree descriptor: "
			  "peb_id %llu, start_page %u, "
			  "index_area_min_size %u\n",
			  log->peb_id, log->start_page,
			  index_area_min_size);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_seg_header_dentries_btree_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;
	struct ssdfs_btree_descriptor *desc;

	seg_hdr = &log->header.seg_hdr;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_DENTRIES_BTREE_ID;

	desc = &seg_hdr->volume_hdr.dentries_btree.desc;

	switch (ssdfs_fsck_btree_desc_check(state, item, log,
					    SSDFS_DENTRIES_BTREE,
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
int ssdfs_fsck_seg_header_extents_btree_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;
	struct ssdfs_btree_descriptor *desc;

	seg_hdr = &log->header.seg_hdr;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_EXTENTS_BTREE_ID;

	desc = &seg_hdr->volume_hdr.extents_btree.desc;

	switch (ssdfs_fsck_btree_desc_check(state, item, log,
					    SSDFS_EXTENTS_BTREE,
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
int ssdfs_fsck_seg_header_xattr_btree_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;
	struct ssdfs_btree_descriptor *desc;

	seg_hdr = &log->header.seg_hdr;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_XATTR_BTREE_ID;

	desc = &seg_hdr->volume_hdr.xattr_btree.desc;

	switch (ssdfs_fsck_btree_desc_check(state, item, log,
					    SSDFS_XATTR_BTREE,
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
int ssdfs_fsck_seg_header_invextree_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;
	struct ssdfs_btree_descriptor *desc;

	seg_hdr = &log->header.seg_hdr;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_INVEXTREE_ID;

	desc = &seg_hdr->volume_hdr.invextree.desc;

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

int ssdfs_fsck_uuid_check(struct ssdfs_thread_state *state,
			  struct ssdfs_fsck_metadata_item_state *item,
			  int metadata_id,
			  struct ssdfs_fsck_found_log *log,
			  const char *uuid_str)
{
	size_t len;
	int upper_bound = SSDFS_UUID_HYPHEN_POSITION_1;
	int i;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_UUID_ID;

	len = strlen(uuid_str);

	if (len != SSDFS_UUID_CHARACTER_SYMBOLS) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted UUID: "
			  "peb_id %llu, start_page %u, "
			  "len %zu\n",
			  log->peb_id, log->start_page,
			  len);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	i = 0;

	while (upper_bound < len) {
		for (; i < upper_bound; i++) {
			if (isxdigit((unsigned char)uuid_str[i]) == 0) {
				item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
				SSDFS_DBG(state->base.show_debug,
					  "corrupted UUID: "
					  "peb_id %llu, start_page %u, "
					  "uuid %s\n",
					  log->peb_id, log->start_page,
					  uuid_str);
				return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
			}
		}

		if (uuid_str[i] != '-') {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted UUID: "
				  "peb_id %llu, start_page %u, "
				  "uuid %s\n",
				  log->peb_id, log->start_page,
				  uuid_str);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		} else
			i++;

		upper_bound += SSDFS_UUID_HYPHEN_POSITION_FACTOR;
		if (upper_bound > SSDFS_UUID_HYPHEN_POSITION_4)
			upper_bound = len;
	}

	for (; i < len; i++) {
		if (isxdigit((unsigned char)uuid_str[i]) == 0) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted UUID: "
				  "peb_id %llu, start_page %u, "
				  "uuid %s\n",
				  log->peb_id, log->start_page,
				  uuid_str);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_seg_header_uuid_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;
	const char *uuid_str;

	seg_hdr = &log->header.seg_hdr;
	uuid_str = uuid_string(seg_hdr->volume_hdr.uuid);

	return ssdfs_fsck_uuid_check(state, item, metadata_id, log, uuid_str);
}

int ssdfs_fsck_log_create_time_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u64 volume_create_time,
				u64 timestamp)
{
	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_LOG_TIMESTAMP_ID;

	if (volume_create_time == 0 || volume_create_time >= U64_MAX) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted create_time: "
			  "peb_id %llu, start_page %u, "
			  "create_time %llx\n",
			  log->peb_id, log->start_page,
			  volume_create_time);
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

	if (volume_create_time > timestamp) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted create_time: "
			  "peb_id %llu, start_page %u, "
			  "create_time %llx, timestamp %llx\n",
			  log->peb_id, log->start_page,
			  volume_create_time, timestamp);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_seg_header_log_create_time_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;
	u64 volume_create_time;
	u64 timestamp;

	seg_hdr = &log->header.seg_hdr;
	volume_create_time = le64_to_cpu(seg_hdr->volume_hdr.create_time);
	timestamp = le64_to_cpu(seg_hdr->timestamp);

	return ssdfs_fsck_log_create_time_check(state, item, metadata_id, log,
						volume_create_time,
						timestamp);
}

int ssdfs_fsck_log_create_cno_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u64 cno)
{
	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_LOG_CNO_ID;

	/* mkfs creates the volume with cno 0 (SSDFS_CREATE_CNO) */
	if (cno >= U64_MAX) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted cno: "
			  "peb_id %llu, start_page %u, "
			  "cno %llx\n",
			  log->peb_id, log->start_page, cno);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_seg_header_log_create_cno_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;
	u64 cno;

	seg_hdr = &log->header.seg_hdr;
	cno = le64_to_cpu(seg_hdr->cno);

	return ssdfs_fsck_log_create_cno_check(state, item, metadata_id, log,
						cno);
}

int ssdfs_fsck_seg_type_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u16 seg_type)
{
	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_SEG_TYPE_ID;

	if (seg_type <= SSDFS_UNKNOWN_SEG_TYPE ||
	    seg_type > SSDFS_LAST_KNOWN_SEG_TYPE) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted seg_type: "
			  "peb_id %llu, start_page %u, "
			  "seg_type %#x\n",
			  log->peb_id, log->start_page, seg_type);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_seg_header_seg_type_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;
	u16 seg_type;

	seg_hdr = &log->header.seg_hdr;
	seg_type = le16_to_cpu(seg_hdr->seg_type);

	return ssdfs_fsck_seg_type_check(state, item, metadata_id, log,
					 seg_type);
}

int ssdfs_fsck_seg_flags_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u32 seg_flags)
{
	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_SEG_FLAGS_ID;

	if (seg_flags & ~SSDFS_SEG_HDR_FLAG_MASK) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted seg_flags: "
			  "peb_id %llu, start_page %u, "
			  "seg_flags %#x\n",
			  log->peb_id, log->start_page, seg_flags);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_seg_header_seg_flags_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;
	u32 seg_flags;

	seg_hdr = &log->header.seg_hdr;
	seg_flags = le32_to_cpu(seg_hdr->seg_flags);

	return ssdfs_fsck_seg_flags_check(state, item, metadata_id, log,
					  seg_flags);
}

int ssdfs_fsck_desc_array_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				struct ssdfs_metadata_descriptor *desc_array,
				size_t array_capacity)
{
	struct ssdfs_metadata_descriptor *desc;
	u8 *data = NULL;
	u8 *uncompr_data = NULL;
	u32 compr_size, uncompr_size;
	size_t lower_threshold = sizeof(struct ssdfs_volume_header);
	u32 offset = 0;
	u32 size = 0;
	u16 flags;
	int processed_items = 0;
	int i;
	int err;
	int res = SSDFS_FSCK_CHECK_RESULT_FAILURE;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_DESC_ARRAY_ID;

	for (i = 0; i < array_capacity; i++) {
		desc = &desc_array[i];

		compr_size = le32_to_cpu(desc->size);

		if (compr_size == 0)
			continue;
		else if (size < compr_size)
			size = compr_size;
	}

	if (size > 0) {
		data = malloc(size);
		if (!data) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_ERR("fail to allocate memory\n");
			return SSDFS_FSCK_CHECK_RESULT_FAILURE;
		}
	} else {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted descriptors array: "
			  "peb_id %llu, start_page %u\n",
			  log->peb_id, log->start_page);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	for (i = 0; i < array_capacity; i++) {
		desc = &desc_array[i];

		size = le32_to_cpu(desc->size);

		if (size == 0)
			continue;

		offset = le32_to_cpu(desc->offset);

		if (offset < lower_threshold) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted descriptors array: "
				  "peb_id %llu, start_page %u, "
				  "desc[%d] offset %u\n",
				  log->peb_id, log->start_page,
				  i, offset);
			res = SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
			goto finish_seg_header_desc_array_check;
		}

		if ((offset + size) > state->peb.peb_size) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted descriptors array: "
				  "peb_id %llu, start_page %u, log_size %u, "
				  "desc[%d] offset %u, size %u\n",
				  log->peb_id, log->start_page,
				  state->peb.log_size,
				  i, offset, size);
			res = SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
			goto finish_seg_header_desc_array_check;
		}

		flags = le16_to_cpu(desc->check.flags);

		if (array_capacity == SSDFS_SEG_HDR_DESC_MAX &&
		    i == SSDFS_LOG_FOOTER_INDEX) {
			/*
			 * The descriptor contains the copy of footer's
			 * checksum that has been calculated with zeroed
			 * checksum field inside of the footer. The footer's
			 * checksum is verified by the footer check.
			 */
			processed_items++;
			continue;
		}

		if (flags & SSDFS_ZLIB_COMPRESSED) {
			compr_size = le32_to_cpu(desc->size);
			uncompr_size = le16_to_cpu(desc->check.bytes);

			if (uncompr_size < compr_size) {
				item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
				SSDFS_DBG(state->base.show_debug,
					  "corrupted descriptors array: "
					  "peb_id %llu, start_page %u, log_size %u, "
					  "desc[%d] offset %u, size %u\n",
					  log->peb_id, log->start_page,
					  state->peb.log_size,
					  i, offset, size);
				res = SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
				goto finish_seg_header_desc_array_check;
			}

			uncompr_data = malloc(uncompr_size);
			if (!uncompr_data) {
				item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
				SSDFS_ERR("fail to allocate memory\n");
				res = SSDFS_FSCK_CHECK_RESULT_FAILURE;
				goto finish_seg_header_desc_array_check;
			}

			err = ssdfs_read_area_content(&state->base,
						      log->peb_id,
						      state->peb.peb_size,
						      offset, compr_size,
						      data);
			if (err) {
				SSDFS_ERR("fail to read block state: "
					  "peb_id %llu, peb_size %u, "
					  "offset %u, size %u, err %d\n",
					  log->peb_id,
					  state->peb.peb_size,
					  offset, compr_size, err);
				item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
				SSDFS_ERR("fail to allocate memory\n");
				res = SSDFS_FSCK_CHECK_RESULT_FAILURE;
				goto finish_seg_header_desc_array_check;
			}

			err = ssdfs_zlib_decompress(data, uncompr_data,
						    compr_size, uncompr_size,
						    state->base.show_debug);
			if (err) {
				SSDFS_ERR("fail to decompress: err %d\n",
					  err);
				res = SSDFS_FSCK_CHECK_RESULT_FAILURE;
				goto finish_seg_header_desc_array_check;
			}

			if (!is_csum_valid(&desc->check,
					   uncompr_data, uncompr_size)) {
				item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
				SSDFS_DBG(state->base.show_debug,
					  "corrupted descriptors array: "
					  "peb_id %llu, start_page %u, log_size %u, "
					  "desc[%d] offset %u, size %u\n",
					  log->peb_id, log->start_page,
					  state->peb.log_size,
					  i, offset, size);
				res = SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
				goto finish_seg_header_desc_array_check;
			}

			free(uncompr_data);
			uncompr_data = NULL;
		} else if (flags & SSDFS_LZO_COMPRESSED) {
			compr_size = le32_to_cpu(desc->size);
			uncompr_size = le16_to_cpu(desc->check.bytes);

			if (uncompr_size < compr_size) {
				item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
				SSDFS_DBG(state->base.show_debug,
					  "corrupted descriptors array: "
					  "peb_id %llu, start_page %u, log_size %u, "
					  "desc[%d] offset %u, size %u\n",
					  log->peb_id, log->start_page,
					  state->peb.log_size,
					  i, offset, size);
				res = SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
				goto finish_seg_header_desc_array_check;
			}

			uncompr_data = malloc(uncompr_size);
			if (!uncompr_data) {
				item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
				SSDFS_ERR("fail to allocate memory\n");
				res = SSDFS_FSCK_CHECK_RESULT_FAILURE;
				goto finish_seg_header_desc_array_check;
			}

			err = ssdfs_read_area_content(&state->base,
						      log->peb_id,
						      state->peb.peb_size,
						      offset, compr_size,
						      data);
			if (err) {
				SSDFS_ERR("fail to read block state: "
					  "peb_id %llu, peb_size %u, "
					  "offset %u, size %u, err %d\n",
					  log->peb_id,
					  state->peb.peb_size,
					  offset, compr_size, err);
				item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
				SSDFS_ERR("fail to allocate memory\n");
				res = SSDFS_FSCK_CHECK_RESULT_FAILURE;
				goto finish_seg_header_desc_array_check;
			}

			err = ssdfs_lzo_decompress(data, uncompr_data,
						   compr_size, uncompr_size,
						   state->base.show_debug);
			if (err) {
				SSDFS_ERR("fail to decompress: err %d\n",
					  err);
				res = SSDFS_FSCK_CHECK_RESULT_FAILURE;
				goto finish_seg_header_desc_array_check;
			}

			if (!is_csum_valid(&desc->check,
					   uncompr_data, uncompr_size)) {
				item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
				SSDFS_DBG(state->base.show_debug,
					  "corrupted descriptors array: "
					  "peb_id %llu, start_page %u, log_size %u, "
					  "desc[%d] offset %u, size %u\n",
					  log->peb_id, log->start_page,
					  state->peb.log_size,
					  i, offset, size);
				res = SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
				goto finish_seg_header_desc_array_check;
			}

			free(uncompr_data);
			uncompr_data = NULL;
		} else if (flags & SSDFS_CRC32) {
			compr_size = le32_to_cpu(desc->size);
			uncompr_size = le16_to_cpu(desc->check.bytes);

			if (uncompr_size != compr_size) {
				item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
				SSDFS_DBG(state->base.show_debug,
					  "corrupted descriptors array: "
					  "peb_id %llu, start_page %u, log_size %u, "
					  "desc[%d] offset %u, size %u\n",
					  log->peb_id, log->start_page,
					  state->peb.log_size,
					  i, offset, size);
				res = SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
				goto finish_seg_header_desc_array_check;
			}

			err = ssdfs_read_area_content(&state->base,
						      log->peb_id,
						      state->peb.peb_size,
						      offset, compr_size,
						      data);
			if (err) {
				SSDFS_ERR("fail to read block state: "
					  "peb_id %llu, peb_size %u, "
					  "offset %u, size %u, err %d\n",
					  log->peb_id,
					  state->peb.peb_size,
					  offset, compr_size, err);
				item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
				SSDFS_ERR("fail to allocate memory\n");
				res = SSDFS_FSCK_CHECK_RESULT_FAILURE;
				goto finish_seg_header_desc_array_check;
			}

			if (!is_csum_valid(&desc->check, data, compr_size)) {
				item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
				SSDFS_DBG(state->base.show_debug,
					  "corrupted descriptors array: "
					  "peb_id %llu, start_page %u, log_size %u, "
					  "desc[%d] offset %u, size %u\n",
					  log->peb_id, log->start_page,
					  state->peb.log_size,
					  i, offset, size);
				res = SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
				goto finish_seg_header_desc_array_check;
			}
		}

		processed_items++;
	}

	if (processed_items <= 0) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted descriptors array: "
			  "peb_id %llu, start_page %u, "
			  "processed_items %d\n",
			  log->peb_id, log->start_page,
			  processed_items);
		res = SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		goto finish_seg_header_desc_array_check;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;
	res = SSDFS_FSCK_CHECK_RESULT_SUCCESS;

finish_seg_header_desc_array_check:
	if (uncompr_data)
		free(uncompr_data);

	if (data)
		free(data);

	return res;
}

static
int ssdfs_fsck_seg_header_desc_array_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;

	seg_hdr = &log->header.seg_hdr;
	return ssdfs_fsck_desc_array_check(state, item,
					   metadata_id, log,
					   seg_hdr->desc_array,
					   SSDFS_SEG_HDR_DESC_MAX);
}

static
int ssdfs_fsck_seg_header_migration_id_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;
	u8 cur_migration_id;
	u8 prev_migration_id;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_PEB_MIGRATION_ID;

	seg_hdr = &log->header.seg_hdr;
	cur_migration_id =
		le8_to_cpu(seg_hdr->peb_migration_id[SSDFS_CUR_MIGRATING_PEB]);

	if (cur_migration_id == SSDFS_PEB_UNKNOWN_MIGRATION_ID ||
	    cur_migration_id == SSDFS_PEB_MIGRATION_ID_MAX) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted peb_migration_id: "
			  "peb_id %llu, start_page %u, "
			  "peb_migration_id %u\n",
			  log->peb_id, log->start_page,
			  cur_migration_id);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	prev_migration_id =
		le8_to_cpu(seg_hdr->peb_migration_id[SSDFS_PREV_MIGRATING_PEB]);

	if (prev_migration_id == SSDFS_PEB_UNKNOWN_MIGRATION_ID) {
		/* PEB hasn't been migrated yet (mkfs creates such PEBs) */
		if (cur_migration_id != SSDFS_PEB_MIGRATION_ID_START) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted peb_migration_id: "
				  "peb_id %llu, start_page %u, "
				  "prev_migration_id %u, "
				  "cur_migration_id %u\n",
				  log->peb_id, log->start_page,
				  prev_migration_id, cur_migration_id);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		item->state = SSDFS_FSCK_METADATA_ITEM_OK;
		return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
	}

	if (prev_migration_id < SSDFS_PEB_MIGRATION_ID_MAX) {
		if (cur_migration_id == (SSDFS_PEB_UNKNOWN_MIGRATION_ID + 1) &&
		    prev_migration_id != (SSDFS_PEB_MIGRATION_ID_MAX - 1)) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted peb_migration_id: "
				  "peb_id %llu, start_page %u, "
				  "prev_migration_id %u, "
				  "cur_migration_id %u\n",
				  log->peb_id, log->start_page,
				  prev_migration_id, cur_migration_id);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		} else if ((prev_migration_id + 1) != cur_migration_id) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted peb_migration_id: "
				  "peb_id %llu, start_page %u, "
				  "prev_migration_id %u, "
				  "cur_migration_id %u\n",
				  log->peb_id, log->start_page,
				  prev_migration_id, cur_migration_id);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_seg_header_peb_create_time_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;
	u64 peb_create_time;
	u64 timestamp;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_PEB_CREATE_TIME_ID;

	seg_hdr = &log->header.seg_hdr;
	peb_create_time = le64_to_cpu(seg_hdr->peb_create_time);
	timestamp = le64_to_cpu(seg_hdr->timestamp);

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

static
int ssdfs_fsck_seg_header_relation_peb_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *seg_hdr;
	u16 seg_type;
	u64 seg_id;
	u64 calculated_seg_id;
	u32 seg_size;
	u64 segs_total;
	u64 leb_id;
	u64 lebs_total;
	u64 peb_id;
	u64 relation_peb_id;
	u32 peb_size;
	int is_found = SSDFS_FALSE;
	int i, j;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, metadata_id %#x\n",
		  log->peb_id, log->start_page, metadata_id);

	item->metadata_id = metadata_id;
	item->item_id = SSDFS_FSCK_HDR_RELATION_PEB_ID;

	seg_hdr = &log->header.seg_hdr;
	seg_type = le16_to_cpu(seg_hdr->seg_type);

	if (seg_type <= SSDFS_UNKNOWN_SEG_TYPE ||
	    seg_type > SSDFS_LAST_KNOWN_SEG_TYPE) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted relation_peb: "
			  "peb_id %llu, start_page %u, "
			  "seg_type %#x\n",
			  log->peb_id, log->start_page, seg_type);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	seg_size = ssdfs_fsck_log2_to_size(seg_hdr->volume_hdr.log_segsize);
	peb_size = ssdfs_fsck_log2_to_size(seg_hdr->volume_hdr.log_erasesize);

	if (seg_size == 0 || peb_size == 0) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted relation_peb: "
			  "peb_id %llu, start_page %u, "
			  "seg_size %u, peb_size %u\n",
			  log->peb_id, log->start_page,
			  seg_size, peb_size);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	segs_total = state->base.fs_size / seg_size;
	lebs_total = state->base.fs_size / peb_size;

	seg_id = le64_to_cpu(seg_hdr->seg_id);

	if (seg_id >= U64_MAX || seg_id >= segs_total) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted relation_peb: "
			  "peb_id %llu, start_page %u, "
			  "seg_id %llu\n",
			  log->peb_id, log->start_page, seg_id);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	leb_id = le64_to_cpu(seg_hdr->leb_id);

	if (leb_id >= U64_MAX || leb_id >= lebs_total) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted relation_peb: "
			  "peb_id %llu, start_page %u, "
			  "leb_id %llu\n",
			  log->peb_id, log->start_page, leb_id);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	calculated_seg_id = (leb_id * peb_size) / seg_size;

	if (calculated_seg_id != seg_id) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted relation_peb: "
			  "peb_id %llu, start_page %u, "
			  "leb_id %llu, seg_id %llu, "
			  "calculated_seg_id %llu\n",
			  log->peb_id, log->start_page,
			  leb_id, seg_id, calculated_seg_id);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	peb_id = le64_to_cpu(seg_hdr->peb_id);

	if (peb_id >= U64_MAX || peb_id >= lebs_total || peb_id != log->peb_id) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted relation_peb: "
			  "peb_id %llu, start_page %u, "
			  "peb_id %llu\n",
			  log->peb_id, log->start_page, peb_id);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	relation_peb_id = le64_to_cpu(seg_hdr->relation_peb_id);

	switch (seg_type) {
	case SSDFS_INITIAL_SNAPSHOT_SEG_TYPE:
		if (seg_id == SSDFS_INITIAL_SNAPSHOT_SEG_ID &&
		    leb_id == SSDFS_INITIAL_SNAPSHOT_SEG_LEB_ID &&
		    peb_id == SSDFS_INITIAL_SNAPSHOT_SEG_PEB_ID &&
		    relation_peb_id == U64_MAX) {
			/* expected state */
		} else {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted relation_peb: "
				  "peb_id %llu, start_page %u, "
				  "seg_id %llu, leb_id %llu, "
				  "peb_id %llu, relation_peb_id %llu\n",
				  log->peb_id, log->start_page,
				  seg_id, leb_id, peb_id,
				  relation_peb_id);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}
		break;

	case SSDFS_SB_SEG_TYPE:
		for (i = 0; i < SSDFS_SB_CHAIN_MAX; i++) {
			for (j = 0; j < SSDFS_SB_SEG_COPY_MAX; j++) {
				struct ssdfs_leb2peb_pair *pair;

				pair = &seg_hdr->volume_hdr.sb_pebs[i][j];

				if (leb_id == le64_to_cpu(pair->leb_id) &&
				    peb_id == le64_to_cpu(pair->peb_id)) {
					if (is_found || i != SSDFS_CUR_SB_SEG) {
						item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
						SSDFS_DBG(state->base.show_debug,
							  "corrupted relation_peb: "
							  "peb_id %llu, start_page %u, "
							  "seg_id %llu, leb_id %llu, "
							  "peb_id %llu, relation_peb_id %llu\n",
							  log->peb_id, log->start_page,
							  seg_id, leb_id, peb_id,
							  relation_peb_id);
						return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
					} else
						is_found = SSDFS_TRUE;
				}
			}
		}

		if (!is_found) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted relation_peb: "
				  "peb_id %llu, start_page %u, "
				  "seg_id %llu, leb_id %llu, "
				  "peb_id %llu, relation_peb_id %llu\n",
				  log->peb_id, log->start_page,
				  seg_id, leb_id, peb_id,
				  relation_peb_id);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		if (relation_peb_id != U64_MAX) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted relation_peb: "
				  "peb_id %llu, start_page %u, "
				  "seg_id %llu, leb_id %llu, "
				  "peb_id %llu, relation_peb_id %llu\n",
				  log->peb_id, log->start_page,
				  seg_id, leb_id, peb_id,
				  relation_peb_id);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}
		break;

	case SSDFS_SEGBMAP_SEG_TYPE:
	case SSDFS_MAPTBL_SEG_TYPE:
	case SSDFS_LEAF_NODE_SEG_TYPE:
	case SSDFS_HYBRID_NODE_SEG_TYPE:
	case SSDFS_INDEX_NODE_SEG_TYPE:
	case SSDFS_USER_DATA_SEG_TYPE:
		if (seg_id == SSDFS_INITIAL_SNAPSHOT_SEG_ID ||
		    leb_id == SSDFS_INITIAL_SNAPSHOT_SEG_LEB_ID ||
		    peb_id == SSDFS_INITIAL_SNAPSHOT_SEG_PEB_ID) {
			item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
			SSDFS_DBG(state->base.show_debug,
				  "corrupted relation_peb: "
				  "peb_id %llu, start_page %u, "
				  "seg_id %llu, leb_id %llu, "
				  "peb_id %llu, relation_peb_id %llu\n",
				  log->peb_id, log->start_page,
				  seg_id, leb_id, peb_id,
				  relation_peb_id);
			return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
		}

		for (i = 0; i < SSDFS_SB_CHAIN_MAX; i++) {
			for (j = 0; j < SSDFS_SB_SEG_COPY_MAX; j++) {
				struct ssdfs_leb2peb_pair *pair;

				pair = &seg_hdr->volume_hdr.sb_pebs[i][j];

				if (leb_id == le64_to_cpu(pair->leb_id) ||
				    peb_id == le64_to_cpu(pair->peb_id)) {
					item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
					SSDFS_DBG(state->base.show_debug,
						  "corrupted relation_peb: "
						  "peb_id %llu, start_page %u, "
						  "seg_id %llu, leb_id %llu, "
						  "peb_id %llu, relation_peb_id %llu\n",
						  log->peb_id, log->start_page,
						  seg_id, leb_id, peb_id,
						  relation_peb_id);
					return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
				}
			}
		}

		if (relation_peb_id < U64_MAX) {
			if (relation_peb_id == SSDFS_INITIAL_SNAPSHOT_SEG_PEB_ID) {
				item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
				SSDFS_DBG(state->base.show_debug,
					  "corrupted relation_peb: "
					  "peb_id %llu, start_page %u, "
					  "seg_id %llu, leb_id %llu, "
					  "peb_id %llu, relation_peb_id %llu\n",
					  log->peb_id, log->start_page,
					  seg_id, leb_id, peb_id,
					  relation_peb_id);
				return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
			}

			if (relation_peb_id == peb_id) {
				item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
				SSDFS_DBG(state->base.show_debug,
					  "corrupted relation_peb: "
					  "peb_id %llu, start_page %u, "
					  "seg_id %llu, leb_id %llu, "
					  "peb_id %llu, relation_peb_id %llu\n",
					  log->peb_id, log->start_page,
					  seg_id, leb_id, peb_id,
					  relation_peb_id);
				return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
			}

			for (i = 0; i < SSDFS_SB_CHAIN_MAX; i++) {
				for (j = 0; j < SSDFS_SB_SEG_COPY_MAX; j++) {
					struct ssdfs_leb2peb_pair *pair;

					pair = &seg_hdr->volume_hdr.sb_pebs[i][j];

					if (relation_peb_id == le64_to_cpu(pair->peb_id)) {
						item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
						SSDFS_DBG(state->base.show_debug,
							  "corrupted relation_peb: "
							  "peb_id %llu, start_page %u, "
							  "seg_id %llu, leb_id %llu, "
							  "peb_id %llu, relation_peb_id %llu\n",
							  log->peb_id, log->start_page,
							  seg_id, leb_id, peb_id,
							  relation_peb_id);
						return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
					}
				}
			}
		}
		break;

	default:
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted relation_peb: "
			  "peb_id %llu, start_page %u, "
			  "seg_type %#x\n",
			  log->peb_id, log->start_page, seg_type);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	item->state = SSDFS_FSCK_METADATA_ITEM_OK;

	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static check_metadata_item_func segment_header_check[SSDFS_FSCK_HDR_ITEMS_MAX] = {
	ssdfs_fsck_seg_header_signature_check,		/* SSDFS_FSCK_HDR_SIGNATURE_ID */
	ssdfs_fsck_seg_header_version_check,		/* SSDFS_FSCK_HDR_VERSION_ID */
	ssdfs_fsck_seg_header_checksum_check,		/* SSDFS_FSCK_HDR_CHECKSUM_ID */
	ssdfs_fsck_seg_header_pagesize_check,		/* SSDFS_FSCK_HDR_PAGESIZE_ID */
	ssdfs_fsck_seg_header_erasesize_check,		/* SSDFS_FSCK_HDR_ERASESIZE_ID */
	ssdfs_fsck_seg_header_segsize_check,		/* SSDFS_FSCK_HDR_SEGSIZE_ID */
	ssdfs_fsck_seg_header_megs_per_peb_check,	/* SSDFS_FSCK_HDR_MEGS_PER_PEB_ID */
	ssdfs_fsck_seg_header_pebs_per_seg_check,	/* SSDFS_FSCK_HDR_PEBS_PER_SEG_ID */
	ssdfs_fsck_seg_header_create_time_check,	/* SSDFS_FSCK_HDR_CREATE_TIME_ID */
	ssdfs_fsck_seg_header_create_cno_check,		/* SSDFS_FSCK_HDR_CREATE_CNO_ID */
	ssdfs_fsck_seg_header_flags_check,		/* SSDFS_FSCK_HDR_FLAGS_ID */
	ssdfs_fsck_seg_header_lebs_per_peb_check,	/* SSDFS_FSCK_HDR_LEBS_PER_PEB_INDEX_ID */
	ssdfs_fsck_seg_header_sb_pebs_check,		/* SSDFS_FSCK_HDR_SB_PEBS_ID */
	ssdfs_fsck_seg_header_segbmap_check,		/* SSDFS_FSCK_HDR_SEGBMAP_ID */
	ssdfs_fsck_seg_header_maptbl_check,		/* SSDFS_FSCK_HDR_MAPTBL_ID */
	ssdfs_fsck_seg_header_log_pages_check,		/* SSDFS_FSCK_HDR_LOG_PAGES_ID */
	ssdfs_fsck_seg_header_create_threads_check,	/* SSDFS_FSCK_HDR_CREATE_THREADS_ID */
	ssdfs_fsck_seg_header_dentries_btree_check,	/* SSDFS_FSCK_HDR_DENTRIES_BTREE_ID */
	ssdfs_fsck_seg_header_extents_btree_check,	/* SSDFS_FSCK_HDR_EXTENTS_BTREE_ID */
	ssdfs_fsck_seg_header_xattr_btree_check,	/* SSDFS_FSCK_HDR_XATTR_BTREE_ID */
	ssdfs_fsck_seg_header_invextree_check,		/* SSDFS_FSCK_HDR_INVEXTREE_ID */
	ssdfs_fsck_seg_header_uuid_check,		/* SSDFS_FSCK_HDR_UUID_ID */
	ssdfs_fsck_seg_header_log_create_time_check,	/* SSDFS_FSCK_HDR_LOG_TIMESTAMP_ID */
	ssdfs_fsck_seg_header_log_create_cno_check,	/* SSDFS_FSCK_HDR_LOG_CNO_ID */
	ssdfs_fsck_seg_header_seg_type_check,		/* SSDFS_FSCK_HDR_SEG_TYPE_ID */
	ssdfs_fsck_seg_header_seg_flags_check,		/* SSDFS_FSCK_HDR_SEG_FLAGS_ID */
	ssdfs_fsck_seg_header_desc_array_check,		/* SSDFS_FSCK_HDR_DESC_ARRAY_ID */
	ssdfs_fsck_seg_header_migration_id_check,	/* SSDFS_FSCK_HDR_PEB_MIGRATION_ID */
	ssdfs_fsck_seg_header_peb_create_time_check,	/* SSDFS_FSCK_HDR_PEB_CREATE_TIME_ID */
	ssdfs_fsck_seg_header_relation_peb_check,	/* SSDFS_FSCK_HDR_RELATION_PEB_ID */
	ssdfs_fsck_seg_header_absent_item,		/* SSDFS_FSCK_HDR_PL_FLAGS_ID */
	ssdfs_fsck_seg_header_absent_item,		/* SSDFS_FSCK_HDR_LOG_BYTES_ID */
	ssdfs_fsck_seg_header_absent_item,		/* SSDFS_FSCK_HDR_NSEGS_ID */
	ssdfs_fsck_seg_header_absent_item,		/* SSDFS_FSCK_HDR_FREE_PAGES_ID */
	ssdfs_fsck_seg_header_absent_item,		/* SSDFS_FSCK_HDR_ROOT_FOLDER_ID */
	ssdfs_fsck_seg_header_absent_item,		/* SSDFS_FSCK_HDR_INODES_BTREE_ID */
	ssdfs_fsck_seg_header_absent_item,		/* SSDFS_FSCK_HDR_SHEXTREE_ID */
	ssdfs_fsck_seg_header_absent_item,		/* SSDFS_FSCK_HDR_SHDICT_BTREE_ID */
	ssdfs_fsck_seg_header_absent_item,		/* SSDFS_FSCK_HDR_SEQUENCE_ID */
	ssdfs_fsck_seg_header_absent_item,		/* SSDFS_FSCK_HDR_SNAP_BTREE_ID */
	ssdfs_fsck_seg_header_absent_item,		/* SSDFS_FSCK_HDR_OPEN_ZONES_ID */
};

int ssdfs_fsck_check_segment_header(struct ssdfs_thread_state *state,
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
		SSDFS_ERR("fail to allocate segment header's items: err %d\n",
			  err);
		return SSDFS_FSCK_CHECK_RESULT_FAILURE;
	}

	for (i = 0; i < SSDFS_FSCK_HDR_ITEMS_MAX; i++) {
		cur = &items->states[i];

		items->count++;
		items->checked++;

		res = segment_header_check[i](state, cur, metadata_id, log);
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
