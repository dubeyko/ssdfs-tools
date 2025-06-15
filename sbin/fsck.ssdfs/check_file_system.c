/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/fsck.ssdfs/check_file_system.c - check file system functionality.
 *
 * Copyright (c) 2025 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#include "fsck.h"

enum {
	SSDFS_FSCK_CHECK_RESULT_UNKNOWN,
	SSDFS_FSCK_CHECK_RESULT_SUCCESS,
	SSDFS_FSCK_CHECK_RESULT_CORRUPTION,
	SSDFS_FSCK_CHECK_RESULT_FAILURE,
	SSDFS_FSCK_CHECK_RESULT_STATE_MAX
};

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

	env->check_result.corruption.creation_point = NULL;
}

typedef void * (*check_fn)(void *arg);

static inline
int is_metadata_map_empty(struct ssdfs_metadata_map *map)
{
	return map->count <= 0;
}



		res = ssdfs_fsck_find_metadata_peb_item_last_log(peb_item, &log);




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
			state->id, state->peb.id, 0);

	BUG_ON(!env->check_result.corruption.creation_point);
	creation_point = env->check_result.corruption.creation_point;
	map = &creation_point->metadata_map[index];

	corruption = &env->check_result.corruption.base_snapshot_seg;

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

		res = ssdfs_fsck_find_metadata_peb_item_last_log(peb_item, &log);
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









/* TODO: implement */

finish_base_snapshot_seg_check:
	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	env->check_result.corruption.mask |=
			SSDFS_FSCK_BASE_SNAPSHOT_SEGMENT_CORRUPTED;

	SSDFS_FSCK_INFO(state->base.show_info,
			"FINISHED: thread %d\n",
			state->id);

	pthread_exit((void *)0);
}

static
void *is_superblock_segment_corrupted(void *arg)
{
	SSDFS_DBG(env->base.show_debug,
		  "Try to find superblock segment corruption(s)\n");

/* TODO: implement */

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	env->check_result.corruption.mask |=
			SSDFS_FSCK_SUPERBLOCK_SEGMENT_CORRUPTED;
	return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
}

static
void *is_segment_bitmap_corrupted(void *arg)
{
	SSDFS_DBG(env->base.show_debug,
		  "Try to find segment bitmap corruption(s)\n");

/* TODO: implement */
[<64;49;24M
	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	env->check_result.corruption.mask |=
			SSDFS_FSCK_SEGMENT_BITMAP_CORRUPTED;
	return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
}

static
void *is_mapping_table_corrupted(void *arg)
{
	SSDFS_DBG(env->base.show_debug,
		  "Try to find mapping table corruption(s)\n");

/* TODO: implement */

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	env->check_result.corruption.mask |=
			SSDFS_FSCK_MAPPING_TABLE_CORRUPTED;
	return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
}

static
void *is_inodes_btree_corrupted(void *arg)
{
	SSDFS_DBG(env->base.show_debug,
		  "Try to find inodes btree corruption(s)\n");

/* TODO: implement */

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	env->check_result.corruption.mask |=
			SSDFS_FSCK_INODES_BTREE_CORRUPTED;
	return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
}

static
void *is_snapshots_btree_corrupted(void *arg)
{
	SSDFS_DBG(env->base.show_debug,
		  "Try to find snapshots btree corruption(s)\n");

/* TODO: implement */

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	env->check_result.corruption.mask |=
			SSDFS_FSCK_SNAPSHOTS_BTREE_CORRUPTED;
	return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
}

static
void *is_invalid_extents_btree_corrupted(void *arg)
{
	SSDFS_DBG(env->base.show_debug,
		  "Try to find invalid extents btree corruption(s)\n");

/* TODO: implement */

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	env->check_result.corruption.mask |=
			SSDFS_FSCK_INVALID_EXTENTS_BTREE_CORRUPTED;
	return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
}

static
void *is_shared_dictionary_btree_corrupted(void *arg)
{
	SSDFS_DBG(env->base.show_debug,
		  "Try to find shared dictionary btree corruption(s)\n");

/* TODO: implement */

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	env->check_result.corruption.mask |=
			SSDFS_FSCK_SHARED_DICT_BTREE_CORRUPTED;
	return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
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
/* 06 */	is_invalid_extents_btree_corrupted,
/* 07 */	is_shared_dictionary_btree_corrupted,
};

static
void ssdfs_fsck_explain_volume_corruption(struct ssdfs_fsck_environment *env)
{
/* TODO: implement */
	SSDFS_DBG(env->base.show_debug,
		  "explain volume corruption\n");
}

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
