//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/test.ssdfs/testing.h - declarations of testing utility.
 *
 * Copyright (c) 2021 Viacheslav Dubeyko <slava@dubeyko.com>
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

#define SSDFS_TESTFS_DEFAULT_PAGE_SIZE		SSDFS_4KB
#define SSDFS_TESTFS_DEFAULT_FILE_COUNT_MAX	(1000000)
#define SSDFS_TESTFS_DEFAULT_EXTENT_LEN		(16)

/* options.c */
void print_usage(void);
void parse_options(int argc, char *argv[],
		   struct ssdfs_testing_environment *env);

#endif /* _SSDFS_UTILS_TESTING_H */
