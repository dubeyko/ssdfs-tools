/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/snapshotfs.ssdfs/snapshotfs.h - declarations of snapshot utility.
 *
 * Copyright (c) 2021-2026 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#ifndef _SSDFS_UTILS_SNAPSHOTFS_H
#define _SSDFS_UTILS_SNAPSHOTFS_H

#ifdef snapshotfs_fmt
#undef snapshotfs_fmt
#endif

#include "version.h"

#define snapshotfs_fmt(fmt) "snapshotfs.ssdfs: " SSDFS_UTILS_VERSION ": " fmt

#include "ssdfs_tools.h"

#define SSDFS_SNAPSHOTFS_INFO(show, fmt, ...) \
	do { \
		if (show) { \
			fprintf(stdout, snapshotfs_fmt(fmt), ##__VA_ARGS__); \
		} \
	} while (0)

/*
 * struct ssdfs_snapshot_create_options - create snapshot options
 * @name: snapshot name (optional)
 * @mode: snapshot mode (READ-ONLY|READ-WRITE)
 * @type: snapshot type (PERIODIC|ONE-TIME)
 * @expiration: snapshot expiration time (WEEK|MONTH|YEAR|NEVER)
 * @frequency: taking snapshot frequency (SYNCFS|HOUR|DAY|WEEK)
 * @snapshots_threshold: max number of simultaneously available snapshots
 */
struct ssdfs_snapshot_create_options {
	const char *name;
	int mode;
	int type;
	int expiration;
	int frequency;
	u32 snapshots_threshold;
};

/*
 * struct ssdfs_snapshot_list_options - list snapshot options
 * @time_range: time range to select the snapshots
 * @mode: snapshot mode (READ-ONLY|READ-WRITE)
 * @type: snapshot type (PERIODIC|ONE-TIME)
 * @max_number: max number of snapshots in output
 */
struct ssdfs_snapshot_list_options {
	struct ssdfs_time_range time_range;
	int mode;
	int type;
	u32 max_number;
};

/*
 * struct ssdfs_snapshot_modify_options - modify snapshot options
 * @time_range: time range to search the snapshot
 * @name: snapshot name
 * @id: snapshot ID
 * @mode: snapshot mode (READ-ONLY|READ-WRITE)
 * @type: snapshot type (PERIODIC|ONE-TIME)
 * @expiration: snapshot expiration time (WEEK|MONTH|YEAR|NEVER)
 * @frequency: taking snapshot frequency (SYNCFS|HOUR|DAY|WEEK)
 * @snapshots_threshold: max number of simultaneously available snapshots
 */
struct ssdfs_snapshot_modify_options {
	struct ssdfs_time_range time_range;
	char *name;
	u8 *id;
	int mode;
	int type;
	int expiration;
	int frequency;
	u32 snapshots_threshold;
};

/*
 * struct ssdfs_snapshot_remove_options - remove snapshot options
 * @name: snapshot name
 * @id: snapshot ID
 */
struct ssdfs_snapshot_remove_options {
	char *name;
	u8 *id;
};

/*
 * struct ssdfs_snapshot_remove_range_options - remove range of snapshots options
 * @time_range: time range to select the snapshots
 */
struct ssdfs_snapshot_remove_range_options {
	struct ssdfs_time_range time_range;
};

/*
 * struct ssdfs_snapshot_show_details_options - show details snapshot options
 * @name: snapshot name
 * @id: snapshot ID
 */
struct ssdfs_snapshot_show_details_options {
	char *name;
	u8 *id;
};

/*
 * struct ssdfs_snapshot_options - snapshot options
 * @create: create snapshot options
 * @list: list snapshot options
 * @modify: modify snapshot options
 * @remove: remove snapshot options
 * @remove_range: remove range of snapshots options
 * @show_details: show details snapshot options
 *
 * @show_debug: show debug messages
 * @operation: requested operation
 * @name_buf: name buffer
 * @uuid_buf: UUID buffer
 */
struct ssdfs_snapshot_options {
	struct ssdfs_snapshot_create_options create;
	struct ssdfs_snapshot_list_options list;
	struct ssdfs_snapshot_modify_options modify;
	struct ssdfs_snapshot_remove_options remove;
	struct ssdfs_snapshot_remove_range_options remove_range;
	struct ssdfs_snapshot_show_details_options show_details;

	int show_debug;
	int operation;
	char name_buf[SSDFS_MAX_NAME_LEN];
	u8 uuid_buf[SSDFS_UUID_SIZE];
};

/* Requested operation */
enum {
	SSDFS_UNKNOWN_OPERATION,
	SSDFS_CREATE_SNAPSHOT,
	SSDFS_LIST_SNAPSHOTS,
	SSDFS_MODIFY_SNAPSHOT,
	SSDFS_REMOVE_SNAPSHOT,
	SSDFS_REMOVE_RANGE,
	SSDFS_SHOW_SNAPSHOT_DETAILS,
	SSDFS_OPERATION_TYPE_MAX
};

/* options.c */
void print_usage(void);
void parse_options(int argc, char *argv[],
		   struct ssdfs_snapshot_options *options);

#endif /* _SSDFS_UTILS_SNAPSHOTFS_H */
