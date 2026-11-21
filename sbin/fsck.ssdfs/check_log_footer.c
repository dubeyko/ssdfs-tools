/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/fsck.ssdfs/check_log_footer.c - check log footer.
 *
 * Copyright (c) 2026 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#include "fsck.h"
#include "check_file_system.h"

/*
 * The log can be finished by two types of footer:
 * (1) struct ssdfs_log_footer (SSDFS_LOG_HAS_FOOTER);
 * (2) struct ssdfs_partial_log_header (SSDFS_PARTIAL_HEADER_INSTEAD_FOOTER).
 * The SSDFS_FSCK_FOOTER_* items represent the union of fields
 * of both footer types. If some field doesn't exist in
 * the partial log header, then the item is marked as absent.
 */
enum {
	SSDFS_FSCK_UNKNOWN_FOOTER_TYPE,
	SSDFS_FSCK_LOG_FOOTER_TYPE,
	SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE,
};

static inline
int ssdfs_fsck_log_footer_type(struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_signature *magic = &log->footer.magic;

	if (is_ssdfs_log_footer(magic))
		return SSDFS_FSCK_LOG_FOOTER_TYPE;

	/* partial log header instead of footer */
	if (is_ssdfs_partial_log_header(magic))
		return SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE;

	return SSDFS_FSCK_UNKNOWN_FOOTER_TYPE;
}

/*
 * ssdfs_fsck_prepare_peb_environment() - prepare PEB's environment
 *
 * The footer's checks need some details of PEB and current log
 * that are stored in log's header. The details are prepared once
 * in PEB's environment before the check of footer's items.
 * The volume's details are prepared by detection logic
 * in the basic environment. The log header has valid signature
 * because footer is checked only after log header's check.
 */
static
void ssdfs_fsck_prepare_peb_environment(struct ssdfs_thread_state *state,
					struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_peb_environment *peb = &state->peb;
	struct ssdfs_segment_header *seg_hdr;
	struct ssdfs_partial_log_header *pl_hdr;
	struct ssdfs_signature *magic = &log->header.magic;

	if (is_ssdfs_segment_header(magic)) {
		seg_hdr = &log->header.seg_hdr;
		peb->log_pages = le16_to_cpu(seg_hdr->log_pages);
		peb->create_time = le64_to_cpu(seg_hdr->peb_create_time);
		peb->log_flags = le32_to_cpu(seg_hdr->seg_flags);
		peb->log_timestamp = le64_to_cpu(seg_hdr->timestamp);
		peb->log_cno = le64_to_cpu(seg_hdr->cno);
	} else if (is_ssdfs_partial_log_header(magic)) {
		pl_hdr = &log->header.pl_hdr;
		peb->log_pages = le16_to_cpu(pl_hdr->log_pages);
		peb->create_time = le64_to_cpu(pl_hdr->peb_create_time);
		peb->log_flags = le32_to_cpu(pl_hdr->pl_flags);
		peb->log_timestamp = le64_to_cpu(pl_hdr->timestamp);
		peb->log_cno = le64_to_cpu(pl_hdr->cno);
	}
}

/*
 * ssdfs_fsck_log_footer_prepare_check() - prepare item for footer check
 *
 * Returns footer type or SSDFS_FSCK_UNKNOWN_FOOTER_TYPE if footer
 * hasn't a valid signature. In the last case, the item is marked
 * as corrupted because footer's content cannot be trusted.
 */
static inline
int ssdfs_fsck_log_footer_prepare_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id, int item_id,
				struct ssdfs_fsck_found_log *log)
{
	int footer_type;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u, "
		  "metadata_id %#x, item_id %d\n",
		  log->peb_id, log->start_page,
		  metadata_id, item_id);

	item->metadata_id = metadata_id;
	item->item_id = item_id;

	footer_type = ssdfs_fsck_log_footer_type(log);
	if (footer_type == SSDFS_FSCK_UNKNOWN_FOOTER_TYPE) {
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		SSDFS_DBG(state->base.show_debug,
			  "corrupted footer's signature: "
			  "peb_id %llu, start_page %u\n",
			  log->peb_id, log->start_page);
		return SSDFS_FSCK_UNKNOWN_FOOTER_TYPE;
	}

	return footer_type;
}

/*
 * ssdfs_fsck_log_footer_item_result() - finalize the check of footer's item
 *
 * Shared helpers set the item ID of header's item. This method
 * restores the footer's item ID and normalizes the result.
 */
static inline
int ssdfs_fsck_log_footer_item_result(struct ssdfs_fsck_metadata_item_state *item,
				      int item_id, int res)
{
	item->item_id = item_id;

	switch (res) {
	case SSDFS_FSCK_CHECK_RESULT_SUCCESS:
	case SSDFS_FSCK_CHECK_RESULT_CORRUPTION:
		return res;

	default:
		item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
		break;
	}

	return SSDFS_FSCK_CHECK_RESULT_FAILURE;
}

static inline
int ssdfs_fsck_log_footer_item_corrupted(struct ssdfs_fsck_metadata_item_state *item)
{
	item->state = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
	return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
}

static inline
int ssdfs_fsck_log_footer_item_ok(struct ssdfs_fsck_metadata_item_state *item)
{
	item->state = SSDFS_FSCK_METADATA_ITEM_OK;
	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static inline
int ssdfs_fsck_log_footer_item_absent(struct ssdfs_fsck_metadata_item_state *item)
{
	item->state = SSDFS_FSCK_METADATA_ITEM_ABSENT;
	return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
}

static
int ssdfs_fsck_log_footer_absent_item(struct ssdfs_thread_state *state,
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
int ssdfs_fsck_log_footer_signature_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	int item_id = SSDFS_FSCK_FOOTER_SIGNATURE_ID;
	int footer_type;

	footer_type = ssdfs_fsck_log_footer_prepare_check(state, item,
							  metadata_id,
							  item_id,
							  log);
	if (footer_type == SSDFS_FSCK_UNKNOWN_FOOTER_TYPE)
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;

	/* footer's type should be consistent with header's flags */
	if (state->peb.log_flags & SSDFS_PARTIAL_HEADER_INSTEAD_FOOTER) {
		if (footer_type != SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE) {
			SSDFS_DBG(state->base.show_debug,
				  "partial log header is expected: "
				  "peb_id %llu, start_page %u, "
				  "header's flags %#x\n",
				  log->peb_id, log->start_page,
				  state->peb.log_flags);
			return ssdfs_fsck_log_footer_item_corrupted(item);
		}
	} else if (state->peb.log_flags & SSDFS_LOG_HAS_FOOTER) {
		if (footer_type != SSDFS_FSCK_LOG_FOOTER_TYPE) {
			SSDFS_DBG(state->base.show_debug,
				  "log footer is expected: "
				  "peb_id %llu, start_page %u, "
				  "header's flags %#x\n",
				  log->peb_id, log->start_page,
				  state->peb.log_flags);
			return ssdfs_fsck_log_footer_item_corrupted(item);
		}
	} else {
		SSDFS_DBG(state->base.show_debug,
			  "header hasn't footer's flag: "
			  "peb_id %llu, start_page %u, "
			  "header's flags %#x\n",
			  log->peb_id, log->start_page,
			  state->peb.log_flags);
		return ssdfs_fsck_log_footer_item_corrupted(item);
	}

	return ssdfs_fsck_log_footer_item_ok(item);
}

static
int ssdfs_fsck_log_footer_version_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_signature *magic = &log->footer.magic;
	int item_id = SSDFS_FSCK_FOOTER_VERSION_ID;
	int footer_type;

	footer_type = ssdfs_fsck_log_footer_prepare_check(state, item,
							  metadata_id,
							  item_id,
							  log);
	if (footer_type == SSDFS_FSCK_UNKNOWN_FOOTER_TYPE)
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;

	if (magic->version.major != SSDFS_MAJOR_REVISION ||
	    magic->version.minor != SSDFS_MINOR_REVISION) {
		SSDFS_DBG(state->base.show_debug,
			  "unexpected on-disk layout version: "
			  "peb_id %llu, start_page %u, "
			  "version (major %u, minor %u), "
			  "expected (major %u, minor %u)\n",
			  log->peb_id, log->start_page,
			  magic->version.major,
			  magic->version.minor,
			  SSDFS_MAJOR_REVISION,
			  SSDFS_MINOR_REVISION);
		return ssdfs_fsck_log_footer_item_corrupted(item);
	}

	return ssdfs_fsck_log_footer_item_ok(item);
}

static
int ssdfs_fsck_log_footer_checksum_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_log_footer *footer;
	struct ssdfs_partial_log_header *pl_hdr;
	int item_id = SSDFS_FSCK_FOOTER_CHECKSUM_ID;
	int is_valid = SSDFS_FALSE;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		footer = &log->footer.footer;
		is_valid = is_csum_valid(&footer->volume_state.check,
					 footer,
					 sizeof(struct ssdfs_log_footer));
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		pl_hdr = &log->footer.pl_hdr;
		is_valid = is_csum_valid(&pl_hdr->check,
					 pl_hdr,
					 sizeof(struct ssdfs_partial_log_header));
		break;

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (!is_valid) {
		SSDFS_DBG(state->base.show_debug,
			  "incorrect footer's checksum: "
			  "peb_id %llu, start_page %u\n",
			  log->peb_id, log->start_page);
		return ssdfs_fsck_log_footer_item_corrupted(item);
	}

	return ssdfs_fsck_log_footer_item_ok(item);
}

static
int ssdfs_fsck_log_footer_nsegs_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	int item_id = SSDFS_FSCK_FOOTER_NSEGS_ID;
	u64 nsegs;
	int res;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		nsegs = le64_to_cpu(log->footer.footer.volume_state.nsegs);
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		nsegs = le64_to_cpu(log->footer.pl_hdr.nsegs);
		break;

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	res = ssdfs_fsck_nsegs_check(state, item, metadata_id, log,
				     ilog2(state->base.seg_size), nsegs);
	return ssdfs_fsck_log_footer_item_result(item, item_id, res);
}

static
int ssdfs_fsck_log_footer_free_pages_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	int item_id = SSDFS_FSCK_FOOTER_FREE_PAGES_ID;
	u64 nsegs;
	u64 free_pages;
	int res;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		nsegs = le64_to_cpu(log->footer.footer.volume_state.nsegs);
		free_pages =
		    le64_to_cpu(log->footer.footer.volume_state.free_pages);
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		nsegs = le64_to_cpu(log->footer.pl_hdr.nsegs);
		free_pages = le64_to_cpu(log->footer.pl_hdr.free_pages);
		break;

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	res = ssdfs_fsck_free_pages_check(state, item, metadata_id, log,
					  ilog2(state->base.page_size),
					  ilog2(state->base.seg_size),
					  nsegs, free_pages);
	return ssdfs_fsck_log_footer_item_result(item, item_id, res);
}

static
int ssdfs_fsck_log_footer_timestamp_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	int item_id = SSDFS_FSCK_FOOTER_LOG_TIMESTAMP_ID;
	u64 timestamp;
	int res;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		timestamp = le64_to_cpu(log->footer.footer.timestamp);
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		timestamp = le64_to_cpu(log->footer.pl_hdr.timestamp);
		break;

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	res = ssdfs_fsck_log_create_time_check(state, item, metadata_id, log,
						state->base.fs_create_time,
						timestamp);
	res = ssdfs_fsck_log_footer_item_result(item, item_id, res);
	if (res != SSDFS_FSCK_CHECK_RESULT_SUCCESS)
		return res;

	/* footer is written after the header of the same log */
	if (timestamp < state->peb.log_timestamp) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted footer's timestamp: "
			  "peb_id %llu, start_page %u, "
			  "footer's timestamp %llx, "
			  "header's timestamp %llx\n",
			  log->peb_id, log->start_page,
			  timestamp, state->peb.log_timestamp);
		return ssdfs_fsck_log_footer_item_corrupted(item);
	}

	return ssdfs_fsck_log_footer_item_ok(item);
}

static
int ssdfs_fsck_log_footer_checkpoint_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	int item_id = SSDFS_FSCK_FOOTER_LOG_CNO_ID;
	u64 cno;
	int res;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		cno = le64_to_cpu(log->footer.footer.cno);
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		cno = le64_to_cpu(log->footer.pl_hdr.cno);
		break;

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	res = ssdfs_fsck_log_create_cno_check(state, item, metadata_id, log,
						cno);
	res = ssdfs_fsck_log_footer_item_result(item, item_id, res);
	if (res != SSDFS_FSCK_CHECK_RESULT_SUCCESS)
		return res;

	if (cno < state->peb.log_cno) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted footer's cno: "
			  "peb_id %llu, start_page %u, "
			  "footer's cno %llu, header's cno %llu\n",
			  log->peb_id, log->start_page,
			  cno, state->peb.log_cno);
		return ssdfs_fsck_log_footer_item_corrupted(item);
	}

	return ssdfs_fsck_log_footer_item_ok(item);
}

static inline
u32 ssdfs_fsck_log_footer_volume_flags(struct ssdfs_fsck_found_log *log,
					int footer_type)
{
	switch (footer_type) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		return le32_to_cpu(log->footer.footer.volume_state.flags);

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		return le32_to_cpu(log->footer.pl_hdr.flags);

	default:
		/* unknown footer type */
		break;
	}

	return 0;
}

static
int ssdfs_fsck_log_footer_flags_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	int item_id = SSDFS_FSCK_FOOTER_FLAGS_ID;
	int footer_type;
	u32 flags;

	footer_type = ssdfs_fsck_log_footer_prepare_check(state, item,
							  metadata_id,
							  item_id,
							  log);
	if (footer_type == SSDFS_FSCK_UNKNOWN_FOOTER_TYPE)
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;

	flags = ssdfs_fsck_log_footer_volume_flags(log, footer_type);

	if (flags & ~SSDFS_VOLUME_STATE_FLAGS_MASK) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted volume state flags: "
			  "peb_id %llu, start_page %u, "
			  "flags %#x\n",
			  log->peb_id, log->start_page, flags);
		return ssdfs_fsck_log_footer_item_corrupted(item);
	}

	return ssdfs_fsck_log_footer_item_ok(item);
}

static
int ssdfs_fsck_log_footer_fs_state_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	int item_id = SSDFS_FSCK_FOOTER_STATE_ID;
	u16 fs_state;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		fs_state =
		    le16_to_cpu(log->footer.footer.volume_state.state);
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		return ssdfs_fsck_log_footer_item_absent(item);

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (fs_state & ~(SSDFS_VALID_FS | SSDFS_ERROR_FS | SSDFS_RESIZE_FS)) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted file system state: "
			  "peb_id %llu, start_page %u, "
			  "state %#x\n",
			  log->peb_id, log->start_page, fs_state);
		return ssdfs_fsck_log_footer_item_corrupted(item);
	}

	return ssdfs_fsck_log_footer_item_ok(item);
}

static
int ssdfs_fsck_log_footer_fs_errors_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	int item_id = SSDFS_FSCK_FOOTER_ERRORS_ID;
	u16 errors;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		errors = le16_to_cpu(log->footer.footer.volume_state.errors);
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		return ssdfs_fsck_log_footer_item_absent(item);

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (errors < SSDFS_ERRORS_CONTINUE ||
	    errors > SSDFS_LAST_KNOWN_FS_ERROR) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted errors behavior: "
			  "peb_id %llu, start_page %u, "
			  "errors %#x\n",
			  log->peb_id, log->start_page, errors);
		return ssdfs_fsck_log_footer_item_corrupted(item);
	}

	return ssdfs_fsck_log_footer_item_ok(item);
}

static
int ssdfs_fsck_log_footer_fs_features_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_volume_state *vs;
	int item_id = SSDFS_FSCK_FOOTER_FEATURES_ID;
	u64 compat, compat_ro, incompat;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		vs = &log->footer.footer.volume_state;
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		return ssdfs_fsck_log_footer_item_absent(item);

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	compat = le64_to_cpu(vs->feature_compat);
	compat_ro = le64_to_cpu(vs->feature_compat_ro);
	incompat = le64_to_cpu(vs->feature_incompat);

	if ((compat & ~SSDFS_FEATURE_COMPAT_SUPP) ||
	    (compat_ro & ~SSDFS_FEATURE_COMPAT_RO_SUPP) ||
	    (incompat & ~SSDFS_FEATURE_INCOMPAT_SUPP)) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted features: "
			  "peb_id %llu, start_page %u, "
			  "compat %#llx, compat_ro %#llx, "
			  "incompat %#llx\n",
			  log->peb_id, log->start_page,
			  compat, compat_ro, incompat);
		return ssdfs_fsck_log_footer_item_corrupted(item);
	}

	return ssdfs_fsck_log_footer_item_ok(item);
}

static
int ssdfs_fsck_log_footer_fs_uuid_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	int item_id = SSDFS_FSCK_FOOTER_UUID_ID;
	__le8 *uuid;
	int res;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		uuid = log->footer.footer.volume_state.uuid;
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		uuid = log->footer.pl_hdr.uuid;
		break;

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	res = ssdfs_fsck_uuid_check(state, item, metadata_id, log,
				    uuid_string(uuid));
	res = ssdfs_fsck_log_footer_item_result(item, item_id, res);
	if (res != SSDFS_FSCK_CHECK_RESULT_SUCCESS)
		return res;

	if (memcmp(uuid, state->base.fs_uuid, SSDFS_UUID_SIZE) != 0) {
		SSDFS_DBG(state->base.show_debug,
			  "footer's UUID is different from header's one: "
			  "peb_id %llu, start_page %u\n",
			  log->peb_id, log->start_page);
		return ssdfs_fsck_log_footer_item_corrupted(item);
	}

	return ssdfs_fsck_log_footer_item_ok(item);
}

static
int ssdfs_fsck_log_footer_cur_segs_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_volume_state *vs;
	int item_id = SSDFS_FSCK_FOOTER_CUR_SEGS_ID;
	u64 nsegs;
	u64 seg_id;
	int i;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		vs = &log->footer.footer.volume_state;
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		return ssdfs_fsck_log_footer_item_absent(item);

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	nsegs = le64_to_cpu(vs->nsegs);

	for (i = 0; i < SSDFS_CUR_SEGS_COUNT; i++) {
		seg_id = le64_to_cpu(vs->cur_segs[i]);

		if (seg_id == U64_MAX)
			continue;

		if (seg_id >= nsegs) {
			SSDFS_DBG(state->base.show_debug,
				  "corrupted current segment: "
				  "peb_id %llu, start_page %u, "
				  "cur_segs[%d] %llu, nsegs %llu\n",
				  log->peb_id, log->start_page,
				  i, seg_id, nsegs);
			return ssdfs_fsck_log_footer_item_corrupted(item);
		}
	}

	return ssdfs_fsck_log_footer_item_ok(item);
}

static
int ssdfs_fsck_log_footer_migration_thld_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	int item_id = SSDFS_FSCK_FOOTER_MIGRATION_THRESHOLD_ID;
	u16 threshold;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		threshold =
		  le16_to_cpu(log->footer.footer.volume_state.migration_threshold);
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		return ssdfs_fsck_log_footer_item_absent(item);

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (threshold == 0 || threshold >= U16_MAX) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted migration threshold: "
			  "peb_id %llu, start_page %u, "
			  "migration_threshold %u\n",
			  log->peb_id, log->start_page, threshold);
		return ssdfs_fsck_log_footer_item_corrupted(item);
	}

	return ssdfs_fsck_log_footer_item_ok(item);
}

static inline
int ssdfs_fsck_log_footer_check_options(struct ssdfs_thread_state *state,
					struct ssdfs_fsck_metadata_item_state *item,
					struct ssdfs_fsck_found_log *log,
					const char *name,
					u16 flags, u16 flags_mask,
					u8 compression, u8 last_compression)
{
	if (flags & ~flags_mask) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted %s options: "
			  "peb_id %llu, start_page %u, "
			  "flags %#x\n",
			  name, log->peb_id, log->start_page, flags);
		return ssdfs_fsck_log_footer_item_corrupted(item);
	}

	if (compression > last_compression) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted %s options: "
			  "peb_id %llu, start_page %u, "
			  "compression %#x\n",
			  name, log->peb_id, log->start_page, compression);
		return ssdfs_fsck_log_footer_item_corrupted(item);
	}

	return ssdfs_fsck_log_footer_item_ok(item);
}

static
int ssdfs_fsck_log_footer_blkbmap_opts_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_blk_bmap_options *opts;
	int item_id = SSDFS_FSCK_FOOTER_BLKBMAP_OPTIONS_ID;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		opts = &log->footer.footer.volume_state.blkbmap;
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		return ssdfs_fsck_log_footer_item_absent(item);

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	return ssdfs_fsck_log_footer_check_options(state, item, log,
					"block bitmap",
					le16_to_cpu(opts->flags),
					SSDFS_BLK_BMAP_OPTIONS_MASK,
					opts->compression,
					SSDFS_BLK_BMAP_ZSTD_COMPR_TYPE);
}

static
int ssdfs_fsck_log_footer_blk2off_opts_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_blk2off_tbl_options *opts;
	int item_id = SSDFS_FSCK_FOOTER_BLK2OFF_OPTIONS_ID;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		opts = &log->footer.footer.volume_state.blk2off_tbl;
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		return ssdfs_fsck_log_footer_item_absent(item);

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	return ssdfs_fsck_log_footer_check_options(state, item, log,
					"offset translation table",
					le16_to_cpu(opts->flags),
					SSDFS_BLK2OFF_TBL_OPTIONS_MASK,
					opts->compression,
					SSDFS_BLK2OFF_TBL_ZSTD_COMPR_TYPE);
}

static
int ssdfs_fsck_log_footer_user_data_opts_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_user_data_options *opts;
	int item_id = SSDFS_FSCK_FOOTER_USER_DATA_OPTIONS_ID;
	u16 threshold;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		opts = &log->footer.footer.volume_state.user_data;
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		return ssdfs_fsck_log_footer_item_absent(item);

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	threshold = le16_to_cpu(opts->migration_threshold);

	if (threshold == 0 || threshold >= U16_MAX) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted user data options: "
			  "peb_id %llu, start_page %u, "
			  "migration_threshold %u\n",
			  log->peb_id, log->start_page, threshold);
		return ssdfs_fsck_log_footer_item_corrupted(item);
	}

	return ssdfs_fsck_log_footer_check_options(state, item, log,
					"user data",
					le16_to_cpu(opts->flags),
					SSDFS_USER_DATA_OPTIONS_MASK,
					opts->compression,
					SSDFS_USER_DATA_ZSTD_COMPR_TYPE);
}

static
int ssdfs_fsck_log_footer_open_zones_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	int item_id = SSDFS_FSCK_FOOTER_OPEN_ZONES_ID;
	u32 open_zones;
	u64 nsegs;
	int res;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		open_zones =
		    le32_to_cpu(log->footer.footer.volume_state.open_zones);
		nsegs = le64_to_cpu(log->footer.footer.volume_state.nsegs);
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		open_zones = le32_to_cpu(log->footer.pl_hdr.open_zones);
		nsegs = le64_to_cpu(log->footer.pl_hdr.nsegs);
		break;

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	res = ssdfs_fsck_open_zones_check(state, item, metadata_id, log,
					  open_zones, nsegs);
	return ssdfs_fsck_log_footer_item_result(item, item_id, res);
}

static
int ssdfs_fsck_log_footer_root_folder_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_inode *root_folder;
	int item_id = SSDFS_FSCK_FOOTER_ROOT_FOLDER_ID;
	int res;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		root_folder = &log->footer.footer.volume_state.root_folder;
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		root_folder = &log->footer.pl_hdr.root_folder;
		break;

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	res = ssdfs_fsck_root_folder_check(state, item, metadata_id, log,
					   root_folder);
	return ssdfs_fsck_log_footer_item_result(item, item_id, res);
}

static
int ssdfs_fsck_log_footer_btree_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				int item_id,
				struct ssdfs_fsck_found_log *log,
				u8 type,
				struct ssdfs_btree_descriptor *desc)
{
	int res;

	res = ssdfs_fsck_btree_desc_check(state, item, log, type, desc);
	item->metadata_id = metadata_id;
	res = ssdfs_fsck_log_footer_item_result(item, item_id, res);
	if (res != SSDFS_FSCK_CHECK_RESULT_SUCCESS)
		return res;

	return ssdfs_fsck_log_footer_item_ok(item);
}

static
int ssdfs_fsck_log_footer_inodes_btree_desc_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_btree_descriptor *desc;
	int item_id = SSDFS_FSCK_FOOTER_INODES_BTREE_ID;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		desc = &log->footer.footer.volume_state.inodes_btree.desc;
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		desc = &log->footer.pl_hdr.inodes_btree.desc;
		break;

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	return ssdfs_fsck_log_footer_btree_check(state, item,
						 metadata_id, item_id, log,
						 SSDFS_INODES_BTREE, desc);
}

static
int ssdfs_fsck_log_footer_shextree_desc_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_btree_descriptor *desc;
	int item_id = SSDFS_FSCK_FOOTER_SHEXTREE_ID;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		desc =
		  &log->footer.footer.volume_state.shared_extents_btree.desc;
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		desc = &log->footer.pl_hdr.shared_extents_btree.desc;
		break;

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	return ssdfs_fsck_log_footer_btree_check(state, item,
						 metadata_id, item_id, log,
						 SSDFS_SHARED_EXTENTS_BTREE,
						 desc);
}

static
int ssdfs_fsck_log_footer_shdict_btree_desc_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_btree_descriptor *desc;
	int item_id = SSDFS_FSCK_FOOTER_SHDICT_BTREE_ID;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		desc =
		  &log->footer.footer.volume_state.shared_dict_btree.desc;
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		desc = &log->footer.pl_hdr.shared_dict_btree.desc;
		break;

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	return ssdfs_fsck_log_footer_btree_check(state, item,
						 metadata_id, item_id, log,
						 SSDFS_SHARED_DICTIONARY_BTREE,
						 desc);
}

static
int ssdfs_fsck_log_footer_snap_btree_desc_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_btree_descriptor *desc;
	int item_id = SSDFS_FSCK_FOOTER_SNAP_BTREE_ID;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		desc = &log->footer.footer.volume_state.snapshots_btree.desc;
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		desc = &log->footer.pl_hdr.snapshots_btree.desc;
		break;

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	return ssdfs_fsck_log_footer_btree_check(state, item,
						 metadata_id, item_id, log,
						 SSDFS_SNAPSHOTS_BTREE, desc);
}

static
int ssdfs_fsck_log_footer_log_bytes_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	int item_id = SSDFS_FSCK_FOOTER_LOG_BYTES_ID;
	int footer_type;
	u32 log_bytes;
	u8 log_pagesize;
	u32 log_pages;
	int res;

	footer_type = ssdfs_fsck_log_footer_prepare_check(state, item,
							  metadata_id,
							  item_id,
							  log);
	switch (footer_type) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		log_bytes = le32_to_cpu(log->footer.footer.log_bytes);
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		log_bytes = le32_to_cpu(log->footer.pl_hdr.log_bytes);
		break;

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	log_pagesize = ilog2(state->base.page_size);
	log_pages = state->peb.log_pages;

	if (ssdfs_fsck_log_footer_volume_flags(log, footer_type) &
					SSDFS_VS_4KB_PAGE_SIZE_PEB) {
		/*
		 * Superblock and base snapshot segments
		 * are created with 4KB page size.
		 */
		log_pagesize = ilog2(SSDFS_4KB);
		log_pages = state->base.erase_size / SSDFS_4KB;
		if (log_pages > U16_MAX)
			log_pages = U16_MAX;
	}

	res = ssdfs_fsck_log_bytes_check(state, item, metadata_id, log,
					 (u16)log_pages, log_pagesize,
					 ilog2(state->base.erase_size), log_bytes);
	return ssdfs_fsck_log_footer_item_result(item, item_id, res);
}

static
int ssdfs_fsck_log_footer_log_flags_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	int item_id = SSDFS_FSCK_FOOTER_LOG_FLAGSS_ID;
	u32 log_flags;
	int res;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		log_flags = le32_to_cpu(log->footer.footer.log_flags);

		if (log_flags & ~SSDFS_LOG_FOOTER_FLAG_MASK) {
			SSDFS_DBG(state->base.show_debug,
				  "corrupted footer's log flags: "
				  "peb_id %llu, start_page %u, "
				  "log_flags %#x\n",
				  log->peb_id, log->start_page,
				  log_flags);
			return ssdfs_fsck_log_footer_item_corrupted(item);
		}
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		log_flags = le32_to_cpu(log->footer.pl_hdr.pl_flags);
		res = ssdfs_fsck_seg_flags_check(state, item, metadata_id,
						 log, log_flags);
		return ssdfs_fsck_log_footer_item_result(item, item_id, res);

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	return ssdfs_fsck_log_footer_item_ok(item);
}

static
int ssdfs_fsck_log_footer_desc_array_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_log_footer *footer;
	struct ssdfs_partial_log_header *pl_hdr;
	int item_id = SSDFS_FSCK_FOOTER_DESC_ARRAY_ID;
	u32 log_flags;
	u32 expected = SSDFS_LOG_FOOTER_HAS_BLK_BMAP |
			SSDFS_LOG_FOOTER_HAS_OFFSET_TABLE;
	int i;
	int res;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		footer = &log->footer.footer;
		log_flags = le32_to_cpu(footer->log_flags);

		if ((log_flags & expected) == 0) {
			/* footer hasn't any metadata areas */
			for (i = 0; i < SSDFS_LOG_FOOTER_DESC_MAX; i++) {
				if (le32_to_cpu(footer->desc_array[i].size)) {
					SSDFS_DBG(state->base.show_debug,
						  "unexpected descriptor: "
						  "peb_id %llu, "
						  "start_page %u, "
						  "log_flags %#x, "
						  "desc[%d]\n",
						  log->peb_id,
						  log->start_page,
						  log_flags, i);
					return ssdfs_fsck_log_footer_item_corrupted(item);
				}
			}

			return ssdfs_fsck_log_footer_item_ok(item);
		}

		res = ssdfs_fsck_desc_array_check(state, item,
						  metadata_id, log,
						  footer->desc_array,
						  SSDFS_LOG_FOOTER_DESC_MAX);
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		pl_hdr = &log->footer.pl_hdr;
		res = ssdfs_fsck_desc_array_check(state, item,
						  metadata_id, log,
						  pl_hdr->desc_array,
						  SSDFS_SEG_HDR_DESC_MAX);
		break;

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	return ssdfs_fsck_log_footer_item_result(item, item_id, res);
}

static
int ssdfs_fsck_log_footer_create_time_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log)
{
	int item_id = SSDFS_FSCK_FOOTER_PEB_CREATE_TIME_ID;
	u64 peb_create_time;
	u64 timestamp;

	switch (ssdfs_fsck_log_footer_prepare_check(state, item,
						    metadata_id, item_id,
						    log)) {
	case SSDFS_FSCK_LOG_FOOTER_TYPE:
		peb_create_time =
			le64_to_cpu(log->footer.footer.peb_create_time);
		timestamp = le64_to_cpu(log->footer.footer.timestamp);
		break;

	case SSDFS_FSCK_PARTIAL_LOG_FOOTER_TYPE:
		peb_create_time =
			le64_to_cpu(log->footer.pl_hdr.peb_create_time);
		timestamp = le64_to_cpu(log->footer.pl_hdr.timestamp);
		break;

	default:
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	if (peb_create_time == 0 || peb_create_time >= U64_MAX) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted create_time: "
			  "peb_id %llu, start_page %u, "
			  "peb_create_time %llx\n",
			  log->peb_id, log->start_page,
			  peb_create_time);
		return ssdfs_fsck_log_footer_item_corrupted(item);
	}

	if (timestamp == 0 || timestamp >= U64_MAX) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted create_time: "
			  "peb_id %llu, start_page %u, "
			  "timestamp %llx\n",
			  log->peb_id, log->start_page,
			  timestamp);
		return ssdfs_fsck_log_footer_item_corrupted(item);
	}

	if (peb_create_time > timestamp) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted create_time: "
			  "peb_id %llu, start_page %u, "
			  "peb_create_time %llx, timestamp %llx\n",
			  log->peb_id, log->start_page,
			  peb_create_time, timestamp);
		return ssdfs_fsck_log_footer_item_corrupted(item);
	}

	/* header and footer belong to the same PEB */
	if (peb_create_time != state->peb.create_time) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted create_time: "
			  "peb_id %llu, start_page %u, "
			  "footer's peb_create_time %llx, "
			  "header's peb_create_time %llx\n",
			  log->peb_id, log->start_page,
			  peb_create_time, state->peb.create_time);
		return ssdfs_fsck_log_footer_item_corrupted(item);
	}

	return ssdfs_fsck_log_footer_item_ok(item);
}

static check_metadata_item_func footer_check[SSDFS_FSCK_FOOTER_ITEMS_MAX] = {
	ssdfs_fsck_log_footer_signature_check,		/* SSDFS_FSCK_FOOTER_SIGNATURE_ID */
	ssdfs_fsck_log_footer_version_check,		/* SSDFS_FSCK_FOOTER_VERSION_ID */
	ssdfs_fsck_log_footer_checksum_check,		/* SSDFS_FSCK_FOOTER_CHECKSUM_ID */
	ssdfs_fsck_log_footer_nsegs_check,		/* SSDFS_FSCK_FOOTER_NSEGS_ID */
	ssdfs_fsck_log_footer_free_pages_check,		/* SSDFS_FSCK_FOOTER_FREE_PAGES_ID */
	ssdfs_fsck_log_footer_timestamp_check,		/* SSDFS_FSCK_FOOTER_LOG_TIMESTAMP_ID */
	ssdfs_fsck_log_footer_checkpoint_check,		/* SSDFS_FSCK_FOOTER_LOG_CNO_ID */
	ssdfs_fsck_log_footer_flags_check,		/* SSDFS_FSCK_FOOTER_FLAGS_ID */
	ssdfs_fsck_log_footer_fs_state_check,		/* SSDFS_FSCK_FOOTER_STATE_ID */
	ssdfs_fsck_log_footer_fs_errors_check,		/* SSDFS_FSCK_FOOTER_ERRORS_ID */
	ssdfs_fsck_log_footer_fs_features_check,	/* SSDFS_FSCK_FOOTER_FEATURES_ID */
	ssdfs_fsck_log_footer_fs_uuid_check,		/* SSDFS_FSCK_FOOTER_UUID_ID */
	ssdfs_fsck_log_footer_cur_segs_check,		/* SSDFS_FSCK_FOOTER_CUR_SEGS_ID */
	ssdfs_fsck_log_footer_migration_thld_check,	/* SSDFS_FSCK_FOOTER_MIGRATION_THRESHOLD_ID */
	ssdfs_fsck_log_footer_blkbmap_opts_check,	/* SSDFS_FSCK_FOOTER_BLKBMAP_OPTIONS_ID */
	ssdfs_fsck_log_footer_blk2off_opts_check,	/* SSDFS_FSCK_FOOTER_BLK2OFF_OPTIONS_ID */
	ssdfs_fsck_log_footer_user_data_opts_check,	/* SSDFS_FSCK_FOOTER_USER_DATA_OPTIONS_ID */
	ssdfs_fsck_log_footer_open_zones_check,		/* SSDFS_FSCK_FOOTER_OPEN_ZONES_ID */
	ssdfs_fsck_log_footer_root_folder_check,	/* SSDFS_FSCK_FOOTER_ROOT_FOLDER_ID */
	ssdfs_fsck_log_footer_inodes_btree_desc_check,	/* SSDFS_FSCK_FOOTER_INODES_BTREE_ID */
	ssdfs_fsck_log_footer_shextree_desc_check,	/* SSDFS_FSCK_FOOTER_SHEXTREE_ID */
	ssdfs_fsck_log_footer_shdict_btree_desc_check,	/* SSDFS_FSCK_FOOTER_SHDICT_BTREE_ID */
	ssdfs_fsck_log_footer_snap_btree_desc_check,	/* SSDFS_FSCK_FOOTER_SNAP_BTREE_ID */
	ssdfs_fsck_log_footer_log_bytes_check,		/* SSDFS_FSCK_FOOTER_LOG_BYTES_ID */
	ssdfs_fsck_log_footer_log_flags_check,		/* SSDFS_FSCK_FOOTER_LOG_FLAGSS_ID */
	ssdfs_fsck_log_footer_desc_array_check,		/* SSDFS_FSCK_FOOTER_DESC_ARRAY_ID */
	ssdfs_fsck_log_footer_create_time_check,	/* SSDFS_FSCK_FOOTER_PEB_CREATE_TIME_ID */
};

static inline
int is_ssdfs_fsck_log_footer_expected(struct ssdfs_thread_state *state)
{
	return state->peb.log_flags & (SSDFS_LOG_HAS_FOOTER |
				       SSDFS_PARTIAL_HEADER_INSTEAD_FOOTER);
}

/*
 * ssdfs_fsck_check_log_footer() - check log footer
 * @state: pointer on thread state
 * @metadata: on-disk metadata items [SSDFS_FSCK_METADATA_ITEMS_MAX]
 * @log: found log descriptor
 *
 * The footer's items are stored in
 * @metadata[SSDFS_FSCK_LOG_FOOTER_METADATA_ID].
 *
 * RETURN:
 * [SSDFS_FSCK_CHECK_RESULT_SUCCESS] - footer is consistent.
 * [SSDFS_FSCK_CHECK_RESULT_CORRUPTION] - footer is corrupted.
 * [SSDFS_FSCK_CHECK_RESULT_FAILURE] - footer check failed.
 */
int ssdfs_fsck_check_log_footer(struct ssdfs_thread_state *state,
		struct ssdfs_fsck_metadata_items_array *metadata,
		struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_fsck_metadata_items_array *items;
	struct ssdfs_fsck_metadata_item_state *cur;
	int metadata_id = SSDFS_FSCK_LOG_FOOTER_METADATA_ID;
	int i;
	int err;
	int res = SSDFS_FSCK_CHECK_RESULT_SUCCESS;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u\n",
		  log->peb_id, log->start_page);

	items = &metadata[metadata_id];

	err = ssdfs_fsck_init_metadata_items(items, SSDFS_FSCK_FOOTER_ITEMS_MAX);
	if (err) {
		SSDFS_ERR("fail to allocate footer's items: err %d\n",
			  err);
		return SSDFS_FSCK_CHECK_RESULT_FAILURE;
	}

	ssdfs_fsck_prepare_peb_environment(state, log);

	if (!is_ssdfs_fsck_log_footer_expected(state)) {
		/*
		 * Partial log (not the latest one) can be finished
		 * without any footer.
		 */
		for (i = 0; i < SSDFS_FSCK_FOOTER_ITEMS_MAX; i++) {
			items->count++;
			ssdfs_fsck_log_footer_absent_item(state,
							  &items->states[i],
							  metadata_id, log);
		}

		SSDFS_DBG(state->base.show_debug,
			  "log hasn't footer: "
			  "peb_id %llu, start_page %u\n",
			  log->peb_id, log->start_page);
		return SSDFS_FSCK_CHECK_RESULT_SUCCESS;
	}

	if (ssdfs_fsck_log_footer_type(log) == SSDFS_FSCK_UNKNOWN_FOOTER_TYPE) {
		/*
		 * Footer's content cannot be trusted without
		 * valid signature. Mark signature as corrupted and
		 * the rest of items as absent.
		 */
		cur = &items->states[SSDFS_FSCK_FOOTER_SIGNATURE_ID];
		items->count++;
		items->checked++;
		items->corrupted++;
		ssdfs_fsck_log_footer_signature_check(state, cur,
						      metadata_id, log);

		for (i = SSDFS_FSCK_FOOTER_SIGNATURE_ID + 1;
		     i < SSDFS_FSCK_FOOTER_ITEMS_MAX; i++) {
			items->count++;
			ssdfs_fsck_log_footer_absent_item(state,
							  &items->states[i],
							  metadata_id, log);
		}

		SSDFS_DBG(state->base.show_debug,
			  "footer's signature is corrupted: "
			  "peb_id %llu, start_page %u\n",
			  log->peb_id, log->start_page);
		return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
	}

	for (i = 0; i < SSDFS_FSCK_FOOTER_ITEMS_MAX; i++) {
		cur = &items->states[i];

		items->count++;
		items->checked++;

		res = footer_check[i](state, cur, metadata_id, log);
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
	else
		res = SSDFS_FSCK_CHECK_RESULT_SUCCESS;

	return res;
}
