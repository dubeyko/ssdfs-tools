/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/tune.ssdfs/tunefs.h - declarations of tunefs utility.
 *
 * Copyright (c) 2023-2026 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#ifndef _SSDFS_UTILS_TUNEFS_H
#define _SSDFS_UTILS_TUNEFS_H

#ifdef tunefs_fmt
#undef tunefs_fmt
#endif

#include "version.h"

#define tunefs_fmt(fmt) "tune.ssdfs: " SSDFS_UTILS_VERSION ": " fmt

#include "ssdfs_tools.h"

#define SSDFS_TUNEFS_INFO(show, fmt, ...) \
	do { \
		if (show) { \
			fprintf(stdout, tunefs_fmt(fmt), ##__VA_ARGS__); \
		} \
	} while (0)

#define SSDFS_TUNEFS_SHOW(fmt, ...) \
	do { \
		fprintf(stdout, fmt, ##__VA_ARGS__); \
	} while (0)

#define SSDFS_ENABLE_OPTION_STRING	"enable"
#define SSDFS_DISABLE_OPTION_STRING	"disable"

#define SSDFS_NONE_COMPRESSION_STRING	"none"
#define SSDFS_ZLIB_COMPRESSION_STRING	"zlib"
#define SSDFS_LZO_COMPRESSION_STRING	"lzo"

#define SSDFS_TUNEFS_LABEL_STRING		"LABEL"
#define SSDFS_TUNEFS_BLOCK_BITMAP_STRING	"BLOCK BITMAP"
#define SSDFS_TUNEFS_BLK2OFF_TABLE_STRING	"OFFSETS TRANSLATION TABLE"
#define SSDFS_TUNEFS_SEGMENT_BITMAP_STRING	"SEGMENT BITMAP"
#define SSDFS_TUNEFS_MAPPING_TABLE_STRING	"PEB MAPPING TABLE"
#define SSDFS_TUNEFS_BTREE_STRING		"B-TREE"
#define SSDFS_TUNEFS_BTREE_LNODE_STRING		"B-TREE: LEAF NODE"
#define SSDFS_TUNEFS_BTREE_HNODE_STRING		"B-TREE: HYBRID NODE"
#define SSDFS_TUNEFS_BTREE_INODE_STRING		"B-TREE: INDEX NODE"
#define SSDFS_TUNEFS_USER_DATA_STRING		"USER DATA"

#define SSDFS_TUNEFS_VOLUME_LABEL_OPTION_STRING		"volume_label"
#define SSDFS_TUNEFS_HAS_BACKUP_OPTION_STRING		"has_backup_copy"
#define SSDFS_TUNEFS_COMPRESSION_OPTION_STRING		"compression"
#define SSDFS_TUNEFS_LOG_PAGES_OPTION_STRING		"log_pages"
#define SSDFS_TUNEFS_MIGRATION_THRESHOLD_OPTION_STRING	"migration_threshold"
#define SSDFS_TUNEFS_RESERVED_PEBS4FRAG_OPTION_STRING	"reserved_pebs_per_fragment"
#define SSDFS_TUNEFS_MIN_INDEX_AREA_SZ_OPTION_STRING	"min_index_area_size"

#define SSDFS_TUNEFS_UNKNOWN_OPTION_VALUE		(-1)
#define SSDFS_TUNEFS_UNKNOWN_RECOMMENDED_VALUE		(-1)

/*
 * struct ssdfs_volume_environment - volume environment
 * @need_get_config: does it need to get current config?
 * @options: configuration options
 * @generic: generic options
 */
struct ssdfs_volume_environment {
	int need_get_config;
	struct ssdfs_tunefs_options options;
	struct ssdfs_environment generic;
};

/* options.c */
void print_version(void);
void print_usage(void);
void parse_options(int argc, char *argv[],
		   struct ssdfs_volume_environment *env);

#endif /* _SSDFS_UTILS_TUNEFS_H */
