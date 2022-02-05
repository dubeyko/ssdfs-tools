//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/test.ssdfs/testing.h - declarations of testing utility.
 *
 * Copyright (c) 2021-2022 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#ifndef _SSDFS_UTILS_TESTING_H
#define _SSDFS_UTILS_TESTING_H

#ifdef testfs_fmt
#undef testfs_fmt
#endif

#include "version.h"

#define testfs_fmt(fmt) "test.ssdfs: " SSDFS_UTILS_VERSION ": " fmt

#include "ssdfs_tools.h"

#define SSDFS_TESTFS_INFO(show, fmt, ...) \
	do { \
		if (show) { \
			fprintf(stdout, testfs_fmt(fmt), ##__VA_ARGS__); \
		} \
	} while (0)

#define SSDFS_TESTFS_DEFAULT_PAGE_SIZE			SSDFS_4KB
#define SSDFS_TESTFS_DEFAULT_FILE_COUNT_MAX		(1000000)
#define SSDFS_TESTFS_DEFAULT_EXTENT_LEN			(16)
#define SSDFS_TESTFS_DEFAULT_BLK_BMAP_CAPACITY		(4096)
#define SSDFS_TESTFS_DEFAULT_PRE_ALLOC_PER_ITER		(8)
#define SSDFS_TESTFS_DEFAULT_ALLOC_PER_ITER		(8)
#define SSDFS_TESTFS_DEFAULT_INVALIDATE_PER_ITER	(2)
#define SSDFS_TESTFS_DEFAULT_RESERVED_PER_ITER		(2)
#define SSDFS_TESTFS_DEFAULT_BLK2OFF_TBL_CAPACITY	(2048)
#define SSDFS_TESTFS_DEFAULT_MAPPING_TBL_ITERATIONS	(1000)
#define SSDFS_TESTFS_DEFAULT_MAPPINGS_PER_ITER		(8)
#define SSDFS_TESTFS_DEFAULT_ADD_MIGRATIONS_PER_ITER	(4)
#define SSDFS_TESTFS_DEFAULT_DELETE_MIGRATIONS_PER_ITER	(2)
#define SSDFS_TESTFS_DEFAULT_SEGBMAP_ITERATIONS		(1000)
#define SSDFS_TESTFS_DEFAULT_USING_SEGS_PER_ITER	(8)
#define SSDFS_TESTFS_DEFAULT_USED_SEGS_PER_ITER		(2)
#define SSDFS_TESTFS_DEFAULT_PRE_DIRTY_SEGS_PER_ITER	(2)
#define SSDFS_TESTFS_DEFAULT_DIRTY_SEGS_PER_ITER	(2)
#define SSDFS_TESTFS_DEFAULT_CLEANED_SEGS_PER_ITER	(2)
#define SSDFS_TESTFS_DEFAULT_LONG_NAMES_NUMBER		(1000)
#define SSDFS_TESTFS_DEFAULT_LONG_NAME_LENGTH		(100)
#define SSDFS_TESTFS_DEFAULT_NAME_STEP_FACTOR		(2)
#define SSDFS_TESTFS_DEFAULT_XATTRS_NUMBER		(1000)
#define SSDFS_TESTFS_DEFAULT_XATTR_BLOB_LEN		SSDFS_512B
#define SSDFS_TESTFS_DEFAULT_XATTR_BLOB_PATTERN		SSDFS_SUPER_MAGIC
#define SSDFS_TESTFS_DEFAULT_SHARED_EXTENTS_NUMBER	(1000)
#define SSDFS_TESTFS_DEFAULT_SHARED_EXTENT_LENGTH	(16)
#define SSDFS_TESTFS_DEFAULT_SHARED_EXTENT_REFS_MAX	(32)

/* options.c */
void print_usage(void);
void parse_options(int argc, char *argv[],
		   struct ssdfs_testing_environment *env);

#endif /* _SSDFS_UTILS_TESTING_H */
