/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/resize.ssdfs/resizefs.h - declarations of resizefs utility.
 *
 * Copyright (c) 2023-2025 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#ifndef _SSDFS_UTILS_RESIZEFS_H
#define _SSDFS_UTILS_RESIZEFS_H

#ifdef resizefs_fmt
#undef resizefs_fmt
#endif

#include "version.h"

#define resizefs_fmt(fmt) "resize.ssdfs: " SSDFS_UTILS_VERSION ": " fmt

#include "ssdfs_tools.h"

#define SSDFS_RESIZEFS_INFO(show, fmt, ...) \
	do { \
		if (show) { \
			fprintf(stdout, resizefs_fmt(fmt), ##__VA_ARGS__); \
		} \
	} while (0)

#define SSDFS_RESIZEFS_SHOW(fmt, ...)({ \
	do { \
		fprintf(stdout, fmt, ##__VA_ARGS__); \
	} while (0)

/*
 * struct ssdfs_new_volume_state - new state of the volume
 * @segments_difference: change of the volume in segments number
 * @percentage_change: change of the volume in percentage from current size
 */
struct ssdfs_new_volume_state {
	u64 segments_difference;
	u32 percentage_change;
};

/*
 * struct ssdfs_new_volume_state_option - volume state option
 * @state: state of the option (ignore or enable)
 * @value: new volume state value
 */
struct ssdfs_new_volume_state_option {
	int state;
	struct ssdfs_new_volume_state value;
};

/*
 * struct ssdfs_new_volume_size_option - volume size option
 * @state: state of the option (ignore or enable)
 * @value: new volume size value
 */
struct ssdfs_new_volume_size_option {
	int state;
	u64 value;
};

/*
 * struct ssdfs_volume_label_option - volume label option
 * @state: state of the option (ignore or enable)
 * @string: volume label string
 */
struct ssdfs_volume_label_option {
	int state;
	char string[SSDFS_VOLUME_LABEL_MAX];
};

/*
 * struct ssdfs_resizefs_environment - resizefs environment
 * @grow_option: grow volume option
 * @shrink_option: shrink volume option
 * @new_size: new size of the volume in bytes
 * @volume_label: volume label string
 * @need_make_snapshot: does it need to make snapshot before resize?
 * @check_by_fsck: does it need to check volume by fsck after resize?
 * @force_resize: does it need to force the resize operation?
 * @rollback_resize: does it need to rollback the resize?
 * @generic: generic options
 */
struct ssdfs_resizefs_environment {
	struct ssdfs_new_volume_state_option grow_option;
	struct ssdfs_new_volume_state_option shrink_option;
	struct ssdfs_new_volume_size_option new_size;
	struct ssdfs_volume_label_option volume_label;

	int need_make_snapshot;
	int check_by_fsck;
	int force_resize;
	int rollback_resize;
	struct ssdfs_environment generic;
};

/* options.c */
void print_version(void);
void print_usage(void);
void parse_options(int argc, char *argv[],
		   struct ssdfs_resizefs_environment *env);

#endif /* _SSDFS_UTILS_RESIZEFS_H */
