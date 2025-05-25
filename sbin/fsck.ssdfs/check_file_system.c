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
}

void ssdfs_fsck_destroy_check_result(struct ssdfs_fsck_environment *env)
{
/* TODO: implement */
	SSDFS_DBG(env->base.show_debug,
		  "destroy check result\n");
}

typedef int (*check_fn)(struct ssdfs_fsck_environment *env);

static
int is_base_snapshot_segment_corrupted(struct ssdfs_fsck_environment *env)
{
	SSDFS_DBG(env->base.show_debug,
		  "Try to find base snapshot segment corruption(s)\n");

/* TODO: implement */

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	env->check_result.corruption.mask |=
			SSDFS_FSCK_BASE_SNAPSHOT_SEGMENT_CORRUPTED;
	return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
}

static
int is_superblock_segment_corrupted(struct ssdfs_fsck_environment *env)
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
int is_segment_bitmap_corrupted(struct ssdfs_fsck_environment *env)
{
	SSDFS_DBG(env->base.show_debug,
		  "Try to find segment bitmap corruption(s)\n");

/* TODO: implement */

	SSDFS_DBG(env->base.show_debug,
		  "finished\n");

	env->check_result.corruption.mask |=
			SSDFS_FSCK_SEGMENT_BITMAP_CORRUPTED;
	return SSDFS_FSCK_CHECK_RESULT_CORRUPTION;
}

static
int is_mapping_table_corrupted(struct ssdfs_fsck_environment *env)
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
int is_inodes_btree_corrupted(struct ssdfs_fsck_environment *env)
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
int is_snapshots_btree_corrupted(struct ssdfs_fsck_environment *env)
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
int is_invalid_extents_btree_corrupted(struct ssdfs_fsck_environment *env)
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
int is_shared_dictionary_btree_corrupted(struct ssdfs_fsck_environment *env)
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
	int i;

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
		goto finish_check;

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

	for (i = 0; i < SSDFS_FSCK_CHECK_FUNCTION_MAX; i++) {
		switch (check_actions[i](env)) {
		case SSDFS_FSCK_CHECK_RESULT_SUCCESS:
		case SSDFS_FSCK_CHECK_RESULT_CORRUPTION:
			/* continue logic */
			break;

		default:
			SSDFS_ERR("fail to check metadata structure: "
				  "index %d\n", i);
			goto check_failure;
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

check_failure:
	SSDFS_ERR("SSDFS volume check failed\n");
	env->check_result.state = SSDFS_FSCK_VOLUME_CHECK_FAILED;
	return env->check_result.state;
}
