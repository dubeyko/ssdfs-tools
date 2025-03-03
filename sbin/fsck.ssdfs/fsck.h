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

struct ssdfs_fsck_detection_result {
	int state;
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

struct ssdfs_fsck_recovery_result {
	int state;
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

/* options.c */
void print_usage(void);
void parse_options(int argc, char *argv[],
		   struct ssdfs_fsck_environment *env);

#endif /* _SSDFS_UTILS_FSCK_H */
