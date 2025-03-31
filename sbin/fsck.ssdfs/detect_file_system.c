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

#include "fsck.h"

enum {
	SSDFS_FSCK_SEARCH_RESULT_UNKNOWN,
	SSDFS_FSCK_SEARCH_RESULT_SUCCESS,
	SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND,
	SSDFS_FSCK_SEARCH_RESULT_FAILURE,
	SSDFS_FSCK_SEARCH_RESULT_STATE_MAX
};

static inline
void ssdfs_fsck_init_creation_array(struct ssdfs_fsck_environment *env)
{
	struct ssdfs_fsck_volume_creation_array *array;

	SSDFS_DBG(env->base.show_debug,
		  "initialize creation array\n");

	array = &env->detection_result.array;
	memset(&array->buf, 0, sizeof(array->buf));
	array->creation_points = &array->buf;
	array->count = 1;
	array->state = SSDFS_FSCK_CREATION_ARRAY_USE_BUFFER;
}

static inline
void ssdfs_fsck_destroy_creation_array(struct ssdfs_fsck_environment *env)
{
/* TODO: implement */
	SSDFS_DBG(env->base.show_debug,
		  "destroy creation array\n");
}

typedef int (*detect_fn)(struct ssdfs_fsck_environment *env);

static
int is_base_snapshot_segment_found(struct ssdfs_fsck_environment *env)
{
	SSDFS_DBG(env->base.show_debug,
		  "Try to find base snapshot segment\n");

/* TODO: implement */

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
}

static
int is_superblock_segments_found(struct ssdfs_fsck_environment *env)
{
	SSDFS_DBG(env->base.show_debug,
		  "Try to find superblock segments\n");

/* TODO: implement */

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
}

static
int is_segment_bitmap_found(struct ssdfs_fsck_environment *env)
{
	SSDFS_DBG(env->base.show_debug,
		  "Try to find segment bitmap\n");

/* TODO: implement */

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
}

static
int is_mapping_table_found(struct ssdfs_fsck_environment *env)
{
	SSDFS_DBG(env->base.show_debug,
		  "Try to find mapping table\n");

/* TODO: implement */

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
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

static
int execute_complete_volume_search(struct ssdfs_fsck_environment *env)
{
	SSDFS_DBG(env->base.show_debug,
		  "Execute complete volume search\n");

/* TODO: implement */

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	return SSDFS_FSCK_SEARCH_RESULT_FAILURE;
}

int is_device_contains_ssdfs_volume(struct ssdfs_fsck_environment *env)
{
	struct ssdfs_fsck_volume_creation_point *creation_point;
	int i;

	SSDFS_DBG(env->base.show_debug,
		  "Check presence SSDFS file system on the volume\n");

	env->detection_result.state = SSDFS_FSCK_UNKNOWN_DETECTION_RESULT;
	ssdfs_fsck_init_creation_array(env);

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
		switch (execute_complete_volume_search(env)) {
		case SSDFS_FSCK_SEARCH_RESULT_SUCCESS:
		case SSDFS_FSCK_SEARCH_RESULT_NOT_FOUND:
			/* continue logic */
			break;

		default:
			SSDFS_ERR("fail to detect the segment bitmap\n");
			goto detection_failure;
		}
	}

	/* TODO: process this case */
	BUG_ON(env->detection_result.array.count > 1);

	creation_point = &env->detection_result.array.creation_points[0];

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

	ssdfs_fsck_destroy_creation_array(env);

	SSDFS_DBG(env->base.show_debug,
		  "finished: detection_result.state %#x\n",
		  env->detection_result.state);

	return env->detection_result.state;

detection_failure:
	ssdfs_fsck_destroy_creation_array(env);

	env->detection_result.state = SSDFS_FSCK_FAILED_DETECT_FILE_SYSTEM;
	return env->detection_result.state;
}
