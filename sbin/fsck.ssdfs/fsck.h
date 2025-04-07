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

#define SSDFS_FSCK_BASE_SNAPSHOT_SEG_FOUND		(1 << 0)
#define SSDFS_FSCK_SB_SEGS_FOUND			(1 << 1)
#define SSDFS_FSCK_MAPPING_TBL_FOUND			(1 << 2)
#define SSDFS_FSCK_SEGMENT_BITMAP_FOUND			(1 << 3)
#define SSDFS_FSCK_NOTHING_FOUND_MASK			0x0
#define SSDFS_FSCK_ALL_CRITICAL_METADATA_FOUND_MASK	0xF

struct ssdfs_fsck_volume_creation_point {
	struct ssdfs_segment_header seg_hdr;
	union {
		struct ssdfs_signature magic;
		struct ssdfs_log_footer footer;
		struct ssdfs_partial_log_header pl_hdr;
	} log;
	void *maptbl_cache;
	u64 found_metadata;
};

enum {
	SSDFS_FSCK_CREATION_ARRAY_NOT_INITIALIZED,
	SSDFS_FSCK_CREATION_ARRAY_USE_BUFFER,
	SSDFS_FSCK_CREATION_ARRAY_ALLOCATED,
	SSDFS_FSCK_CREATION_ARRAY_STATE_MAX
};

struct ssdfs_fsck_volume_creation_array {
	int state;
	int count;
	struct ssdfs_fsck_volume_creation_point *creation_points;
	struct ssdfs_fsck_volume_creation_point buf;
};

struct ssdfs_fsck_detection_result {
	int state;
	struct ssdfs_fsck_volume_creation_array array;
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

/*
 * Corruption mask flags
 */
#define SSDFS_FSCK_BASE_SNAPSHOT_SEGMENT_CORRUPTED		(1 << 0)
#define SSDFS_FSCK_SUPERBLOCK_SEGMENT_CORRUPTED			(1 << 1)
#define SSDFS_FSCK_MAPPING_TABLE_CORRUPTED			(1 << 2)
#define SSDFS_FSCK_SEGMENT_BITMAP_CORRUPTED			(1 << 3)
#define SSDFS_FSCK_INODES_BTREE_CORRUPTED			(1 << 4)
#define SSDFS_FSCK_SNAPSHOTS_BTREE_CORRUPTED			(1 << 5)
#define SSDFS_FSCK_DENTRIES_BTREE_CORRUPTED			(1 << 6)
#define SSDFS_FSCK_EXTENTS_BTREE_CORRUPTED			(1 << 7)
#define SSDFS_FSCK_SHARED_EXTENTS_BTREE_CORRUPTED		(1 << 8)
#define SSDFS_FSCK_INVALID_EXTENTS_BTREE_CORRUPTED		(1 << 9)
#define SSDFS_FSCK_SHARED_DICT_BTREE_CORRUPTED			(1 << 10)
#define SSDFS_FSCK_XATTR_BTREE_CORRUPTED			(1 << 11)
#define SSDFS_FSCK_SHARED_XATTR_BTREE_CORRUPTED			(1 << 12)

struct ssdfs_fsck_base_snapshot_segment_corruption {
	int state;
};

struct ssdfs_fsck_superblock_segment_corruption {
	int state;
};

struct ssdfs_fsck_mapping_table_corruption {
	int state;
};

struct ssdfs_fsck_segment_bitmap_corruption {
	int state;
};

struct ssdfs_fsck_inodes_btree_corruption {
	int state;
};

struct ssdfs_fsck_snapshots_btree_corruption {
	int state;
};

struct ssdfs_fsck_invalid_extents_btree_corruption {
	int state;
};

struct ssdfs_fsck_shared_dictionary_btree_corruption {
	int state;
};

struct ssdfs_fsck_corruption_details {
	u64 mask;
	struct ssdfs_fsck_base_snapshot_segment_corruption base_snapshot_seg;
	struct ssdfs_fsck_superblock_segment_corruption superblock_seg;
	struct ssdfs_fsck_mapping_table_corruption mapping_table;
	struct ssdfs_fsck_segment_bitmap_corruption segment_bitmap;
	struct ssdfs_fsck_inodes_btree_corruption inodes_btree;
	struct ssdfs_fsck_snapshots_btree_corruption snapshots_btree;
	struct ssdfs_fsck_invalid_extents_btree_corruption invalid_extents;
	struct ssdfs_fsck_shared_dictionary_btree_corruption shared_dictionary;
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

/* Application APIs */

/* detect_file_system.c */
int is_device_contains_ssdfs_volume(struct ssdfs_fsck_environment *env);

/* check_file_system.c */
int is_ssdfs_volume_corrupted(struct ssdfs_fsck_environment *env);

/* recover_file_system.c */
int recover_corrupted_ssdfs_volume(struct ssdfs_fsck_environment *env);

/* options.c */
void print_usage(void);
void parse_options(int argc, char *argv[],
		   struct ssdfs_fsck_environment *env);

#endif /* _SSDFS_UTILS_FSCK_H */
