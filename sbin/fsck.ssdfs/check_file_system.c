/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/fsck.ssdfs/check_file_system.c - check file system functionality.
 *
 * Copyright (c) 2025-2026 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#include "fsck.h"
#include "check_file_system.h"

void ssdfs_fsck_init_check_result(struct ssdfs_fsck_environment *env)
{
/* TODO: implement */
	SSDFS_DBG(env->base.show_debug,
		  "init check result\n");

	env->check_result.corruption.creation_point = NULL;
	env->check_result.corruption.mask = SSDFS_FSCK_NOTHING_CORRUPTED_MASK;
}

void ssdfs_fsck_destroy_check_result(struct ssdfs_fsck_environment *env)
{
/* TODO: implement */
	SSDFS_DBG(env->base.show_debug,
		  "destroy check result\n");

	struct ssdfs_fsck_corruption_details *details =
					&env->check_result.corruption;

	details->creation_point = NULL;

	ssdfs_fsck_destroy_metadata_items_array(details->base_snapshot_seg.metadata);
	ssdfs_fsck_destroy_metadata_items_array(details->superblock_seg.metadata);
	ssdfs_fsck_destroy_metadata_items_array(details->mapping_table.metadata);
	ssdfs_fsck_destroy_metadata_items_array(details->segment_bitmap.metadata);
	ssdfs_fsck_destroy_metadata_items_array(details->inodes_btree.metadata);
	ssdfs_fsck_destroy_metadata_items_array(details->snapshots_btree.metadata);
	ssdfs_fsck_destroy_metadata_items_array(details->invalid_extents.metadata);
	ssdfs_fsck_destroy_metadata_items_array(details->shared_dictionary.metadata);
}

/*
 * ssdfs_fsck_init_metadata_items() - allocate items array
 * @items: metadata items array
 * @capacity: number of items in the array
 *
 * The method allocates @capacity item states and resets
 * the counters of the array. Previous content of the array
 * is freed.
 *
 * RETURN:
 * [success] - 0.
 * [failure] - negative error code.
 */
int ssdfs_fsck_init_metadata_items(struct ssdfs_fsck_metadata_items_array *items,
				   int capacity)
{
	if (!items || capacity <= 0)
		return -EINVAL;

	ssdfs_fsck_destroy_metadata_items(items);

	items->states = calloc(capacity,
				sizeof(struct ssdfs_fsck_metadata_item_state));
	if (!items->states) {
		SSDFS_ERR("fail to allocate items array: %s\n",
			  strerror(errno));
		return -ENOMEM;
	}

	items->capacity = capacity;

	return 0;
}

/*
 * ssdfs_fsck_destroy_metadata_items() - free items array
 * @items: metadata items array
 */
void ssdfs_fsck_destroy_metadata_items(struct ssdfs_fsck_metadata_items_array *items)
{
	if (!items)
		return;

	free(items->states);
	items->states = NULL;
	items->capacity = 0;
	items->count = 0;
	items->checked = 0;
	items->corrupted = 0;
}

/*
 * ssdfs_fsck_destroy_metadata_items_array() - free items of all on-disk metadata
 * @metadata: on-disk metadata items [SSDFS_FSCK_METADATA_ITEMS_MAX]
 */
void ssdfs_fsck_destroy_metadata_items_array(struct ssdfs_fsck_metadata_items_array *metadata)
{
	int i;

	for (i = 0; i < SSDFS_FSCK_METADATA_ITEMS_MAX; i++)
		ssdfs_fsck_destroy_metadata_items(&metadata[i]);
}

/*
 * ssdfs_fsck_metadata_checked_items() - number of checked items
 * @metadata: on-disk metadata items [SSDFS_FSCK_METADATA_ITEMS_MAX]
 */
int ssdfs_fsck_metadata_checked_items(struct ssdfs_fsck_metadata_items_array *metadata)
{
	int checked = 0;
	int i;

	for (i = 0; i < SSDFS_FSCK_METADATA_ITEMS_MAX; i++)
		checked += metadata[i].checked;

	return checked;
}

/*
 * ssdfs_fsck_metadata_corrupted_items() - number of corrupted items
 * @metadata: on-disk metadata items [SSDFS_FSCK_METADATA_ITEMS_MAX]
 */
int ssdfs_fsck_metadata_corrupted_items(struct ssdfs_fsck_metadata_items_array *metadata)
{
	int corrupted = 0;
	int i;

	for (i = 0; i < SSDFS_FSCK_METADATA_ITEMS_MAX; i++)
		corrupted += metadata[i].corrupted;

	return corrupted;
}

typedef void * (*check_fn)(void *arg);

static inline
int is_metadata_map_empty(struct ssdfs_metadata_map *map)
{
	return map->count <= 0;
}

static
int ssdfs_fsck_find_metadata_peb_item_last_log(struct ssdfs_thread_state *state,
						struct ssdfs_metadata_peb_item *pi,
						struct ssdfs_fsck_found_log *log)
{
	struct ssdfs_fsck_environment *env =
		(struct ssdfs_fsck_environment *)state->private_data;
	struct ssdfs_segment_header *hdr = NULL;
	struct ssdfs_fsck_found_log tmp_log;
	u32 peb_size = state->base.erase_size;
	u32 page_size = U32_MAX;
	u32 pages_per_peb = U32_MAX;
	u16 full_log_pages = U16_MAX;
	int err = 0;
	int res = SSDFS_FSCK_CHECK_RESULT_FAILURE;

	SSDFS_DBG(state->base.show_debug,
		  "find last log: seg_id %llu, leb_id %llu, peb_id %llu\n",
		  pi->seg_id, pi->leb_id, pi->peb_id);

	memset(&tmp_log, 0, sizeof(struct ssdfs_fsck_found_log));

	tmp_log.peb_id = pi->peb_id;
	tmp_log.start_page = 0;

	err = ssdfs_read_segment_header(&state->base,
					pi->peb_id,
					state->base.erase_size,
					0,
					sizeof(union ssdfs_metadata_header),
					&tmp_log.header.seg_hdr);
	if (err) {
		res = SSDFS_FSCK_CHECK_RESULT_FAILURE;
		SSDFS_ERR("fail to read segment header: "
			  "peb_id %llu, err %d\n",
			  pi->peb_id, err);
		goto finish_find_last_log;
	}

	if (is_ssdfs_segment_header(&tmp_log.header.magic)) {
		hdr = &tmp_log.header.seg_hdr;

		page_size = ssdfs_fsck_log_page_size(&state->base, hdr);
		BUG_ON(page_size == 0);
		pages_per_peb = peb_size / page_size;
		BUG_ON(pages_per_peb == 0);
		full_log_pages = le16_to_cpu(hdr->log_pages);
		BUG_ON(full_log_pages == 0 || full_log_pages >= U16_MAX);
	} else {
		res = SSDFS_FSCK_CHECK_RESULT_FAILURE;
		SSDFS_ERR("invalid state of header: "
			  "peb_id %llu, start_page %u\n",
			  pi->peb_id, tmp_log.start_page);
		goto finish_find_last_log;
	}

	BUG_ON(!hdr);

	memcpy(log, &tmp_log, sizeof(struct ssdfs_fsck_found_log));

	res = ssdfs_fsck_find_last_full_log(env, &tmp_log);
	switch (res) {
	case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
		memcpy(log, &tmp_log, sizeof(struct ssdfs_fsck_found_log));
		break;

	case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
		/* do nothing */
		break;

	default:
		res = SSDFS_FSCK_CHECK_RESULT_FAILURE;
		SSDFS_ERR("fail to find last full log: "
			  "seg_id %llu, leb_id %llu, "
			  "peb_id %llu, start_page %u\n",
			  pi->seg_id, pi->leb_id,
			  pi->peb_id, tmp_log.start_page);
		goto finish_find_last_log;
	}

	res = ssdfs_fsck_find_last_partial_log(env, &tmp_log);
	switch (res) {
	case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
		res = SSDFS_FSCK_CHECK_RESULT_SUCCESS;
		memcpy(log, &tmp_log, sizeof(struct ssdfs_fsck_found_log));
		break;

	case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
		res = SSDFS_FSCK_CHECK_RESULT_SUCCESS;
		break;

	default:
		res = SSDFS_FSCK_CHECK_RESULT_FAILURE;
		SSDFS_ERR("fail to find last partial log: "
			  "seg_id %llu, leb_id %llu, "
			  "peb_id %llu, start_page %u\n",
			  pi->seg_id, pi->leb_id,
			  pi->peb_id, tmp_log.start_page);
		goto finish_find_last_log;
	}

finish_find_last_log:
	SSDFS_DBG(state->base.show_debug,
		  "finished\n");

	return res;
}

static
int ssdfs_fsck_check_log_header(struct ssdfs_thread_state *state,
			struct ssdfs_fsck_metadata_items_array *metadata,
			struct ssdfs_fsck_found_log *log)
{
	int res = SSDFS_FSCK_CHECK_RESULT_CORRUPTION;

	SSDFS_DBG(state->base.show_debug,
		  "peb_id %llu, start_page %u\n",
		  log->peb_id, log->start_page);

	if (is_ssdfs_segment_header(&log->header.magic)) {
		res = ssdfs_fsck_check_segment_header(state, metadata, log);
	} else if (is_ssdfs_partial_log_header(&log->header.magic)) {
		res = ssdfs_fsck_check_partial_log_header(state, metadata,
							  log);
	} else {
		res = SSDFS_FSCK_CHECK_RESULT_FAILURE;
		SSDFS_ERR("valid magic hasn't been detected: "
			  "peb_id %llu, start_page %u\n",
			  log->peb_id, log->start_page);
	}

	return res;
}

static
void *is_base_snapshot_segment_corrupted(void *arg)
{
	struct ssdfs_thread_state *state = (struct ssdfs_thread_state *)arg;
	struct ssdfs_fsck_environment *env;
	struct ssdfs_fsck_base_snapshot_segment_corruption *corruption;
	struct ssdfs_fsck_volume_creation_point *creation_point;
	struct ssdfs_metadata_map *map;
	struct ssdfs_metadata_peb_item *peb_item;
	struct ssdfs_fsck_found_log log;
	int index = SSDFS_INITIAL_SNAPSHOT_SEG_TYPE;
	int i;
	int res = SSDFS_FSCK_CHECK_RESULT_CORRUPTION;

	if (!state)
		pthread_exit((void *)1);

	SSDFS_DBG(state->base.show_debug,
		  "Try to find base snapshot segment corruption(s)\n");

	state->err = 0;

	env = (struct ssdfs_fsck_environment *)state->private_data;

	if (!env) {
		state->err = -EINVAL;
		SSDFS_ERR("invalid pointer on fsck environment\n");
		pthread_exit((void *)1);
	}

	SSDFS_FSCK_INFO(state->base.show_info,
			"thread %d, checking base snapshot segment...\n",
			state->id);

	BUG_ON(!env->check_result.corruption.creation_point);
	creation_point = env->check_result.corruption.creation_point;
	map = &creation_point->metadata_map[index];

	corruption = &env->check_result.corruption.base_snapshot_seg;
	corruption->state = SSDFS_FSCK_METADATA_STRUCTURE_UNKNOWN_STATE;
	ssdfs_fsck_destroy_metadata_items_array(corruption->metadata);

	if (creation_point->found_metadata & SSDFS_FSCK_BASE_SNAPSHOT_SEG_FOUND) {
		memcpy(&log, &creation_point->base_snapshot_seg.log,
			sizeof(struct ssdfs_fsck_found_log));
	} else if (!is_metadata_map_empty(map)) {
		if (map->count > map->capacity) {
			state->err = -EFAULT;
			corruption->state =
				SSDFS_FSCK_METADATA_STRUCTURE_CHECK_FAILED;
			SSDFS_ERR("corrupted metadata map: "
				  "map->count %d, map->capacity %d\n",
				  map->count, map->capacity);
			goto finish_base_snapshot_seg_check;
		}

		if (!map->array) {
			state->err = -EFAULT;
			corruption->state =
				SSDFS_FSCK_METADATA_STRUCTURE_CHECK_FAILED;
			SSDFS_ERR("corrupted metadata map: "
				  "array is absent\n");
			goto finish_base_snapshot_seg_check;
		}

		i = 0;
		for (; i < map->count; i++) {
			peb_item = &map->array[i];

			if (peb_item->type != SSDFS_MAPTBL_INIT_SNAP_PEB_TYPE) {
				SSDFS_ERR("invalid PEB type: "
					  "type %#x, index %d\n",
					  peb_item->type, i);
				continue;
			}

			if (peb_item->seg_id != 0 ||
			    peb_item->leb_id != 0 ||
			    peb_item->peb_id != 0) {
				SSDFS_ERR("invalid PEB ID: "
					  "seg_id %llu, leb_id %llu, "
					  "peb_id %llu\n",
					  peb_item->seg_id,
					  peb_item->leb_id,
					  peb_item->peb_id);
				continue;
			}

			/* we can check the PEB item */
			break;
		}

		if (i >= map->count) {
			corruption->state = SSDFS_FSCK_METADATA_STRUCTURE_ABSENT;
			env->check_result.corruption.mask |=
				SSDFS_FSCK_BASE_SNAPSHOT_SEGMENT_CORRUPTED;
			goto finish_base_snapshot_seg_check;
		}

		peb_item = &map->array[i];

		res = ssdfs_fsck_find_metadata_peb_item_last_log(state,
								 peb_item,
								 &log);
		switch (res) {
		case SSDFS_FSCK_CHECK_RESULT_SUCCESS:
			/* continue logic */
			break;

		case SSDFS_FSCK_CHECK_RESULT_CORRUPTION:
			corruption->state =
				SSDFS_FSCK_METADATA_STRUCTURE_CORRUPTED;
			env->check_result.corruption.mask |=
				SSDFS_FSCK_BASE_SNAPSHOT_SEGMENT_CORRUPTED;
			goto finish_base_snapshot_seg_check;

		case SSDFS_FSCK_CHECK_RESULT_FAILURE:
			state->err = -EFAULT;
			corruption->state =
				SSDFS_FSCK_METADATA_STRUCTURE_CHECK_FAILED;
			goto finish_base_snapshot_seg_check;

		default:
			state->err = -EFAULT;
			SSDFS_ERR("unexpected return result %#x\n", res);
			corruption->state =
				SSDFS_FSCK_METADATA_STRUCTURE_CHECK_FAILED;
			goto finish_base_snapshot_seg_check;
		}
	} else {
		corruption->state = SSDFS_FSCK_METADATA_STRUCTURE_ABSENT;
		env->check_result.corruption.mask |=
				SSDFS_FSCK_BASE_SNAPSHOT_SEGMENT_CORRUPTED;
		goto finish_base_snapshot_seg_check;
	}

	res = ssdfs_fsck_check_log_header(state, corruption->metadata, &log);
	switch (res) {
	case SSDFS_FSCK_CHECK_RESULT_SUCCESS:
		/* continue logic */
		break;

	case SSDFS_FSCK_CHECK_RESULT_CORRUPTION:
		corruption->state = SSDFS_FSCK_METADATA_STRUCTURE_CORRUPTED;
		env->check_result.corruption.mask |=
				SSDFS_FSCK_BASE_SNAPSHOT_SEGMENT_CORRUPTED;
		break;

	case SSDFS_FSCK_CHECK_RESULT_FAILURE:
		state->err = -EFAULT;
		corruption->state = SSDFS_FSCK_METADATA_STRUCTURE_CHECK_FAILED;
		goto finish_base_snapshot_seg_check;

	default:
		state->err = -EFAULT;
		SSDFS_ERR("unexpected return result %#x\n", res);
		corruption->state = SSDFS_FSCK_METADATA_STRUCTURE_CHECK_FAILED;
		goto finish_base_snapshot_seg_check;
	}

	res = ssdfs_fsck_check_log_footer(state, corruption->metadata, &log);
	switch (res) {
	case SSDFS_FSCK_CHECK_RESULT_SUCCESS:
		/* continue logic */
		break;

	case SSDFS_FSCK_CHECK_RESULT_CORRUPTION:
		corruption->state = SSDFS_FSCK_METADATA_STRUCTURE_CORRUPTED;
		env->check_result.corruption.mask |=
				SSDFS_FSCK_BASE_SNAPSHOT_SEGMENT_CORRUPTED;
		break;

	case SSDFS_FSCK_CHECK_RESULT_FAILURE:
		state->err = -EFAULT;
		corruption->state = SSDFS_FSCK_METADATA_STRUCTURE_CHECK_FAILED;
		goto finish_base_snapshot_seg_check;

	default:
		state->err = -EFAULT;
		SSDFS_ERR("unexpected return result %#x\n", res);
		corruption->state = SSDFS_FSCK_METADATA_STRUCTURE_CHECK_FAILED;
		goto finish_base_snapshot_seg_check;
	}

	if (corruption->state == SSDFS_FSCK_METADATA_STRUCTURE_UNKNOWN_STATE) {
		if (ssdfs_fsck_metadata_corrupted_items(corruption->metadata) > 0) {
			corruption->state =
				SSDFS_FSCK_METADATA_STRUCTURE_CORRUPTED;
			env->check_result.corruption.mask |=
				SSDFS_FSCK_BASE_SNAPSHOT_SEGMENT_CORRUPTED;
		} else
			corruption->state = SSDFS_FSCK_METADATA_STRUCTURE_OK;
	}

finish_base_snapshot_seg_check:
	SSDFS_DBG(env->base.show_debug,
		  "finished: state %#x, checked %d, corrupted %d\n",
		  corruption->state,
		  ssdfs_fsck_metadata_checked_items(corruption->metadata),
		  ssdfs_fsck_metadata_corrupted_items(corruption->metadata));

	SSDFS_FSCK_INFO(state->base.show_info,
			"FINISHED: thread %d\n",
			state->id);

	pthread_exit((void *)0);
}

/*
 * TODO: the check of metadata structure is not implemented yet.
 *       The thread doesn't report any corruption.
 */
static
void *ssdfs_fsck_check_not_implemented(void *arg, const char *name)
{
	struct ssdfs_thread_state *state = (struct ssdfs_thread_state *)arg;

	if (!state)
		pthread_exit((void *)1);

	state->err = 0;

	SSDFS_DBG(state->base.show_debug,
		  "check of %s is not implemented yet\n", name);

	SSDFS_FSCK_INFO(state->base.show_info,
			"thread %d, check of %s is not implemented yet\n",
			state->id, name);

	pthread_exit((void *)0);
}

static
void *is_superblock_segment_corrupted(void *arg)
{
	return ssdfs_fsck_check_not_implemented(arg, "superblock segment");
}

static
void *is_segment_bitmap_corrupted(void *arg)
{
	return ssdfs_fsck_check_not_implemented(arg, "segment bitmap");
}

static
void *is_mapping_table_corrupted(void *arg)
{
	return ssdfs_fsck_check_not_implemented(arg, "mapping table");
}

static
void *is_inodes_btree_corrupted(void *arg)
{
	return ssdfs_fsck_check_not_implemented(arg, "inodes b-tree");
}

static
void *is_snapshots_btree_corrupted(void *arg)
{
	return ssdfs_fsck_check_not_implemented(arg, "snapshots b-tree");
}

static
void *is_invalidated_extents_btree_corrupted(void *arg)
{
	return ssdfs_fsck_check_not_implemented(arg, "invalidated extents b-tree");
}

static
void *is_shared_dictionary_btree_corrupted(void *arg)
{
	return ssdfs_fsck_check_not_implemented(arg, "shared dictionary b-tree");
}

enum {
	SSDFS_FSCK_BASE_SNAPSHOT_SEG_CHECK_FUNCTION,
	SSDFS_FSCK_SUPERBLOCK_SEG_CHECK_FUNCTION,
	SSDFS_FSCK_SEGMENT_BITMAP_CHECK_FUNCTION,
	SSDFS_FSCK_MAPPING_TABLE_CHECK_FUNCTION,
	SSDFS_FSCK_INODES_BTREE_CHECK_FUNCTION,
	SSDFS_FSCK_SNAPSHOTS_BTREE_CHECK_FUNCTION,
	SSDFS_FSCK_INVALID_EXTENTS_BTREE_CHECK_FUNCTION,
	SSDFS_FSCK_SHARED_DICTIONARY_BTREE_CHECK_FUNCTION,
	SSDFS_FSCK_CHECK_FUNCTION_MAX
};

static check_fn check_actions[SSDFS_FSCK_CHECK_FUNCTION_MAX] = {
/* 00 */	is_base_snapshot_segment_corrupted,
/* 01 */	is_superblock_segment_corrupted,
/* 02 */	is_segment_bitmap_corrupted,
/* 03 */	is_mapping_table_corrupted,
/* 04 */	is_inodes_btree_corrupted,
/* 05 */	is_snapshots_btree_corrupted,
/* 06 */	is_invalidated_extents_btree_corrupted,
/* 07 */	is_shared_dictionary_btree_corrupted,
};

int is_ssdfs_volume_corrupted(struct ssdfs_fsck_environment *env)
{
	u64 pebs_count;
	int i;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "Detect any SSDFS file system corruption(s)\n");

	env->check_result.state = SSDFS_FSCK_VOLUME_UNKNOWN_CHECK_RESULT;

	switch (env->detection_result.state) {
	case SSDFS_FSCK_DEVICE_HAS_FILE_SYSTEM:
		/* continue logic */
		break;

	case SSDFS_FSCK_DEVICE_HAS_SOME_METADATA:
		SSDFS_DBG(env->base.show_debug,
			  "Some metadata is absent\n");
		env->check_result.state = SSDFS_FSCK_VOLUME_HEAVILY_CORRUPTED;
		/* make the check of found metadata */
		break;

	case SSDFS_FSCK_NO_FILE_SYSTEM_DETECTED:
		SSDFS_DBG(env->base.show_debug,
			  "No file system has been detected\n");
		env->check_result.state = SSDFS_FSCK_VOLUME_COMPLETELY_DESTROYED;
		goto finish_check;

	default:
		SSDFS_ERR("unexpected detection phase result %#x\n",
			  env->detection_result.state);
		goto check_failure;
	}

/* TODO: process the selection of creation point */

	if (env->detection_result.array.count == 0) {
		SSDFS_ERR("detection result array is empty\n");
		goto check_failure;
	}

	i = env->detection_result.array.count - 1;
	env->check_result.corruption.creation_point =
				ssdfs_fsck_get_creation_point(env, i);
	if (!env->check_result.corruption.creation_point) {
		SSDFS_ERR("fail to get a creation point: "
			  "index %d\n", i);
		goto check_failure;
	}

	env->threads.jobs = calloc(SSDFS_FSCK_CHECK_FUNCTION_MAX,
				   sizeof(struct ssdfs_thread_state));
	if (!env->threads.jobs) {
		SSDFS_ERR("fail to allocate threads pool: %s\n",
			  strerror(errno));
		goto check_failure;
	}

	pebs_count = env->base.fs_size / env->base.erase_size;

	/* detection phase could leave the counter of its own threads */
	env->threads.requested_jobs = 0;

	for (i = 0; i < SSDFS_FSCK_CHECK_FUNCTION_MAX; i++) {
		ssdfs_init_thread_state(&env->threads.jobs[i],
					i,
					&env->base,
					env,
					pebs_count, pebs_count,
					env->base.erase_size);

		err = pthread_create(&env->threads.jobs[i].thread, NULL,
				     check_actions[i],
				     (void *)&env->threads.jobs[i]);
		if (err) {
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
	env->threads.requested_jobs = 0;

	for (i = 0; i < SSDFS_FSCK_CHECK_FUNCTION_MAX; i++) {
		err = env->threads.jobs[i].err;

		if (err) {
			SSDFS_ERR("fail to check metadata structure: "
				  "index %d\n", i);
			goto free_threads_pool;
		}
	}

	free(env->threads.jobs);
	env->threads.jobs = NULL;

	if (env->check_result.state == SSDFS_FSCK_VOLUME_UNKNOWN_CHECK_RESULT) {
		/* TODO: distinguish slight and heavy corruption */
		if (env->check_result.corruption.mask ==
					SSDFS_FSCK_NOTHING_CORRUPTED_MASK) {
			env->check_result.state = SSDFS_FSCK_VOLUME_HEALTHY;
		} else {
			env->check_result.state =
				SSDFS_FSCK_VOLUME_SLIGHTLY_CORRUPTED;
		}
	}

finish_check:
	switch (env->check_result.state) {
	case SSDFS_FSCK_VOLUME_COMPLETELY_DESTROYED:
	case SSDFS_FSCK_VOLUME_HEAVILY_CORRUPTED:
	case SSDFS_FSCK_VOLUME_SLIGHTLY_CORRUPTED:
	case SSDFS_FSCK_VOLUME_UNCLEAN_UMOUNT:
		ssdfs_fsck_explain_volume_corruption(env);
		break;

	case SSDFS_FSCK_VOLUME_HEALTHY:
		/* continue logic */
		break;

	default:
		goto check_failure;
	}

	SSDFS_DBG(env->base.show_debug,
		  "finished: check_result.state %#x\n",
		  env->check_result.state);

	return env->check_result.state;

free_threads_pool:
	free(env->threads.jobs);
	env->threads.jobs = NULL;

check_failure:
	SSDFS_ERR("SSDFS volume check failed\n");
	env->check_result.state = SSDFS_FSCK_VOLUME_CHECK_FAILED;
	return env->check_result.state;
}
