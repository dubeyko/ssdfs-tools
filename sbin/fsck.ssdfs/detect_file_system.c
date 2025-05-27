/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/fsck.ssdfs/detect_file_system.c - detect file system functionality.
 *
 * Copyright (c) 2025 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#include "ssdfs_tools.h"
#include "fsck.h"

enum {
	SSDFS_FSCK_SEARCH_RESULT_UNKNOWN,
	SSDFS_FSCK_SEARCH_RESULT_SUCCESS,
	SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND,
	SSDFS_FSCK_SEARCH_RESULT_FAILURE,
	SSDFS_FSCK_SEARCH_RESULT_STATE_MAX
};

static inline
struct ssdfs_fsck_volume_creation_point *
ssdfs_fsck_get_creation_point(struct ssdfs_fsck_environment *env, int index)
{
	struct ssdfs_fsck_volume_creation_array *array = NULL;
	struct ssdfs_fsck_volume_creation_point *creation_point = NULL;

	array = &env->detection_result.array;

	switch (array->state) {
	case SSDFS_FSCK_CREATION_ARRAY_USE_BUFFER:
	case SSDFS_FSCK_CREATION_ARRAY_ALLOCATED:
		/* continue logic */
		break;

	default:
		SSDFS_ERR("unexpected state of creation array: "
			  "state %#x\n", array->state);
		return NULL;
	}

	if (array->count <= index) {
		SSDFS_ERR("unexpected state of creation array: "
			  "count %d, index %d\n",
			  array->count, index);
		return NULL;
	}

	creation_point = &array->creation_points[index];
	return creation_point;
}

static
void ssdfs_fsck_init_creation_point(struct ssdfs_fsck_volume_creation_point *ptr)
{
	ptr->found_metadata = SSDFS_FSCK_NOTHING_FOUND_MASK;
	ptr->volume_creation_timestamp = U64_MAX;
	ssdfs_fsck_init_base_snapshot_seg(ptr);
	ssdfs_fsck_init_superblock_seg(ptr);
	ssdfs_fsck_init_segbmap(ptr);
	ssdfs_fsck_init_maptbl(ptr);
	ssdfs_fsck_init_maptbl_cache(ptr);
	ssdfs_fsck_init_metadata_map(ptr);
}

void ssdfs_fsck_init_detection_result(struct ssdfs_fsck_environment *env)
{
	struct ssdfs_fsck_volume_creation_array *array;
	struct ssdfs_fsck_volume_creation_point *creation_point;

	SSDFS_DBG(env->base.show_debug,
		  "init detection result\n");

	array = &env->detection_result.array;
	array->state = SSDFS_FSCK_CREATION_ARRAY_NOT_INITIALIZED;
	array->count = 0;

	memset(&array->buf, 0, sizeof(array->buf));
	array->creation_points = &array->buf;
	array->count = 1;
	array->state = SSDFS_FSCK_CREATION_ARRAY_USE_BUFFER;

	creation_point = ssdfs_fsck_get_creation_point(env, 0);
	ssdfs_fsck_init_creation_point(creation_point);
}

void ssdfs_fsck_destroy_detection_result(struct ssdfs_fsck_environment *env)
{
	struct ssdfs_fsck_volume_creation_array *array;
	struct ssdfs_fsck_volume_creation_point *creation_point;
	int i;

	SSDFS_DBG(env->base.show_debug,
		  "destroy detection result\n");

	array = &env->detection_result.array;

	for (i = 0; i < array->count; i++) {
		creation_point = &array->creation_points[i];

		ssdfs_fsck_destroy_base_snapshot_seg(creation_point);
		ssdfs_fsck_destroy_superblock_seg(creation_point);
		ssdfs_fsck_destroy_segbmap(creation_point);
		ssdfs_fsck_destroy_maptbl(creation_point);
		ssdfs_fsck_destroy_maptbl_cache(creation_point);
		ssdfs_fsck_destroy_metadata_map(creation_point);
	}

	switch (array->state) {
	case SSDFS_FSCK_CREATION_ARRAY_USE_BUFFER:
		array->creation_points = NULL;
		break;

	case SSDFS_FSCK_CREATION_ARRAY_ALLOCATED:
		free(array->creation_points);
		array->creation_points = NULL;
		break;

	default:
		return;
	}

	array->count = 0;
	array->state = SSDFS_FSCK_CREATION_ARRAY_NOT_INITIALIZED;
}

typedef int (*detect_fn)(struct ssdfs_fsck_environment *env);

static inline
int is_ssdfs_segment_header(struct ssdfs_signature *magic)
{
	return le32_to_cpu(magic->common) == SSDFS_SUPER_MAGIC &&
			le16_to_cpu(magic->key) == SSDFS_SEGMENT_HDR_MAGIC;
}

static inline
int is_ssdfs_partial_log_header(struct ssdfs_signature *magic)
{
	return le32_to_cpu(magic->common) == SSDFS_SUPER_MAGIC &&
			le16_to_cpu(magic->key) == SSDFS_PARTIAL_LOG_HDR_MAGIC;
}

static inline
int is_ssdfs_log_footer(struct ssdfs_signature *magic)
{
	return le32_to_cpu(magic->common) == SSDFS_SUPER_MAGIC &&
			le16_to_cpu(magic->key) == SSDFS_LOG_FOOTER_MAGIC;
}

static
int ssdfs_fsck_find_metadata_segment(struct ssdfs_fsck_environment *env,
				     u64 peb_id,
				     int seg_type,
				     u32 log_offset,
				     struct ssdfs_segment_header *hdr)
{
	struct ssdfs_signature *magic = NULL;
	u32 peb_size = env->base.erase_size;
	size_t sg_size = sizeof(struct ssdfs_segment_header);
	int err = 0;

	SSDFS_DBG(env->base.show_debug,
		  "Try to find metadata segment: "
		  "peb_id %llu, seg_type %#x, log_offset %u\n",
		  peb_id, seg_type, log_offset);

	err = ssdfs_read_segment_header(&env->base,
					peb_id,
					peb_size,
					log_offset,
					sg_size,
					hdr);
	if (err) {
		SSDFS_ERR("fail to read segment header: "
			  "peb_id %llu, err %d\n",
			  peb_id, err);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	magic = &hdr->volume_hdr.magic;

	if (!is_ssdfs_segment_header(magic))
		err = -ENODATA;
	else if (le16_to_cpu(hdr->seg_type) != seg_type)
		err = -ENODATA;

	if (err == -ENODATA) {
		SSDFS_DBG(env->base.show_debug,
			  "metadata segment hasn't been found\n");
		return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;
	}

	return SSDFS_FSCK_SEARCH_RESULT_SUCCESS;
}

static
int ssdfs_fsck_find_last_full_log(struct ssdfs_fsck_environment *env,
				  struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *hdr = NULL;
	struct ssdfs_segment_header cur_hdr;
	u32 peb_size = env->base.erase_size;
	size_t sg_size = sizeof(struct ssdfs_segment_header);
	u32 page_size = U32_MAX;
	u32 pages_per_peb = U32_MAX;
	u32 cur_page = U32_MAX;
	u16 full_log_pages;
	u32 log_offset;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "Try to find last full log: "
		  "peb_id %llu, start_page %u\n",
		  log->peb_id, log->start_page);

	if (is_ssdfs_segment_header(&log->header.magic)) {
		hdr = &log->header.seg_hdr;

		page_size = 1 << hdr->volume_hdr.log_pagesize;
		BUG_ON(page_size == 0);
		pages_per_peb = peb_size / page_size;
		BUG_ON(pages_per_peb == 0);
	} else {
		SSDFS_ERR("invalid state of header: "
			  "peb_id %llu, start_page %u\n",
			  log->peb_id, log->start_page);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	cur_page = log->start_page;

	if (cur_page >= pages_per_peb) {
		SSDFS_ERR("invalid state of start page: "
			  "peb_id %llu, start_page %u, "
			  "pages_per_peb %u\n",
			  log->peb_id, log->start_page,
			  pages_per_peb);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	BUG_ON(!hdr);

	full_log_pages = le16_to_cpu(hdr->log_pages);
	BUG_ON(full_log_pages == 0 || full_log_pages >= U16_MAX);
	cur_page += full_log_pages;

	if (cur_page > pages_per_peb) {
		SSDFS_ERR("invalid state of start page: "
			  "peb_id %llu, start_page %u, "
			  "pages_per_peb %u\n",
			  log->peb_id, log->start_page,
			  pages_per_peb);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	} else if (cur_page == pages_per_peb) {
		SSDFS_DBG(env->base.show_debug,
			  "No more full logs in erase block: "
			  "peb_id %llu, cur_page %u, "
			  "pages_per_peb %u\n",
			  log->peb_id, cur_page,
			  pages_per_peb);
		return SSDFS_FSCK_SEARCH_RESULT_SUCCESS;
	}

	do {
		struct ssdfs_signature *magic = NULL;

		log_offset = cur_page * page_size;

		err = ssdfs_read_segment_header(&env->base,
						log->peb_id,
						peb_size,
						log_offset,
						sg_size,
						&cur_hdr);
		if (err) {
			SSDFS_ERR("fail to read segment header: "
				  "peb_id %llu, cur_page %u, err %d\n",
				  log->peb_id, cur_page, err);
			return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
		}

		magic = &cur_hdr.volume_hdr.magic;

		if (!is_ssdfs_segment_header(magic)) {
			SSDFS_DBG(env->base.show_debug,
				  "cur_page %u hasn't valid segment header\n",
				  cur_page);
			break;
		} else {
			memcpy(hdr, &cur_hdr, sg_size);
			log->start_page = cur_page;
		}

		cur_page += full_log_pages;
	} while (cur_page < pages_per_peb);

	return SSDFS_FSCK_SEARCH_RESULT_SUCCESS;
}

static
int ssdfs_fsck_find_last_partial_log(struct ssdfs_fsck_environment *env,
				     struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_segment_header *hdr = NULL;
	struct ssdfs_partial_log_header pl_hdr;
	struct ssdfs_log_footer footer;
	struct ssdfs_metadata_descriptor *desc;
	size_t plh_size = sizeof(struct ssdfs_partial_log_header);
	size_t footer_size = sizeof(struct ssdfs_log_footer);
	u32 peb_size = env->base.erase_size;
	u32 page_size = U32_MAX;
	u32 pages_per_peb = U32_MAX;
	u32 cur_page = U32_MAX;
	u16 full_log_pages = U16_MAX;
	u32 upper_page;
	u32 area_offset;
	u32 area_size;
	u32 seg_flags;
	u32 log_offset;
	u32 log_bytes = U32_MAX;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "Try to find last partial log: "
		  "peb_id %llu, start_page %u\n",
		  log->peb_id, log->start_page);

	if (is_ssdfs_segment_header(&log->header.magic)) {
		hdr = &log->header.seg_hdr;

		page_size = 1 << hdr->volume_hdr.log_pagesize;
		BUG_ON(page_size == 0);
		pages_per_peb = peb_size / page_size;
		BUG_ON(pages_per_peb == 0);
		full_log_pages = le16_to_cpu(hdr->log_pages);
		BUG_ON(full_log_pages == 0 || full_log_pages >= U16_MAX);
	} else {
		SSDFS_ERR("invalid state of header: "
			  "peb_id %llu, start_page %u\n",
			  log->peb_id, log->start_page);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	BUG_ON(!hdr);

	desc = &hdr->desc_array[SSDFS_LOG_FOOTER_INDEX];
	area_offset = le32_to_cpu(desc->offset);
	area_size = le32_to_cpu(desc->size);
	seg_flags = le32_to_cpu(hdr->seg_flags);

	if (seg_flags & SSDFS_PARTIAL_HEADER_INSTEAD_FOOTER) {
		err = ssdfs_read_partial_log_footer(&env->base,
						    log->peb_id,
						    peb_size,
						    area_offset,
						    area_size,
						    &pl_hdr);
		if (err) {
			SSDFS_ERR("fail to read partial log footer: "
				  "peb_id %llu, peb_size %u, "
				  "area_offset %u, area_size %u, "
				  "err %d\n",
				  log->peb_id, peb_size,
				  area_offset, area_size,
				  err);
			return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
		}

		log_bytes = le32_to_cpu(pl_hdr.log_bytes);
	} else if (seg_flags & SSDFS_LOG_HAS_FOOTER) {
		err = ssdfs_read_log_footer(&env->base,
					    log->peb_id,
					    peb_size,
					    area_offset,
					    area_size,
					    &footer);
		if (err) {
			SSDFS_ERR("fail to read log footer: "
				  "peb_id %llu, peb_size %u, "
				  "area_offset %u, area_size %u, "
				  "err %d\n",
				  log->peb_id, peb_size,
				  area_offset, area_size,
				  err);
			return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
		}

		log_bytes = le32_to_cpu(footer.log_bytes);
	} else {
		SSDFS_ERR("invalid state of log footer: "
			  "peb_id %llu, start_page %u\n",
			  log->peb_id, log->start_page);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	if ((log_bytes / page_size) >= full_log_pages) {
		if (seg_flags & SSDFS_PARTIAL_HEADER_INSTEAD_FOOTER) {
			memcpy(&log->footer.pl_hdr, &pl_hdr, plh_size);
		} else {
			memcpy(&log->footer.footer, &footer, footer_size);
		}

		return SSDFS_FSCK_SEARCH_RESULT_SUCCESS;
	}

	cur_page = log->start_page + (log_bytes / page_size);
	upper_page = log->start_page + full_log_pages;

	do {
		struct ssdfs_signature *magic = NULL;
		u32 pl_flags;

		log_offset = cur_page * page_size;

		err = ssdfs_read_partial_log_header(&env->base,
						    log->peb_id,
						    peb_size,
						    log_offset,
						    plh_size,
						    &pl_hdr);
		if (err) {
			SSDFS_ERR("fail to read partial log header: "
				  "peb_id %llu, cur_page %u, err %d\n",
				  log->peb_id, cur_page, err);
			return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
		}

		magic = &pl_hdr.magic;

		if (!is_ssdfs_partial_log_header(magic)) {
			SSDFS_DBG(env->base.show_debug,
				  "cur_page %u hasn't valid partial log header\n",
				  cur_page);
			break;
		} else {
			memcpy(&log->header.pl_hdr, &pl_hdr, plh_size);
			log->start_page = cur_page;
		}

		pl_flags = le32_to_cpu(pl_hdr.pl_flags);

		if (pl_flags & SSDFS_LOG_HAS_FOOTER) {
			desc = &pl_hdr.desc_array[SSDFS_LOG_FOOTER_INDEX];
			area_offset = le32_to_cpu(desc->offset);
			area_size = le32_to_cpu(desc->size);

			err = ssdfs_read_log_footer(&env->base,
						    log->peb_id,
						    peb_size,
						    area_offset,
						    area_size,
						    &log->footer.footer);
			if (err) {
				SSDFS_ERR("fail to read log footer: "
					  "peb_id %llu, peb_size %u, "
					  "area_offset %u, area_size %u, "
					  "err %d\n",
					  log->peb_id, peb_size,
					  area_offset, area_size,
					  err);
			}
		}

		log_bytes = le32_to_cpu(pl_hdr.log_bytes);
		if ((log_bytes / page_size) == 0)
			cur_page++;
		else
			cur_page += (log_bytes / page_size);
	} while (cur_page < upper_page);

	return SSDFS_FSCK_SEARCH_RESULT_SUCCESS;
}

static
int is_base_snapshot_segment_found(struct ssdfs_fsck_environment *env)
{
	struct ssdfs_fsck_volume_creation_point *creation_point;
	struct ssdfs_fsck_base_snapshot_segment_detection *base_snapshot_seg;
	struct ssdfs_fsck_found_log *log;
	struct ssdfs_segment_header *hdr;
	int seg_type = SSDFS_INITIAL_SNAPSHOT_SEG_TYPE;
	u64 offset = 0;
	u32 peb_size = env->base.erase_size;
	u64 peb_id = offset / peb_size;
	int res;

	SSDFS_DBG(env->base.show_debug,
		  "Try to find base snapshot segment\n");

	creation_point = ssdfs_fsck_get_creation_point(env, 0);

	if (!creation_point) {
		SSDFS_ERR("fail to get creation point\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	base_snapshot_seg = &creation_point->base_snapshot_seg;
	log = &base_snapshot_seg->log;

	log->peb_id = U64_MAX;
	log->start_page = U32_MAX;

	hdr = &log->header.seg_hdr;

	SSDFS_DBG(env->base.show_debug,
		  "try to read the offset %llu\n",
		  offset);

	res = ssdfs_fsck_find_metadata_segment(env,
						peb_id,
						seg_type,
						offset,
						hdr);
	switch (res) {
	case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
		/* continue logic */
		break;

	case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
		SSDFS_DBG(env->base.show_debug,
			  "unable to find base snapshot segment\n");
		return res;

	default:
		SSDFS_ERR("fail to find base snapshot segment\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	creation_point->volume_creation_timestamp =
				le64_to_cpu(hdr->volume_hdr.create_time);

	log->peb_id = peb_id;
	log->start_page = 0;

	res = ssdfs_fsck_find_last_full_log(env, log);
	switch (res) {
	case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
	case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
		/* continue logic */
		break;

	default:
		SSDFS_ERR("fail to find last full log\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	env->seg_size = 1 << log->header.seg_hdr.volume_hdr.log_segsize;
	creation_point->found_metadata |= SSDFS_FSCK_BASE_SNAPSHOT_SEG_FOUND;

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return SSDFS_FSCK_SEARCH_RESULT_SUCCESS;
}

static
int __is_superblock_segment_found(struct ssdfs_fsck_environment *env,
				  int copy_index)
{
	struct ssdfs_fsck_volume_creation_point *creation_point;
	struct ssdfs_fsck_base_snapshot_segment_detection *base_snapshot_seg;
	struct ssdfs_fsck_superblock_segment_detection *superblock_seg;
	struct ssdfs_fsck_found_log *log;
	struct ssdfs_segment_header *hdr = NULL;
	struct ssdfs_segment_header cur_hdr;
	struct ssdfs_signature *magic = NULL;
	struct ssdfs_leb2peb_pair *leb2peb_pair;
	int cur_index = SSDFS_CUR_SB_SEG;
	int next_index = SSDFS_NEXT_SB_SEG;
	int reserved_index = SSDFS_RESERVED_SB_SEG;
	u64 peb_id = U64_MAX;
	int seg_type = SSDFS_SB_SEG_TYPE;
	size_t sg_size = sizeof(struct ssdfs_segment_header);
	u64 pebs_per_volume = env->base.fs_size / env->base.erase_size;
	u64 number_of_tries = 0;
	int res;

	SSDFS_DBG(env->base.show_debug,
		  "Try to find superblock segment\n");

	creation_point = ssdfs_fsck_get_creation_point(env, 0);

	if (!creation_point) {
		SSDFS_ERR("fail to get creation point\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	if (!(creation_point->found_metadata &
				SSDFS_FSCK_BASE_SNAPSHOT_SEG_FOUND)) {
		SSDFS_ERR("base snapshot segment is not found\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	base_snapshot_seg = &creation_point->base_snapshot_seg;
	log = &base_snapshot_seg->log;
	magic = &log->header.magic;

	if (is_ssdfs_segment_header(magic)) {
		hdr = &log->header.seg_hdr;
		leb2peb_pair = &hdr->volume_hdr.sb_pebs[cur_index][copy_index];
		peb_id = le64_to_cpu(leb2peb_pair->peb_id);
	} else {
		SSDFS_ERR("invalid state of base snapshot segment's found log\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	res = ssdfs_fsck_find_metadata_segment(env,
						peb_id,
						seg_type,
						0,
						&cur_hdr);
	switch (res) {
	case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
		/* continue logic */
		break;

	case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
		SSDFS_DBG(env->base.show_debug,
			  "unable to find superblock segment for current index\n");
		break;

	default:
		SSDFS_ERR("fail to find superblock segment\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	if (res == SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND) {
		leb2peb_pair = &hdr->volume_hdr.sb_pebs[next_index][copy_index];
		peb_id = le64_to_cpu(leb2peb_pair->peb_id);

		res = ssdfs_fsck_find_metadata_segment(env,
							peb_id,
							seg_type,
							0,
							&cur_hdr);
		switch (res) {
		case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
			/* continue logic */
			break;

		case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
			SSDFS_DBG(env->base.show_debug,
				  "unable to find superblock segment for next index\n");
			break;

		default:
			SSDFS_ERR("fail to find superblock segment\n");
			return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
		}
	}

	if (res == SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND) {
		leb2peb_pair = &hdr->volume_hdr.sb_pebs[reserved_index][copy_index];
		peb_id = le64_to_cpu(leb2peb_pair->peb_id);

		res = ssdfs_fsck_find_metadata_segment(env,
							peb_id,
							seg_type,
							0,
							&cur_hdr);
		switch (res) {
		case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
			/* continue logic */
			break;

		case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
			SSDFS_DBG(env->base.show_debug,
				  "unable to find superblock segment for reserved index\n");
			return res;

		default:
			SSDFS_ERR("fail to find superblock segment\n");
			return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
		}
	}

	superblock_seg = &creation_point->superblock_seg;
	log = &superblock_seg->logs[copy_index];
	hdr = &log->header.seg_hdr;
	memcpy(hdr, &cur_hdr, sg_size);

	log->peb_id = peb_id;
	log->start_page = 0;

	do {
		leb2peb_pair = &hdr->volume_hdr.sb_pebs[next_index][copy_index];
		peb_id = le64_to_cpu(leb2peb_pair->peb_id);

		res = ssdfs_fsck_find_metadata_segment(env,
							peb_id,
							seg_type,
							0,
							&cur_hdr);
		switch (res) {
		case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
			log->peb_id = peb_id;
			log->start_page = 0;
			memcpy(hdr, &cur_hdr, sg_size);
			break;

		default:
			break;
		}

		number_of_tries++;
		if (number_of_tries >= pebs_per_volume) {
			SSDFS_ERR("fail to find superblock segment: "
				  "number_of_tries %llu, pebs_per_volume %llu\n",
				  number_of_tries, pebs_per_volume);
			return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
		}
	} while (res == SSDFS_FSCK_SEARCH_RESULT_SUCCESS);

	res = ssdfs_fsck_find_last_full_log(env, log);
	switch (res) {
	case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
	case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
		/* continue logic */
		break;

	default:
		SSDFS_ERR("fail to find last full log\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return SSDFS_FSCK_SEARCH_RESULT_SUCCESS;
}

static
int is_main_superblock_segment_found(struct ssdfs_fsck_environment *env)
{
	return __is_superblock_segment_found(env, SSDFS_MAIN_SB_SEG);
}

static
int is_copy_superblock_segment_found(struct ssdfs_fsck_environment *env)
{
	return __is_superblock_segment_found(env, SSDFS_COPY_SB_SEG);
}

static
int is_mapping_table_cache_found(struct ssdfs_fsck_environment *env)
{
	struct ssdfs_fsck_volume_creation_point *creation_point;
	struct ssdfs_fsck_superblock_segment_detection *superblock_seg;
	struct ssdfs_fsck_mapping_table_cache *maptbl_cache;
	struct ssdfs_fsck_found_log *log;
	struct ssdfs_segment_header *hdr = NULL;
	struct ssdfs_metadata_descriptor *desc;
	u8 *buf;
	struct ssdfs_maptbl_cache_header *cache_hdr = NULL;
	size_t hdr_size = sizeof(struct ssdfs_maptbl_cache_header);
	u32 area_offset;
	u32 area_size;
	u16 flags;
	u16 bytes_count;
	u8 *data = NULL;
	u8 *uncompr_data = NULL;
	u32 compr_size, uncompr_size;
	int err;
	int res = SSDFS_FSCK_SEARCH_RESULT_SUCCESS;

	SSDFS_DBG(env->base.show_debug,
		  "Try to find mapping table cache\n");

	creation_point = ssdfs_fsck_get_creation_point(env, 0);

	if (!creation_point) {
		SSDFS_ERR("fail to get creation point\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	if (!(creation_point->found_metadata & SSDFS_FSCK_SB_SEGS_FOUND)) {
		SSDFS_ERR("superblock segment is not found\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	superblock_seg = &creation_point->superblock_seg;
	log = &superblock_seg->logs[SSDFS_MAIN_SB_SEG];
	hdr = &log->header.seg_hdr;

	desc = &hdr->desc_array[SSDFS_MAPTBL_CACHE_INDEX];
	area_offset = le32_to_cpu(desc->offset);
	area_size = le32_to_cpu(desc->size);

	if (!is_ssdfs_fsck_area_valid(desc)) {
		SSDFS_ERR("unable to find mappimng table cache: "
			  "metadata area descriptor is corrupted: "
			  "area_offset %u, area_size %u\n",
			  area_offset, area_size);
		return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;
	}

	maptbl_cache = &creation_point->maptbl_cache;

	BUG_ON(maptbl_cache->data);

	buf = malloc(area_size);
	if (!buf) {
		SSDFS_ERR("fail to allocate memory: "
			  "size %u\n", area_size);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	memset(buf, 0, area_size);

	err = ssdfs_read_mapping_table_cache(&env->base,
					     log->peb_id,
					     env->base.erase_size,
					     area_offset,
					     area_size,
					     buf);
	if (err) {
		SSDFS_ERR("fail to read mapping table cache: "
			  "peb_id %llu, peb_size %u, "
			  "area_offset %u, area_size %u, err %d\n",
			  log->peb_id, env->base.erase_size,
			  area_offset, area_size, err);
		res = SSDFS_FSCK_SEARCH_RESULT_FAILURE;
		goto finish_process_mapping_table_cache;
	}

	cache_hdr = (struct ssdfs_maptbl_cache_header *)(buf);
	memcpy(&maptbl_cache->hdr, cache_hdr, hdr_size);

	flags = le16_to_cpu(cache_hdr->flags);
	bytes_count = le16_to_cpu(cache_hdr->bytes_count);
	compr_size = area_size;
	uncompr_size = bytes_count;

	if (uncompr_size < compr_size) {
		SSDFS_ERR("mapping table cache's header is corrupted\n");
		res = SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;
		goto finish_process_mapping_table_cache;
	}

	uncompr_data = malloc(uncompr_size);
	if (!uncompr_data) {
		SSDFS_ERR("fail to allocate memory\n");
		res = SSDFS_FSCK_SEARCH_RESULT_FAILURE;
		goto finish_process_mapping_table_cache;
	}

	data = buf + hdr_size;

	if (flags & SSDFS_MAPTBL_CACHE_ZLIB_COMPR) {
		err = ssdfs_zlib_decompress(data, uncompr_data,
					    compr_size, uncompr_size,
					    env->base.show_debug);
		if (err) {
			SSDFS_ERR("fail to decompress: err %d\n",
				  err);
			free(uncompr_data);
			res = SSDFS_FSCK_SEARCH_RESULT_FAILURE;
			goto finish_process_mapping_table_cache;
		}

		maptbl_cache->data = uncompr_data;
		maptbl_cache->bytes_count = uncompr_size;
	} else if (flags & SSDFS_MAPTBL_CACHE_LZO_COMPR) {
		SSDFS_ERR("COMPRESSED STATE IS NOT SUPPORTED YET\n");
		free(uncompr_data);
		res = SSDFS_FSCK_SEARCH_RESULT_FAILURE;
		goto finish_process_mapping_table_cache;
	} else {
		maptbl_cache->data = uncompr_data;
		maptbl_cache->bytes_count = area_size - hdr_size;
		memcpy(maptbl_cache->data, data, maptbl_cache->bytes_count);
	}

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

finish_process_mapping_table_cache:
	if (buf)
		free(buf);

	return res;
}

static
int is_superblock_segments_found(struct ssdfs_fsck_environment *env)
{
	struct ssdfs_fsck_volume_creation_point *creation_point;

	SSDFS_DBG(env->base.show_debug,
		  "Try to find superblock segments\n");

	creation_point = ssdfs_fsck_get_creation_point(env, 0);

	if (!creation_point) {
		SSDFS_ERR("fail to get creation point\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	switch (is_main_superblock_segment_found(env)) {
	case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
		/* continue logic */
		break;

	case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
		SSDFS_DBG(env->base.show_debug,
			  "unable to find main superblock segment\n");
		return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;

	default:
		SSDFS_ERR("fail to find main superblock segment\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	switch (is_copy_superblock_segment_found(env)) {
	case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
		/* continue logic */
		break;

	case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
		SSDFS_DBG(env->base.show_debug,
			  "unable to find copy superblock segment\n");
		return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;

	default:
		SSDFS_ERR("fail to find copy superblock segment\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	creation_point->found_metadata |= SSDFS_FSCK_SB_SEGS_FOUND;

	switch (is_mapping_table_cache_found(env)) {
	case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
		/* continue logic */
		break;

	case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
		SSDFS_DBG(env->base.show_debug,
			  "unable to find mapping table cache\n");
		return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;

	default:
		SSDFS_ERR("fail to find mapping table cache\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	creation_point->found_metadata |= SSDFS_FSCK_MAPTBL_CACHE_FOUND;

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return SSDFS_FSCK_SEARCH_RESULT_SUCCESS;
}

static
int ssdfs_fsck_allocate_segbmap_logs_array(struct ssdfs_fsck_environment *env)
{
	struct ssdfs_fsck_volume_creation_point *creation_point;
	struct ssdfs_segbmap_sb_header *segbmap_hdr;
	struct ssdfs_fsck_segment_bitmap_detection *segbmap;
	struct ssdfs_fsck_found_log *log;
	u16 segs_count;
	u32 lebs_count;
	u32 lebs_per_seg;

	SSDFS_DBG(env->base.show_debug,
		  "Allocate segment bitmap found logs array\n");

	creation_point = ssdfs_fsck_get_creation_point(env, 0);

	if (!creation_point) {
		SSDFS_ERR("fail to get creation point\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	if (!(creation_point->found_metadata & SSDFS_FSCK_SB_SEGS_FOUND)) {
		SSDFS_ERR("unexpected state of found_metadata: "
			  "found_metadata %#llx\n",
			  creation_point->found_metadata);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	log = &creation_point->superblock_seg.logs[SSDFS_MAIN_SB_SEG];
	segbmap_hdr = &log->header.seg_hdr.volume_hdr.segbmap;

	segs_count = le16_to_cpu(segbmap_hdr->segs_count);

	if (segs_count == 0 || segs_count > SSDFS_SEGBMAP_SEGS) {
		SSDFS_ERR("invalid number of segment bitmap segments: "
			  "segs_count %u\n", segs_count);
		return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;
	}

	lebs_per_seg = env->seg_size / env->base.erase_size;
	BUG_ON(lebs_per_seg == 0);
	lebs_count = segs_count * lebs_per_seg;
	BUG_ON(lebs_count == 0);
	BUG_ON(lebs_count > (SSDFS_SEGBMAP_SEGS * lebs_per_seg));

	segbmap = &creation_point->segbmap;
	segbmap->segs_count = segs_count;
	memcpy(&segbmap->segbmap_sb_hdr, segbmap_hdr,
		sizeof(struct ssdfs_segbmap_sb_header));

	segbmap->array.pairs = calloc(lebs_count,
					sizeof(struct ssdfs_fsck_logs_pair));
	if (!segbmap->array.pairs) {
		SSDFS_ERR("fail to allocate memory for segment bitmap's logs\n");
		return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;
	}

	segbmap->array.capacity = lebs_count;
	segbmap->array.count = 0;

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return SSDFS_FSCK_SEARCH_RESULT_SUCCESS;
}

static
u64 ssdfs_maptbl_cache_convert_leb2peb(struct ssdfs_fsck_environment *env,
					u64 leb_id)
{
	struct ssdfs_fsck_volume_creation_point *creation_point;
	struct ssdfs_fsck_mapping_table_cache *maptbl_cache;
	struct ssdfs_maptbl_cache_header *cache_hdr;
	u16 items_count;
	u16 bytes_count;
	u16 processed_bytes = 0;
	u64 start_leb;
	u64 end_leb;
	__le64 *cur_id = NULL;
	int i;

	SSDFS_DBG(env->base.show_debug,
		  "Convert LEB to PEB: "
		  "leb_id %llu\n",
		  leb_id);

	creation_point = ssdfs_fsck_get_creation_point(env, 0);

	if (!creation_point) {
		SSDFS_ERR("fail to get creation point\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	if (!(creation_point->found_metadata & SSDFS_FSCK_MAPTBL_CACHE_FOUND)) {
		SSDFS_ERR("Mapping table cache is not found\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	maptbl_cache = &creation_point->maptbl_cache;
	cache_hdr = &maptbl_cache->hdr;

	items_count = le16_to_cpu(cache_hdr->items_count);
	bytes_count = le16_to_cpu(cache_hdr->bytes_count);
	start_leb = le16_to_cpu(cache_hdr->start_leb);
	end_leb = le16_to_cpu(cache_hdr->end_leb);

	if (leb_id < start_leb || leb_id > end_leb) {
		SSDFS_ERR("unable to find LEB ID: "
			  "start_leb %llu, end_leb %llu\n",
			  start_leb, end_leb);
		return U64_MAX;
	}

	if (bytes_count < sizeof(__le64)) {
		SSDFS_ERR("invalid bytes_count %u\n",
			  bytes_count);
		return U64_MAX;
	}

	cur_id = (__le64 *)maptbl_cache->data;
	for (i = 0; i < items_count; i++) {
		u64 cur_leb_id;
		u64 cur_peb_id;

		cur_leb_id = le64_to_cpu(*cur_id);
		cur_id++;
		processed_bytes += sizeof(__le64);

		if (processed_bytes >= bytes_count) {
			SSDFS_ERR("unable to find LEB ID: "
				  "processed_bytes %u, bytes_count %u\n",
				  processed_bytes, bytes_count);
			return U64_MAX;
		}

		cur_peb_id = le64_to_cpu(*cur_id);

		if (cur_leb_id == leb_id)
			return cur_peb_id;

		cur_id++;
		processed_bytes += sizeof(__le64);

		if (processed_bytes >= bytes_count) {
			SSDFS_ERR("unable to find LEB ID: "
				  "processed_bytes %u, bytes_count %u\n",
				  processed_bytes, bytes_count);
			return U64_MAX;
		}
	}

	SSDFS_ERR("unable to find LEB ID: "
		  "processed_bytes %u, bytes_count %u\n",
		  processed_bytes, bytes_count);
	return U64_MAX;
}

static
int ssdfs_fsck_find_segbmap_peb(struct ssdfs_fsck_environment *env,
				struct ssdfs_fsck_found_log *log,
				u64 peb_id)
{
	struct ssdfs_segment_header *hdr;
	int seg_type = SSDFS_SEGBMAP_SEG_TYPE;
	int res;

	SSDFS_DBG(env->base.show_debug,
		  "Find segment bitmap's PEB: "
		  "peb_id %llu\n",
		  peb_id);

	log->peb_id = U64_MAX;
	log->start_page = U32_MAX;

	hdr = &log->header.seg_hdr;
	res = ssdfs_fsck_find_metadata_segment(env,
						peb_id,
						seg_type,
						0,
						hdr);
	switch (res) {
	case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
		/* continue logic */
		break;

	case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
		SSDFS_DBG(env->base.show_debug,
			  "unable to find segment bitmap's PEB: "
			  "peb_id %llu\n", peb_id);
		return res;

	default:
		SSDFS_ERR("fail to find segment bitmap's PEB: "
			  "peb_id %llu\n", peb_id);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	log->peb_id = peb_id;
	log->start_page = 0;

	res = ssdfs_fsck_find_last_full_log(env, log);
	switch (res) {
	case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
	case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
		/* continue logic */
		break;

	default:
		SSDFS_ERR("fail to find last full log\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	res = ssdfs_fsck_find_last_partial_log(env, log);
	switch (res) {
	case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
	case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
		/* continue logic */
		break;

	default:
		SSDFS_ERR("fail to find last partial log\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return SSDFS_FSCK_SEARCH_RESULT_SUCCESS;
}

static
int __ssdfs_fsck_find_segbmap_segment(struct ssdfs_fsck_environment *env,
				      int seg_index,
				      int copy_index)
{
	struct ssdfs_fsck_volume_creation_point *creation_point;
	struct ssdfs_fsck_segment_bitmap_detection *segbmap;
	u16 flags;
	u64 seg_id;
	u64 start_leb_id;
	u64 leb_id;
	u32 lebs_per_seg;
	u64 peb_id;
	u16 fragments_per_seg;
	u16 fragments_per_peb;
	u16 processed_fragments = 0;
	int i;
	int res;

	SSDFS_DBG(env->base.show_debug,
		  "Find segment bitmap's segment: "
		  "seg_index %d, copy_index %d\n",
		  seg_index, copy_index);

	creation_point = ssdfs_fsck_get_creation_point(env, 0);

	if (!creation_point) {
		SSDFS_ERR("fail to get creation point\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	segbmap = &creation_point->segbmap;

	if (segbmap->segs_count == 0 ||
	    segbmap->segs_count > SSDFS_SEGBMAP_SEGS) {
		SSDFS_ERR("invalid number of segment bitmap segments: "
			  "segs_count %u\n", segbmap->segs_count);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	if (seg_index >= segbmap->segs_count) {
		SSDFS_ERR("invalid segment index: "
			  "seg_index %d, segs_count %u\n",
			  seg_index, segbmap->segs_count);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	if (copy_index >= SSDFS_FSCK_LOGS_NUMBER_MAX) {
		SSDFS_ERR("invalid copy_index: "
			  "copy_index %d, logs_number_max %u\n",
			  copy_index,
			  (u32)SSDFS_FSCK_LOGS_NUMBER_MAX);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	flags = le16_to_cpu(segbmap->segbmap_sb_hdr.flags);

	switch (copy_index) {
	case SSDFS_FSCK_COPY_LOG:
		if (flags & SSDFS_SEGBMAP_HAS_COPY) {
			/*
			 * continue logic
			 */
		} else {
			SSDFS_DBG(env->base.show_debug,
				  "There is no copy of segment bitmap: "
				  "seg_index %d, copy_index %d\n",
				  seg_index, copy_index);
			goto finish_search_segmap_segment;
		}
		break;

	default:
		/* continue logic */
		break;
	}

	seg_id = le64_to_cpu(segbmap->segbmap_sb_hdr.segs[seg_index][copy_index]);

	if (seg_id >= U64_MAX) {
		SSDFS_DBG(env->base.show_debug,
			  "Found invalid segment ID: "
			  "seg_index %d, copy_index %d\n",
			  seg_index, copy_index);
		return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;
	}

	fragments_per_seg = le16_to_cpu(segbmap->segbmap_sb_hdr.fragments_per_seg);

	if (fragments_per_seg == 0 || fragments_per_seg >= U16_MAX) {
		SSDFS_DBG(env->base.show_debug,
			  "Found invalid fragments_per_seg: "
			  "seg_index %d, copy_index %d\n",
			  seg_index, copy_index);
		return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;
	}

	fragments_per_peb = le16_to_cpu(segbmap->segbmap_sb_hdr.fragments_per_peb);

	if (fragments_per_peb == 0 || fragments_per_peb >= U16_MAX) {
		SSDFS_DBG(env->base.show_debug,
			  "Found invalid fragments_per_peb: "
			  "seg_index %d, copy_index %d\n",
			  seg_index, copy_index);
		return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;
	}

	lebs_per_seg = env->seg_size / env->base.erase_size;
	BUG_ON(lebs_per_seg == 0);

	if (fragments_per_seg % fragments_per_peb ||
	    (fragments_per_seg / fragments_per_peb) > lebs_per_seg) {
		SSDFS_DBG(env->base.show_debug,
			  "Found invalid fragments_per_peb: "
			  "seg_index %d, copy_index %d, "
			  "fragments_per_peb %u, fragments_per_seg %u, "
			  "lebs_per_seg %u\n",
			  seg_index, copy_index,
			  fragments_per_peb, fragments_per_seg,
			  lebs_per_seg);
		return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;
	}

	start_leb_id = seg_id * lebs_per_seg;

	for (i = 0; i < lebs_per_seg; i++) {
		struct ssdfs_fsck_found_log *log;
		leb_id = start_leb_id + i;

		if (processed_fragments >= fragments_per_seg)
			goto finish_search_segmap_segment;

		peb_id = ssdfs_maptbl_cache_convert_leb2peb(env, leb_id);
		if (peb_id >= U64_MAX) {
			SSDFS_DBG(env->base.show_debug,
				  "Found invalid PEB ID: "
				  "seg_id %llu, leb_id %llu\n",
				  seg_id, leb_id);
			return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;
		}

		log = &segbmap->array.pairs[segbmap->array.count].logs[copy_index];

		res = ssdfs_fsck_find_segbmap_peb(env, log, peb_id);
		switch (res) {
		case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
			/* continue logic */
			break;

		case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
			SSDFS_DBG(env->base.show_debug,
				  "unable to find segment bitmap's PEB: "
				  "seg_id %llu, leb_id %llu, peb_id %llu\n",
				  seg_id, leb_id, peb_id);
			return res;

		default:
			SSDFS_DBG(env->base.show_debug,
				  "fail to find segment bitmap's PEB: "
				  "seg_id %llu, leb_id %llu, peb_id %llu\n",
				  seg_id, leb_id, peb_id);
			return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
		}

		processed_fragments += fragments_per_peb;
	}

finish_search_segmap_segment:
	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return SSDFS_FSCK_SEARCH_RESULT_SUCCESS;
}

static inline
int ssdfs_fsck_find_segbmap_segment(struct ssdfs_fsck_environment *env,
				    int seg_index)
{
	struct ssdfs_fsck_volume_creation_point *creation_point;
	struct ssdfs_fsck_segment_bitmap_detection *segbmap;
	int res;

	SSDFS_DBG(env->base.show_debug,
		  "Find segment bitmap's segment: "
		  "seg_index %d\n",
		  seg_index);

	creation_point = ssdfs_fsck_get_creation_point(env, 0);

	if (!creation_point) {
		SSDFS_ERR("fail to get creation point\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	segbmap = &creation_point->segbmap;

	res = __ssdfs_fsck_find_segbmap_segment(env, seg_index,
						SSDFS_FSCK_MAIN_LOG);
	switch (res) {
	case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
		/* continue logic */
		break;

	case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
		SSDFS_DBG(env->base.show_debug,
			  "unable to find main segment bitmap's segment: "
			  "seg_index %d\n", seg_index);
		return res;

	default:
		SSDFS_ERR("fail to find main segment bitmap's segment: "
			  "seg_index %d\n", seg_index);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	res = __ssdfs_fsck_find_segbmap_segment(env, seg_index,
						SSDFS_FSCK_COPY_LOG);
	switch (res) {
	case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
		/* continue logic */
		break;

	case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
		SSDFS_DBG(env->base.show_debug,
			  "unable to find copy segment bitmap's segment: "
			  "seg_index %d\n", seg_index);
		return res;

	default:
		SSDFS_ERR("fail to find copy segment bitmap's segment: "
			  "seg_index %d\n", seg_index);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	segbmap->array.count++;

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return SSDFS_FSCK_SEARCH_RESULT_SUCCESS;
}

static
int is_segment_bitmap_found(struct ssdfs_fsck_environment *env)
{
	struct ssdfs_fsck_volume_creation_point *creation_point;
	u32 segs_count;
	int i;
	int res;

	SSDFS_DBG(env->base.show_debug,
		  "Try to find segment bitmap\n");

	creation_point = ssdfs_fsck_get_creation_point(env, 0);

	if (!creation_point) {
		SSDFS_ERR("fail to get creation point\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	res = ssdfs_fsck_allocate_segbmap_logs_array(env);
	switch (res) {
	case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
		/* continue logic */
		break;

	case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
		SSDFS_DBG(env->base.show_debug,
			  "unable to find segment bitmap\n");
		return res;

	default:
		SSDFS_ERR("fail to allocate segbmap found logs array\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	segs_count = creation_point->segbmap.segs_count;

	if (segs_count == 0 || segs_count >= U32_MAX) {
		SSDFS_ERR("invalid segs_count %u\n",
			  segs_count);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	for (i = 0; i < segs_count; i++) {
		res = ssdfs_fsck_find_segbmap_segment(env, i);
		switch (res) {
		case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
			/* continue logic */
			break;

		case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
			SSDFS_DBG(env->base.show_debug,
				  "unable to find segment bitmap\n");
			return res;

		default:
			SSDFS_ERR("fail to find segment bitmap: "
				  "segment index %d\n", i);
			return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
		}
	}

	creation_point->found_metadata |= SSDFS_FSCK_SEGMENT_BITMAP_FOUND;

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return SSDFS_FSCK_SEARCH_RESULT_SUCCESS;
}

static
int ssdfs_fsck_allocate_maptbl_logs_array(struct ssdfs_fsck_environment *env)
{
	struct ssdfs_fsck_volume_creation_point *creation_point;
	struct ssdfs_maptbl_sb_header *maptbl_hdr;
	struct ssdfs_fsck_mapping_table_detection *maptbl;
	struct ssdfs_fsck_found_log *log;
	u64 volume_segs;
	u16 segs_count;
	u32 lebs_count;
	u32 lebs_per_seg;
	u64 threshold;
	u32 extents_count;
	int i, j;

	SSDFS_DBG(env->base.show_debug,
		  "Allocate mapping table found logs array\n");

	creation_point = ssdfs_fsck_get_creation_point(env, 0);

	if (!creation_point) {
		SSDFS_ERR("fail to get creation point\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	if (!(creation_point->found_metadata & SSDFS_FSCK_SB_SEGS_FOUND)) {
		SSDFS_ERR("unexpected state of found_metadata: "
			  "found_metadata %#llx\n",
			  creation_point->found_metadata);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	log = &creation_point->superblock_seg.logs[SSDFS_MAIN_SB_SEG];
	maptbl_hdr = &log->header.seg_hdr.volume_hdr.maptbl;

	lebs_per_seg = env->seg_size / env->base.erase_size;
	BUG_ON(lebs_per_seg == 0);

	segs_count = 0;
	extents_count = 0;

	for (i = 0; i < MAPTBL_LIMIT1; i++) {
		struct ssdfs_meta_area_extent *extent;
		u32 len;

		for (j = 0; j < MAPTBL_LIMIT2; j++) {
			extent = &maptbl_hdr->extents[i][j];

			SSDFS_DBG(env->base.show_debug,
				  "extent[%d][%d]: (start_id %llu, len %u, type %#x)\n",
				  i, j,
				  le64_to_cpu(extent->start_id),
				  le32_to_cpu(extent->len),
				  le16_to_cpu(extent->type));

			switch (le16_to_cpu(extent->type)) {
			case SSDFS_EMPTY_EXTENT_TYPE:
				/* skip empty extent */
				continue;

			case SSDFS_SEG_EXTENT_TYPE:
				segs_count += le32_to_cpu(extent->len);
				extents_count++;
				break;

			case SSDFS_PEB_EXTENT_TYPE:
				len = le32_to_cpu(extent->len);
				BUG_ON(len % lebs_per_seg);
				segs_count += len / lebs_per_seg;
				extents_count++;
				break;

			case SSDFS_BLK_EXTENT_TYPE:
				SSDFS_ERR("unexpected extent type %#x\n",
					  le16_to_cpu(extent->type));
				return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;

			default:
				if (extents_count > 0) {
					/* skip invalid extent */
				} else {
					SSDFS_ERR("unexpected extent type %#x\n",
						  le16_to_cpu(extent->type));
					return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;
				}
			}
		}
	}

	volume_segs = env->base.fs_size / env->seg_size;
	threshold = (volume_segs * 25) / 100;

	if (segs_count == 0 || segs_count > threshold) {
		SSDFS_ERR("invalid number of mapping table segments: "
			  "segs_count %u\n", segs_count);
		return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;
	}

	if (extents_count == 0 || extents_count > MAPTBL_LIMIT1) {
		SSDFS_ERR("invalid number of mapping table extents: "
			  "extents_count %u\n", extents_count);
		return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;
	}

	lebs_count = segs_count * lebs_per_seg;
	BUG_ON(lebs_count == 0);
	BUG_ON(lebs_count > (threshold * lebs_per_seg));

	maptbl = &creation_point->maptbl;
	maptbl->segs_count = segs_count;
	maptbl->found_segs = 0;
	maptbl->extents_count = extents_count;
	memcpy(&maptbl->maptbl_sb_hdr, maptbl_hdr,
		sizeof(struct ssdfs_maptbl_sb_header));

	SSDFS_DBG(env->base.show_debug,
		  "extents_count %u, lebs_count %u\n",
		  extents_count, lebs_count);

	maptbl->array.pairs = calloc(lebs_count,
					sizeof(struct ssdfs_fsck_logs_pair));
	if (!maptbl->array.pairs) {
		SSDFS_ERR("fail to allocate memory for mapping table's logs\n");
		return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;
	}

	maptbl->array.capacity = lebs_count;
	maptbl->array.count = 0;

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return SSDFS_FSCK_SEARCH_RESULT_SUCCESS;
}

static
int __ssdfs_fsck_find_maptbl_peb(struct ssdfs_fsck_environment *env,
				struct ssdfs_fsck_found_log *log,
				u64 peb_id)
{
	struct ssdfs_segment_header *hdr;
	int seg_type = SSDFS_MAPTBL_SEG_TYPE;
	int res;

	SSDFS_DBG(env->base.show_debug,
		  "Find mapping table's PEB: "
		  "peb_id %llu\n",
		  peb_id);

	log->peb_id = U64_MAX;
	log->start_page = U32_MAX;

	hdr = &log->header.seg_hdr;
	res = ssdfs_fsck_find_metadata_segment(env,
						peb_id,
						seg_type,
						0,
						hdr);
	switch (res) {
	case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
		/* continue logic */
		break;

	case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
		SSDFS_DBG(env->base.show_debug,
			  "unable to find mapping table's PEB: "
			  "peb_id %llu\n", peb_id);
		return res;

	default:
		SSDFS_ERR("fail to find mapping table's PEB: "
			  "peb_id %llu\n", peb_id);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	log->peb_id = peb_id;
	log->start_page = 0;

	res = ssdfs_fsck_find_last_full_log(env, log);
	switch (res) {
	case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
	case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
		/* continue logic */
		break;

	default:
		SSDFS_ERR("fail to find last full log\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	res = ssdfs_fsck_find_last_partial_log(env, log);
	switch (res) {
	case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
	case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
		/* continue logic */
		break;

	default:
		SSDFS_ERR("fail to find last partial log\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return SSDFS_FSCK_SEARCH_RESULT_SUCCESS;
}

static
int ssdfs_fsck_find_maptbl_peb(struct ssdfs_fsck_environment *env,
				u64 leb_id, int copy_index)
{
	struct ssdfs_fsck_volume_creation_point *creation_point;
	struct ssdfs_fsck_mapping_table_detection *maptbl;
	struct ssdfs_fsck_found_log *log;
	u64 peb_id;
	int res;

	SSDFS_DBG(env->base.show_debug,
		  "Find mapping table's PEB: "
		  "leb_id %llu, copy_index %d\n",
		  leb_id, copy_index);

	if (leb_id >= U64_MAX) {
		SSDFS_DBG(env->base.show_debug,
			  "Invalid LEB: "
			  "leb_id %llu\n",
			  leb_id);
		return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;
	}

	if (copy_index >= SSDFS_FSCK_LOGS_NUMBER_MAX) {
		SSDFS_ERR("invalid copy_index: "
			  "copy_index %d, logs_number_max %u\n",
			  copy_index,
			  (u32)SSDFS_FSCK_LOGS_NUMBER_MAX);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	creation_point = ssdfs_fsck_get_creation_point(env, 0);

	if (!creation_point) {
		SSDFS_ERR("fail to get creation point\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	maptbl = &creation_point->maptbl;

	peb_id = ssdfs_maptbl_cache_convert_leb2peb(env, leb_id);
	if (peb_id >= U64_MAX) {
		SSDFS_DBG(env->base.show_debug,
			  "Found invalid PEB ID: "
			  "leb_id %llu\n",
			  leb_id);
		return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;
	}

	log = &maptbl->array.pairs[maptbl->array.count].logs[copy_index];

	res = __ssdfs_fsck_find_maptbl_peb(env, log, peb_id);
	switch (res) {
	case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
		/* continue logic */
		break;

	case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
		SSDFS_DBG(env->base.show_debug,
			  "unable to find mapping table's PEB: "
			  "leb_id %llu, peb_id %llu\n",
			  leb_id, peb_id);
		return res;

	default:
		SSDFS_DBG(env->base.show_debug,
			  "fail to find mapping table's PEB: "
			  "leb_id %llu, peb_id %llu\n",
			  leb_id, peb_id);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return SSDFS_FSCK_SEARCH_RESULT_SUCCESS;
}

static
int ssdfs_fsck_find_maptbl_segment(struct ssdfs_fsck_environment *env,
				   u64 seg_id, int copy_index)
{
	struct ssdfs_fsck_volume_creation_point *creation_point;
	struct ssdfs_fsck_mapping_table_detection *maptbl;
	u32 lebs_per_seg;
	u64 start_leb_id;
	u64 leb_id;
	u64 peb_id;
	int i;
	int res;

	SSDFS_DBG(env->base.show_debug,
		  "Find mapping table's segment: "
		  "seg_id %llu, copy_index %d\n",
		  seg_id, copy_index);

	if (seg_id >= U64_MAX) {
		SSDFS_DBG(env->base.show_debug,
			  "Invalid segment ID: "
			  "seg_id %llu\n",
			  seg_id);
		return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;
	}

	if (copy_index >= SSDFS_FSCK_LOGS_NUMBER_MAX) {
		SSDFS_ERR("invalid copy_index: "
			  "copy_index %d, logs_number_max %u\n",
			  copy_index,
			  (u32)SSDFS_FSCK_LOGS_NUMBER_MAX);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	creation_point = ssdfs_fsck_get_creation_point(env, 0);

	if (!creation_point) {
		SSDFS_ERR("fail to get creation point\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	maptbl = &creation_point->maptbl;

	lebs_per_seg = env->seg_size / env->base.erase_size;
	BUG_ON(lebs_per_seg == 0);

	start_leb_id = seg_id * lebs_per_seg;

	for (i = 0; i < lebs_per_seg; i++) {
		struct ssdfs_fsck_found_log *log;
		leb_id = start_leb_id + i;

		peb_id = ssdfs_maptbl_cache_convert_leb2peb(env, leb_id);
		if (peb_id >= U64_MAX) {
			SSDFS_DBG(env->base.show_debug,
				  "Found invalid PEB ID: "
				  "seg_id %llu, leb_id %llu\n",
				  seg_id, leb_id);
			return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;
		}

		log = &maptbl->array.pairs[maptbl->array.count].logs[copy_index];

		res = __ssdfs_fsck_find_maptbl_peb(env, log, peb_id);
		switch (res) {
		case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
			/* continue logic */
			break;

		case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
			SSDFS_DBG(env->base.show_debug,
				  "unable to find segment bitmap's PEB: "
				  "seg_id %llu, leb_id %llu, peb_id %llu\n",
				  seg_id, leb_id, peb_id);
			return res;

		default:
			SSDFS_DBG(env->base.show_debug,
				  "fail to find segment bitmap's PEB: "
				  "seg_id %llu, leb_id %llu, peb_id %llu\n",
				  seg_id, leb_id, peb_id);
			return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
		}
	}

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return SSDFS_FSCK_SEARCH_RESULT_SUCCESS;
}

static inline
int ssdfs_fsck_find_maptbl_extent(struct ssdfs_fsck_environment *env,
				  int extent_index)
{
	struct ssdfs_fsck_volume_creation_point *creation_point;
	struct ssdfs_fsck_mapping_table_detection *maptbl;
	struct ssdfs_maptbl_sb_header *maptbl_hdr;
	struct ssdfs_meta_area_extent *extent;
	u32 lebs_per_seg;
	u64 start_id;
	u32 len;
	u16 flags;
	int i;
	int res;

	SSDFS_DBG(env->base.show_debug,
		  "Find mapping table's extent: "
		  "extent_index %d\n",
		  extent_index);

	creation_point = ssdfs_fsck_get_creation_point(env, 0);

	if (!creation_point) {
		SSDFS_ERR("fail to get creation point\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	maptbl = &creation_point->maptbl;
	maptbl_hdr = &maptbl->maptbl_sb_hdr;

	lebs_per_seg = env->seg_size / env->base.erase_size;
	BUG_ON(lebs_per_seg == 0);

	if (extent_index >= MAPTBL_LIMIT1) {
		SSDFS_ERR("invalid extent index: "
			  "extent_index %d, upper_limit %u\n",
			  extent_index,  (u32)MAPTBL_LIMIT1);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	extent = &maptbl_hdr->extents[extent_index][SSDFS_FSCK_MAIN_LOG];

	switch (le16_to_cpu(extent->type)) {
	case SSDFS_EMPTY_EXTENT_TYPE:
		/* skip empty extent */
		return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;

	case SSDFS_SEG_EXTENT_TYPE:
		start_id = le64_to_cpu(extent->start_id);
		len = le32_to_cpu(extent->len);

		for (i = 0; i < len; i++) {
			res = ssdfs_fsck_find_maptbl_segment(env,
							     start_id + i,
							     SSDFS_FSCK_MAIN_LOG);
			switch (res) {
			case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
				/* continue logic */
				break;

			case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
				SSDFS_DBG(env->base.show_debug,
					  "unable to find mapping table\n");
				return res;

			default:
				SSDFS_ERR("fail to find mapping table: "
					  "seg_id %llu\n", start_id + i);
				return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
			}
		}

		maptbl->found_segs += len;
		break;

	case SSDFS_PEB_EXTENT_TYPE:
		start_id = le64_to_cpu(extent->start_id);
		len = le32_to_cpu(extent->len);

		for (i = 0; i < len; i++) {
			res = ssdfs_fsck_find_maptbl_peb(env,
							 start_id + i,
							 SSDFS_FSCK_MAIN_LOG);
			switch (res) {
			case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
				/* continue logic */
				break;

			case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
				SSDFS_DBG(env->base.show_debug,
					  "unable to find mapping table\n");
				return res;

			default:
				SSDFS_ERR("fail to find mapping table: "
					  "seg_id %llu\n", start_id + i);
				return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
			}
		}

		BUG_ON(len % lebs_per_seg);
		maptbl->found_segs += len / lebs_per_seg;
		break;

	case SSDFS_BLK_EXTENT_TYPE:
		SSDFS_ERR("unexpected extent type %#x\n",
			  le16_to_cpu(extent->type));
		return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;

	default:
		SSDFS_ERR("unexpected extent type %#x\n",
			  le16_to_cpu(extent->type));
		return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;
	}

	extent = &maptbl_hdr->extents[extent_index][SSDFS_FSCK_COPY_LOG];
	flags = le16_to_cpu(maptbl_hdr->flags);

	switch (le16_to_cpu(extent->type)) {
	case SSDFS_EMPTY_EXTENT_TYPE:
		if (flags & SSDFS_MAPTBL_HAS_COPY) {
			/* backup copy is not found */
			return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;
		} else {
			/* backup copy is absent */
			return SSDFS_FSCK_SEARCH_RESULT_SUCCESS;
		}

	case SSDFS_SEG_EXTENT_TYPE:
		start_id = le64_to_cpu(extent->start_id);
		len = le32_to_cpu(extent->len);

		for (i = 0; i < len; i++) {
			res = ssdfs_fsck_find_maptbl_segment(env,
							     start_id + i,
							     SSDFS_FSCK_COPY_LOG);
			switch (res) {
			case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
				/* continue logic */
				break;

			case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
				SSDFS_DBG(env->base.show_debug,
					  "unable to find mapping table\n");
				return res;

			default:
				SSDFS_ERR("fail to find mapping table: "
					  "seg_id %llu\n", start_id + i);
				return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
			}
		}

		maptbl->found_segs += len;
		break;

	case SSDFS_PEB_EXTENT_TYPE:
		start_id = le64_to_cpu(extent->start_id);
		len = le32_to_cpu(extent->len);

		for (i = 0; i < len; i++) {
			res = ssdfs_fsck_find_maptbl_peb(env,
							 start_id + i,
							 SSDFS_FSCK_COPY_LOG);
			switch (res) {
			case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
				/* continue logic */
				break;

			case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
				SSDFS_DBG(env->base.show_debug,
					  "unable to find mapping table\n");
				return res;

			default:
				SSDFS_ERR("fail to find mapping table: "
					  "seg_id %llu\n", start_id + i);
				return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
			}
		}

		BUG_ON(len % lebs_per_seg);
		maptbl->found_segs += len / lebs_per_seg;
		break;

	case SSDFS_BLK_EXTENT_TYPE:
		SSDFS_ERR("unexpected extent type %#x\n",
			  le16_to_cpu(extent->type));
		return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;

	default:
		if (flags & SSDFS_MAPTBL_HAS_COPY) {
			/* backup copy is not found */
			return SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND;
		} else {
			/* backup copy is absent */
			return SSDFS_FSCK_SEARCH_RESULT_SUCCESS;
		}
	}

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return SSDFS_FSCK_SEARCH_RESULT_SUCCESS;
}

static
int is_mapping_table_found(struct ssdfs_fsck_environment *env)
{
	struct ssdfs_fsck_volume_creation_point *creation_point;
	u32 extents_count;
	int i;
	int res;

	SSDFS_DBG(env->base.show_debug,
		  "Try to find mapping table\n");

	creation_point = ssdfs_fsck_get_creation_point(env, 0);

	if (!creation_point) {
		SSDFS_ERR("fail to get creation point\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	res = ssdfs_fsck_allocate_maptbl_logs_array(env);
	switch (res) {
	case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
		/* continue logic */
		break;

	case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
		SSDFS_DBG(env->base.show_debug,
			  "unable to find mapping table\n");
		return res;

	default:
		SSDFS_ERR("fail to allocate maptbl found logs array\n");
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	extents_count = creation_point->maptbl.extents_count;

	if (extents_count == 0 || extents_count > MAPTBL_LIMIT1) {
		SSDFS_ERR("invalid extents_count %u\n",
			  extents_count);
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	for (i = 0; i < extents_count; i++) {
		res = ssdfs_fsck_find_maptbl_extent(env, i);
		switch (res) {
		case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
			/* continue logic */
			break;

		case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
			SSDFS_DBG(env->base.show_debug,
				  "unable to find mapping table\n");
			return res;

		default:
			SSDFS_ERR("fail to find mapping table: "
				  "extent index %d\n", i);
			return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
		}
	}

	creation_point->found_metadata |= SSDFS_FSCK_MAPPING_TBL_FOUND;

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return SSDFS_FSCK_SEARCH_RESULT_SUCCESS;
}

enum {
	SSDFS_FSCK_BASE_SNAPSHOT_SEG_SEARCH_FUNCTION,
	SSDFS_FSCK_SUPERBLOCK_SEG_SEARCH_FUNCTION,
	SSDFS_FSCK_SEGMENT_BITMAP_SEARCH_FUNCTION,
	SSDFS_FSCK_MAPPING_TABLE_SEARCH_FUNCTION,
	SSDFS_FSCK_SEARCH_FUNCTION_MAX
};

static detect_fn detect_actions[SSDFS_FSCK_SEARCH_FUNCTION_MAX] = {
/* 00 */	is_base_snapshot_segment_found,
/* 01 */	is_superblock_segments_found,
/* 02 */	is_segment_bitmap_found,
/* 03 */	is_mapping_table_found,
};

static inline
int is_ssdfs_metadata_map_exhausted(struct ssdfs_metadata_map *metadata_map)
{
	if (metadata_map->capacity == 0)
		return SSDFS_TRUE;
	else if (metadata_map->count >= metadata_map->capacity)
		return SSDFS_TRUE;

	return SSDFS_FALSE;
}

static
int ssdfs_fsck_resize_metadata_map(struct ssdfs_metadata_map *metadata_map)
{
	size_t item_size = sizeof(struct ssdfs_metadata_peb_item);
	int capacity;

	if (!is_ssdfs_metadata_map_exhausted(metadata_map)) {
		return 0;
	}

	if (metadata_map->capacity == 0) {
		capacity = SSDFS_FSCK_METADATA_MAP_CAPACITY_MIN;

		metadata_map->array = calloc(capacity, item_size);
		if (!metadata_map->array) {
			SSDFS_ERR("cannot allocate memory: "
				  "count %d, item size %zu, err: %s\n",
				  capacity, item_size,
				  strerror(errno));
			return errno;
		}

		metadata_map->capacity = capacity;
	} else {
		size_t bytes_count;

		capacity = metadata_map->capacity * 2;
		bytes_count = (size_t)capacity * item_size;

		metadata_map->array = realloc(metadata_map->array, bytes_count);
		if (!metadata_map->array) {
			SSDFS_ERR("fail to re-allocate buffer: "
				  "size %zu, err: %s\n",
				  bytes_count,
				  strerror(errno));
			return errno;
		}

		metadata_map->capacity = capacity;
	}

	return 0;
}

static
int ssdfs_fsck_metadata_map_add_peb_descriptor(struct ssdfs_thread_state *state,
						struct ssdfs_segment_header *hdr)
{
	struct ssdfs_metadata_map *metadata_map;
	struct ssdfs_metadata_peb_item *item;
	int err;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu\n",
		  state->id, state->peb.id);

	metadata_map = &state->metadata_map;

	if (is_ssdfs_metadata_map_exhausted(metadata_map)) {
		err = ssdfs_fsck_resize_metadata_map(metadata_map);
		if (err) {
			SSDFS_ERR("fail to resize metadata map: "
				  "err %d\n", err);
			return err;
		}
	}

	item = &metadata_map->array[metadata_map->count];

	item->seg_id = le64_to_cpu(hdr->seg_id);
	item->leb_id = le64_to_cpu(hdr->leb_id);
	item->peb_id = state->peb.id;

	switch (le16_to_cpu(hdr->seg_type)) {
	case SSDFS_INITIAL_SNAPSHOT_SEG_TYPE:
		item->type = SSDFS_MAPTBL_INIT_SNAP_PEB_TYPE;
		break;

	case SSDFS_SB_SEG_TYPE:
		item->type = SSDFS_MAPTBL_SBSEG_PEB_TYPE;
		break;

	case SSDFS_SEGBMAP_SEG_TYPE:
		item->type = SSDFS_MAPTBL_SEGBMAP_PEB_TYPE;
		break;

	case SSDFS_MAPTBL_SEG_TYPE:
		item->type = SSDFS_MAPTBL_MAPTBL_PEB_TYPE;
		break;

	case SSDFS_LEAF_NODE_SEG_TYPE:
		item->type = SSDFS_MAPTBL_LNODE_PEB_TYPE;
		break;

	case SSDFS_HYBRID_NODE_SEG_TYPE:
		item->type = SSDFS_MAPTBL_HNODE_PEB_TYPE;
		break;

	case SSDFS_INDEX_NODE_SEG_TYPE:
		item->type = SSDFS_MAPTBL_IDXNODE_PEB_TYPE;
		break;

	default:
		SSDFS_ERR("unexpected segment type %#x\n",
			  le16_to_cpu(hdr->seg_type));
		return -EINVAL;
	}

	item->peb_creation_timestamp = le64_to_cpu(hdr->peb_create_time);
	item->volume_creation_timestamp = le64_to_cpu(hdr->volume_hdr.create_time);

	metadata_map->count++;

	SSDFS_DBG(state->base.show_debug,
		  "finished\n");

	return 0;
}

static
int ssdfs_fsck_process_peb(struct ssdfs_thread_state *state)
{
	struct ssdfs_segment_header hdr;
	struct ssdfs_signature *magic;
	size_t sg_size = sizeof(struct ssdfs_segment_header);
	int err;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu\n",
		  state->id, state->peb.id);

	err = ssdfs_read_segment_header(&state->base,
					state->peb.id,
					state->peb.peb_size,
					0,
					sg_size,
					&hdr);
	if (err) {
		SSDFS_ERR("fail to read segment header: "
			  "peb_id %llu, err %d\n",
			  state->peb.id, err);
		return err;
	}

	magic = &hdr.volume_hdr.magic;

	if (!is_ssdfs_segment_header(magic)) {
		/* ignore empty erase block */
		return 0;
	}

	switch (le16_to_cpu(hdr.seg_type)) {
	case SSDFS_INITIAL_SNAPSHOT_SEG_TYPE:
	case SSDFS_SB_SEG_TYPE:
	case SSDFS_SEGBMAP_SEG_TYPE:
	case SSDFS_MAPTBL_SEG_TYPE:
	case SSDFS_LEAF_NODE_SEG_TYPE:
	case SSDFS_HYBRID_NODE_SEG_TYPE:
	case SSDFS_INDEX_NODE_SEG_TYPE:
		err = ssdfs_fsck_metadata_map_add_peb_descriptor(state, &hdr);
		if (err) {
			SSDFS_ERR("fail to process erase block: "
				  "thread %d, PEB %llu, err %d\n",
				  state->id, state->peb.id, err);
			return err;
		}
		break;

	default:
		/* ignore not metadata erase block */
		return 0;
	}

	SSDFS_DBG(state->base.show_debug,
		  "finished\n");

	return 0;
}

void *ssdfs_fsck_process_peb_range(void *arg)
{
	struct ssdfs_thread_state *state = (struct ssdfs_thread_state *)arg;
	u64 per_1_percent = 0;
	u64 message_threshold = 0;
	u64 start_peb_id;
	u64 i;
	int err;

	if (!state)
		pthread_exit((void *)1);

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu\n",
		  state->id, state->peb.id);

	state->err = 0;
	start_peb_id = state->peb.id;

	per_1_percent = state->peb.pebs_count / 100;
	if (per_1_percent == 0)
		per_1_percent = 1;

	message_threshold = per_1_percent;

	SSDFS_FSCK_INFO(state->base.show_info,
			"thread %d, PEB %llu, percentage %u\n",
			state->id, state->peb.id, 0);

	for (i = 0; i < state->peb.pebs_count; i++) {
		state->peb.id = start_peb_id + i;

		if (i >= message_threshold) {
			SSDFS_FSCK_INFO(state->base.show_info,
					"thread %d, PEB %llu, percentage %llu\n",
					state->id, state->peb.id,
					(i / per_1_percent));

			message_threshold += per_1_percent;
		}

		err = ssdfs_fsck_process_peb(state);
		if (err) {
			SSDFS_ERR("fail to process PEB: "
				  "peb_id %llu, err %d\n",
				  state->peb.id, err);
		}
	}

	SSDFS_FSCK_INFO(state->base.show_info,
			"FINISHED: thread %d\n",
			state->id);

	pthread_exit((void *)0);
}

static
struct ssdfs_fsck_volume_creation_point *
ssdfs_fsck_find_creation_point(struct ssdfs_fsck_environment *env,
				u64 create_time)
{
	struct ssdfs_fsck_volume_creation_array *array = NULL;
	struct ssdfs_fsck_volume_creation_point *creation_point = NULL;
	int i;

	array = &env->detection_result.array;

	switch (array->state) {
	case SSDFS_FSCK_CREATION_ARRAY_USE_BUFFER:
	case SSDFS_FSCK_CREATION_ARRAY_ALLOCATED:
		/* continue logic */
		break;

	default:
		SSDFS_ERR("unexpected state of creation array: "
			  "state %#x\n", array->state);
		return NULL;
	}

	for (i = 0; i < array->count; i++) {
		creation_point = &array->creation_points[i];

		if (creation_point->volume_creation_timestamp == create_time)
			return creation_point;
	}

	return NULL;
}

static
int ssdfs_fsck_add_creation_point(struct ssdfs_fsck_environment *env,
				  u64 create_time)
{
	struct ssdfs_fsck_volume_creation_array *array = NULL;
	struct ssdfs_fsck_volume_creation_point *creation_point = NULL;
	size_t item_size = sizeof(struct ssdfs_fsck_volume_creation_point);
	int new_count;
	size_t bytes_count;
	int i;

	SSDFS_DBG(env->base.show_debug,
		  "Add creation point: create_time %llu\n",
		  create_time);

	array = &env->detection_result.array;

	if (array->count <= 0) {
		SSDFS_ERR("unexpected state of creation array: "
			  "array->count %d\n",
			  array->count);
		return -EINVAL;
	}

	new_count = array->count + 1;
	bytes_count = (size_t)new_count * item_size;

	switch (array->state) {
	case SSDFS_FSCK_CREATION_ARRAY_USE_BUFFER:
		creation_point = &array->creation_points[array->count - 1];

		if (creation_point->volume_creation_timestamp >= U64_MAX) {
			ssdfs_fsck_init_creation_point(creation_point);
			creation_point->volume_creation_timestamp = create_time;
			goto finish_add_creation_point;
		}

		array->creation_points = calloc(new_count, item_size);
		if (!array->creation_points) {
			SSDFS_ERR("cannot allocate memory: "
				  "new_count %d, item size %zu, err: %s\n",
				  new_count, item_size,
				  strerror(errno));
			return errno;
		}

		creation_point = &array->creation_points[array->count - 1];
		memcpy(creation_point, &array->buf, item_size);

		creation_point = &array->creation_points[array->count];
		ssdfs_fsck_init_creation_point(creation_point);

		array->state = SSDFS_FSCK_CREATION_ARRAY_ALLOCATED;
		break;

	case SSDFS_FSCK_CREATION_ARRAY_ALLOCATED:
		array->creation_points = realloc(array->creation_points,
						 bytes_count);
		if (!array->creation_points) {
			SSDFS_ERR("cannot re-allocate memory: "
				  "new_count %d, item size %zu, err: %s\n",
				  new_count, item_size,
				  strerror(errno));
			return errno;
		}

		creation_point = &array->creation_points[array->count];
		ssdfs_fsck_init_creation_point(creation_point);
		break;

	default:
		SSDFS_ERR("unexpected state of creation array: "
			  "state %#x\n", array->state);
		return -ERANGE;
	}

	for (i = 0; i < array->count; i++) {
		creation_point = &array->creation_points[i];

		if (create_time < creation_point->volume_creation_timestamp)
			break;
	}

	if (i < array->count) {
		memmove(&array->creation_points[i + 1],
			&array->creation_points[i],
			(array->count - i) * item_size);
	}

	creation_point = &array->creation_points[i];
	ssdfs_fsck_init_creation_point(creation_point);
	creation_point->volume_creation_timestamp = create_time;
	array->count++;

finish_add_creation_point:
	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return 0;
}

static
int ssdfs_fsck_process_metadata_map_item(struct ssdfs_fsck_environment *env,
					 struct ssdfs_metadata_peb_item *item)
{
	struct ssdfs_fsck_volume_creation_point *creation_point = NULL;
	struct ssdfs_metadata_map *metadata_map;
	size_t item_size = sizeof(struct ssdfs_metadata_peb_item);
	u64 create_time;
	int index;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "Process metadata map item: "
		  "seg_id %llu, leb_id %llu, peb_id %llu, "
		  "volume_creation_timestamp %llu\n",
		  item->seg_id, item->leb_id,
		  item->peb_id, item->volume_creation_timestamp);

	create_time = item->volume_creation_timestamp;
	creation_point = ssdfs_fsck_find_creation_point(env, create_time);
	if (!creation_point) {
		err = ssdfs_fsck_add_creation_point(env, create_time);
		if (err) {
			SSDFS_ERR("fail to add creation point: "
				  "create_time %llu, err %d\n",
				  create_time, err);
			return err;
		}

		creation_point = ssdfs_fsck_find_creation_point(env, create_time);
		if (!creation_point) {
			SSDFS_ERR("fail to find creation point: "
				  "create_time %llu\n",
				  create_time);
			return -ENOENT;
		}
	}

	switch (item->type) {
	case SSDFS_MAPTBL_INIT_SNAP_PEB_TYPE:
		index = SSDFS_INITIAL_SNAPSHOT_SEG_TYPE;
		break;

	case SSDFS_MAPTBL_SBSEG_PEB_TYPE:
		index = SSDFS_SB_SEG_TYPE;
		break;

	case SSDFS_MAPTBL_SEGBMAP_PEB_TYPE:
		index = SSDFS_SEGBMAP_SEG_TYPE;
		break;

	case SSDFS_MAPTBL_MAPTBL_PEB_TYPE:
		index = SSDFS_MAPTBL_SEG_TYPE;
		break;

	case SSDFS_MAPTBL_LNODE_PEB_TYPE:
		index = SSDFS_LEAF_NODE_SEG_TYPE;
		break;

	case SSDFS_MAPTBL_HNODE_PEB_TYPE:
		index = SSDFS_HYBRID_NODE_SEG_TYPE;
		break;

	case SSDFS_MAPTBL_IDXNODE_PEB_TYPE:
		index = SSDFS_INDEX_NODE_SEG_TYPE;
		break;

	default:
		SSDFS_ERR("unexpected PEB type %#x\n",
			  item->type);
		return -EINVAL;
	}

	metadata_map = &creation_point->metadata_map[index];

	if (is_ssdfs_metadata_map_exhausted(metadata_map)) {
		err = ssdfs_fsck_resize_metadata_map(metadata_map);
		if (err) {
			SSDFS_ERR("fail to resize metadata map: "
				  "err %d\n", err);
			return err;
		}
	}

	memcpy(&metadata_map->array[metadata_map->count], item, item_size);
	metadata_map->count++;

	creation_point->found_metadata |= SSDFS_FSCK_METADATA_PEB_MAP_PREPARED;

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return 0;
}

static
int ssdfs_fsck_process_thread_metadata_map(struct ssdfs_fsck_environment *env,
					   int thread_index)
{
	struct ssdfs_thread_state *state;
	struct ssdfs_metadata_map *metadata_map;
	struct ssdfs_metadata_peb_item *item;
	int i;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "Process metadata map: thread_index %d\n",
		  thread_index);

	state = &env->threads.jobs[thread_index];
	metadata_map = &state->metadata_map;

	for (i = 0; i < metadata_map->count; i++) {
		item = &metadata_map->array[i];

		err = ssdfs_fsck_process_metadata_map_item(env, item);
		if (err) {
			SSDFS_ERR("fail to process metadata map's item: "
				  "thread_index %d, item_index %d, err %d\n",
				  thread_index, i, err);
			return err;
		}
	}

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return 0;
}

static
int execute_whole_volume_search(struct ssdfs_fsck_environment *env)
{
	u64 pebs_count;
	u64 pebs_per_thread;
	int i;
	int err;
	int res = SSDFS_FSCK_SEARCH_RESULT_SUCCESS;

	SSDFS_DBG(env->base.show_debug,
		  "Execute whole volume search\n");

	pebs_count = env->base.fs_size / env->base.erase_size;
	pebs_per_thread = (pebs_count + env->threads.capacity - 1);
	pebs_per_thread /= env->threads.capacity;

	env->threads.jobs = calloc(env->threads.capacity,
				   sizeof(struct ssdfs_thread_state));
	if (!env->threads.jobs) {
		SSDFS_ERR("fail to allocate threads pool: %s\n",
			  strerror(errno));
		return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
	}

	for (i = 0; i < env->threads.capacity; i++) {
		ssdfs_init_thread_state(&env->threads.jobs[i],
					i,
					&env->base,
					pebs_per_thread,
					pebs_count,
					env->base.erase_size);

		err = pthread_create(&env->threads.jobs[i].thread, NULL,
				     ssdfs_fsck_process_peb_range,
				     (void *)&env->threads.jobs[i]);
		if (err) {
			res = SSDFS_FSCK_SEARCH_RESULT_FAILURE;
			SSDFS_ERR("fail to create thread %d: %s\n",
				  i, strerror(errno));
			for (i--; i >= 0; i--) {
				pthread_join(env->threads.jobs[i].thread, NULL);
				env->threads.requested_jobs--;
			}
			goto free_threads_pool;
		}

		env->threads.requested_jobs++;
	}

	ssdfs_wait_threads_activity_ending(env);

	for (i = 0; i < env->threads.capacity; i++) {
		err = ssdfs_fsck_process_thread_metadata_map(env, i);
		if (err) {
			res = SSDFS_FSCK_SEARCH_RESULT_FAILURE;
			SSDFS_ERR("fail to process metadata map: "
				  "thread %d, err %d\n",
				  i, err);
			goto free_metadata_maps;
		}
	}

free_metadata_maps:
	for (i = 0; i < env->threads.capacity; i++) {
		struct ssdfs_thread_state *state;
		struct ssdfs_metadata_map *metadata_map;

		state = &env->threads.jobs[i];
		metadata_map = &state->metadata_map;
		__ssdfs_fsck_destroy_metadata_map(metadata_map);
	}

free_threads_pool:
	free(env->threads.jobs);
	env->threads.jobs = NULL;

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return res;
}

int is_device_contains_ssdfs_volume(struct ssdfs_fsck_environment *env)
{
	struct ssdfs_fsck_detection_result *detection_result;
	union ssdfs_metadata_header *buf;
	struct ssdfs_fsck_volume_creation_point *creation_point;
	int index;
	int i;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "Check presence SSDFS file system on the volume\n");

	detection_result = &env->detection_result;
	buf = &detection_result->found_valid_peb;
	env->detection_result.state = SSDFS_FSCK_UNKNOWN_DETECTION_RESULT;

	err = ssdfs_find_any_valid_peb(&env->base, &buf->seg_hdr);
	if (err == -ENODATA) {
		SSDFS_DBG(env->base.show_debug,
			  "unable to find any valid PEB\n");
		env->detection_result.state = SSDFS_FSCK_NO_FILE_SYSTEM_DETECTED;
		goto finish_detection;
	} else if (err) {
		SSDFS_ERR("fail to find any valid PEB\n");
		goto detection_failure;
	}

	env->base.erase_size = 1 << buf->seg_hdr.volume_hdr.log_erasesize;
	env->base.page_size = 1 << buf->seg_hdr.volume_hdr.log_pagesize;

	for (i = 0; i < SSDFS_FSCK_SEARCH_FUNCTION_MAX; i++) {
		switch (detect_actions[i](env)) {
		case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
			/* continue logic */
			break;

		case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
			goto complete_volume_search;

		default:
			SSDFS_ERR("fail to detect metadata structure: "
				  "index %d\n", i);
			goto detection_failure;
		}
	}

	if (env->force_checking) {
complete_volume_search:
		switch (execute_whole_volume_search(env)) {
		case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
		case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
			/* continue logic */
			break;

		default:
			SSDFS_ERR("fail to detect metadata structures\n");
			goto detection_failure;
		}
	}

	BUG_ON(env->detection_result.array.count <= 0);

	index = env->detection_result.array.count - 1;
	creation_point = &env->detection_result.array.creation_points[index];

	if (creation_point->found_metadata ==
				SSDFS_FSCK_NOTHING_FOUND_MASK) {
		env->detection_result.state = SSDFS_FSCK_NO_FILE_SYSTEM_DETECTED;
		SSDFS_DBG(env->base.show_debug,
			  "file system hasn't been detected\n");
	} else if (creation_point->found_metadata ==
				SSDFS_FSCK_ALL_CRITICAL_METADATA_FOUND_MASK) {
		env->detection_result.state = SSDFS_FSCK_DEVICE_HAS_FILE_SYSTEM;
		SSDFS_DBG(env->base.show_debug,
			  "file system has been found\n");
	} else {
		env->detection_result.state = SSDFS_FSCK_DEVICE_HAS_SOME_METADATA;
		SSDFS_DBG(env->base.show_debug,
			  "some metadata have been found\n");
	}

finish_detection:
	SSDFS_DBG(env->base.show_debug,
		  "finished: detection_result.state %#x\n",
		  env->detection_result.state);

	return env->detection_result.state;

detection_failure:
	env->detection_result.state = SSDFS_FSCK_FAILED_DETECT_FILE_SYSTEM;
	return env->detection_result.state;
}
