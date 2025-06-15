/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/fsck.ssdfs/fsck.h - declarations of fsck.ssdfs utility.
 *
 * Copyright (c) 2025 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#ifndef _SSDFS_UTILS_FSCK_H
#define _SSDFS_UTILS_FSCK_H

#ifdef fsck_fmt
#undef fsck_fmt
#endif

#include "version.h"

#define fsck_fmt(fmt) "fsck.ssdfs: " SSDFS_UTILS_VERSION ": " fmt

#include <pthread.h>
#include <dirent.h>

#include "ssdfs_tools.h"
#include "detect_file_system.h"
#include "check_file_system.h"

#define SSDFS_FSCK_INFO(show, fmt, ...) \
	do { \
		if (show) { \
			fprintf(stdout, fsck_fmt(fmt), ##__VA_ARGS__); \
		} \
	} while (0)

#define SSDFS_FSCK_DEFAULT_THREADS		(1)

enum {
	SSDFS_FSCK_NONE_USER_ANSWER,
	SSDFS_FSCK_YES_USER_ANSWER,
	SSDFS_FSCK_NO_USER_ANSWER,
	SSDFS_FSCK_UNKNOWN_USER_ANSWER,
};

#define SSDFS_FSCK_SHORT_YES_STRING1		"y"
#define SSDFS_FSCK_SHORT_YES_STRING2		"Y"
#define SSDFS_FSCK_LONG_YES_STRING1		"yes"
#define SSDFS_FSCK_LONG_YES_STRING2		"Yes"
#define SSDFS_FSCK_LONG_YES_STRING3		"YES"

#define SSDFS_FSCK_SHORT_NO_STRING1		"n"
#define SSDFS_FSCK_SHORT_NO_STRING2		"N"
#define SSDFS_FSCK_LONG_NO_STRING1		"no"
#define SSDFS_FSCK_LONG_NO_STRING2		"No"
#define SSDFS_FSCK_LONG_NO_STRING3		"NO"

enum {
	SSDFS_FSCK_DEVICE_HAS_FILE_SYSTEM,
	SSDFS_FSCK_DEVICE_HAS_SOME_METADATA,
	SSDFS_FSCK_NO_FILE_SYSTEM_DETECTED,
	SSDFS_FSCK_FAILED_DETECT_FILE_SYSTEM,
	SSDFS_FSCK_UNKNOWN_DETECTION_RESULT,
};

struct ssdfs_fsck_detection_result {
	int state;
	struct ssdfs_fsck_volume_creation_array array;
	union ssdfs_metadata_header found_valid_peb;
};

enum {
	SSDFS_FSCK_VOLUME_COMPLETELY_DESTROYED,
	SSDFS_FSCK_VOLUME_HEAVILY_CORRUPTED,
	SSDFS_FSCK_VOLUME_SLIGHTLY_CORRUPTED,
	SSDFS_FSCK_VOLUME_UNCLEAN_UMOUNT,
	SSDFS_FSCK_VOLUME_HEALTHY,
	SSDFS_FSCK_VOLUME_CHECK_FAILED,
	SSDFS_FSCK_VOLUME_UNKNOWN_CHECK_RESULT,
};

struct ssdfs_fsck_check_result {
	int state;
	struct ssdfs_fsck_corruption_details corruption;
};

enum {
	SSDFS_FSCK_NO_RECOVERY_NECCESSARY,
	SSDFS_FSCK_UNABLE_RECOVER,
	SSDFS_FSCK_COMPLETE_METADATA_REBUILD,
	SSDFS_FSCK_METADATA_PARTIALLY_LOST,
	SSDFS_FSCK_USER_DATA_PARTIALLY_LOST,
	SSDFS_FSCK_RECOVERY_NAND_DEGRADED,
	SSDFS_FSCK_RECOVERY_DEVICE_MALFUNCTION,
	SSDFS_FSCK_RECOVERY_INTERRUPTED,
	SSDFS_FSCK_RECOVERY_SUCCESS,
	SSDFS_FSCK_RECOVERY_FAILED,
	SSDFS_FSCK_UNKNOWN_RECOVERY_RESULT,
};

struct ssdfs_fsck_mapping_table_recovery {
	int state;
};

struct ssdfs_fsck_segment_bitmap_recovery {
	int state;
};

struct ssdfs_fsck_inodes_btree_recovery {
	int state;
};

struct ssdfs_fsck_shared_dictionary_btree_recovery {
	int state;
};

struct ssdfs_fsck_snapshots_btree_recovery {
	int state;
};

struct ssdfs_fsck_invalid_extents_btree_recovery {
	int state;
};

struct ssdfs_fsck_superblock_segment_recovery {
	int state;
};

struct ssdfs_fsck_base_snapshot_segment_recovery {
	int state;
};

struct ssdfs_fsck_recovery_details {
	struct ssdfs_fsck_mapping_table_recovery mapping_table;
	struct ssdfs_fsck_segment_bitmap_recovery segment_bitmap;
	struct ssdfs_fsck_inodes_btree_recovery inodes_btree;
	struct ssdfs_fsck_shared_dictionary_btree_recovery shared_dictionary;
	struct ssdfs_fsck_snapshots_btree_recovery snapshots_btree;
	struct ssdfs_fsck_invalid_extents_btree_recovery invalid_extents;
	struct ssdfs_fsck_superblock_segment_recovery superblock_seg;
	struct ssdfs_fsck_base_snapshot_segment_recovery base_snapshot_seg;
};

struct ssdfs_fsck_recovery_result {
	int state;
	struct ssdfs_fsck_recovery_details details;
};

/*
 * struct ssdfs_fsck_environment - fsck environment
 * @force_checking: force checking even if filesystem is marked clean
 * @no_change: make no changes to the filesystem
 * @auto_repair: automatic repair
 * @yes_all_questions: assume YES to all questions
 * @be_verbose: be verbose
 * @seg_size: segment size in bytes
 * @base: basic environment
 * @threads: threads environment
 * @detection_result: detection result
 * @check_result: check result
 * @recovery_result: recovery result
 */
struct ssdfs_fsck_environment {
	int force_checking;
	int no_change;
	int auto_repair;
	int yes_all_questions;
	int be_verbose;

	u32 seg_size;

	struct ssdfs_environment base;
	struct ssdfs_threads_environment threads;
	struct ssdfs_fsck_detection_result detection_result;
	struct ssdfs_fsck_check_result check_result;
	struct ssdfs_fsck_recovery_result recovery_result;
};

/* Inline functions */

static inline
int is_ssdfs_fsck_area_valid(struct ssdfs_metadata_descriptor *desc)
{
	u32 area_offset = le32_to_cpu(desc->offset);
	u32 area_size = le32_to_cpu(desc->size);

	if (area_size == 0 || area_size >= U32_MAX)
		return SSDFS_FALSE;

	if (area_offset == 0 || area_offset >= U32_MAX)
		return SSDFS_FALSE;

	return SSDFS_TRUE;
}

static inline
void ssdfs_init_thread_descriptor(struct ssdfs_thread_state *state,
				  int thread_id)
{
	state->id = thread_id;
	state->err = 0;
}

static inline
void ssdfs_init_base_environment(struct ssdfs_thread_state *state,
				 struct ssdfs_environment *base)
{
	memcpy(&state->base, base, sizeof(struct ssdfs_environment));
}

static inline
void ssdfs_init_peb_environment(struct ssdfs_thread_state *state,
				int thread_id, u64 pebs_per_thread,
				u64 pebs_count, u32 erase_size)
{
	state->peb.id = (u64)thread_id * pebs_per_thread;
	state->peb.pebs_count = min_t(u64,
					pebs_count - state->peb.id,
					pebs_per_thread);
	state->peb.peb_size = erase_size;
}

static inline
void ssdfs_init_metadata_map(struct ssdfs_thread_state *state)
{
	state->metadata_map.array = NULL;
	state->metadata_map.capacity = 0;
	state->metadata_map.count = 0;
}

static inline
void ssdfs_init_thread_state(struct ssdfs_thread_state *state,
			     int thread_id,
			     struct ssdfs_environment *base,
			     struct ssdfs_fsck_environment *fsck_env,
			     u64 pebs_per_thread,
			     u64 pebs_count,
			     u32 erase_size)
{
	ssdfs_init_thread_descriptor(state, thread_id);
	ssdfs_init_base_environment(state, base);
	ssdfs_init_peb_environment(state, thread_id, pebs_per_thread,
				   pebs_count, erase_size);
	ssdfs_init_metadata_map(state);

	state->private_data = fsck_env;
}

static inline
void ssdfs_wait_threads_activity_ending(struct ssdfs_fsck_environment *env)
{
	int i;

	SSDFS_DBG(env->base.show_debug,
		  "requested_jobs %u, tasks_capacity %u\n",
		  env->threads.requested_jobs, env->threads.capacity);

	for (i = 0; i < env->threads.requested_jobs; i++) {
		pthread_join(env->threads.jobs[i].thread, NULL);

		if (env->threads.jobs[i].err != 0) {
			SSDFS_ERR("thread %d has failed: err %d\n",
				  i, env->threads.jobs[i].err);
		}
	}
}

/* Application APIs */

/* detect_file_system.c */
int is_device_contains_ssdfs_volume(struct ssdfs_fsck_environment *env);
void ssdfs_fsck_init_detection_result(struct ssdfs_fsck_environment *env);
void ssdfs_fsck_destroy_detection_result(struct ssdfs_fsck_environment *env);
struct ssdfs_fsck_volume_creation_point *
ssdfs_fsck_get_creation_point(struct ssdfs_fsck_environment *env, int index);

/* check_file_system.c */
int is_ssdfs_volume_corrupted(struct ssdfs_fsck_environment *env);
void ssdfs_fsck_init_check_result(struct ssdfs_fsck_environment *env);
void ssdfs_fsck_destroy_check_result(struct ssdfs_fsck_environment *env);

/* recover_file_system.c */
int recover_corrupted_ssdfs_volume(struct ssdfs_fsck_environment *env);
void ssdfs_fsck_init_recovery_result(struct ssdfs_fsck_environment *env);
void ssdfs_fsck_destroy_recovery_result(struct ssdfs_fsck_environment *env);

/* options.c */
void print_usage(void);
void parse_options(int argc, char *argv[],
		   struct ssdfs_fsck_environment *env);

#endif /* _SSDFS_UTILS_FSCK_H */
