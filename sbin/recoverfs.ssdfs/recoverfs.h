//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/recoverfs.ssdfs/recoverfs.h - declarations of recoverfs utility.
 *
 * Copyright (c) 2022-2023 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#ifndef _SSDFS_UTILS_RECOVERFS_H
#define _SSDFS_UTILS_RECOVERFS_H

#ifdef recoverfs_fmt
#undef recoverfs_fmt
#endif

#include "version.h"

#define recoverfs_fmt(fmt) "recoverfs.ssdfs: " SSDFS_UTILS_VERSION ": " fmt

#include "ssdfs_tools.h"

#define SSDFS_RECOVERFS_INFO(show, fmt, ...) \
	do { \
		if (show) { \
			fprintf(stdout, recoverfs_fmt(fmt), ##__VA_ARGS__); \
		} \
	} while (0)

#define SSDFS_RECOVERFS_DEFAULT_THREADS		(1)

/*
 * struct ssdfs_recoverfs_environment - recoverfs environment
 * @base: basic environment
 * @threads: number of threads
 * @output_folder: path to the output folder
 * @output_fd: output folder descriptor
 */
struct ssdfs_recoverfs_environment {
	struct ssdfs_environment base;
	unsigned int threads;
	const char *output_folder;
	int output_fd;
};

/* Inline functions */

static inline
int is_pagesize_valid(int pagesize)
{
	switch (pagesize) {
	case SSDFS_4KB:
	case SSDFS_8KB:
	case SSDFS_16KB:
	case SSDFS_32KB:
	case SSDFS_64KB:
	case SSDFS_128KB:
		/* do nothing: proper value */
		break;

	default:
		return SSDFS_FALSE;
	}

	return SSDFS_TRUE;
}

/* options.c */
void print_usage(void);
void parse_options(int argc, char *argv[],
		   struct ssdfs_recoverfs_environment *env);

#endif /* _SSDFS_UTILS_RECOVERFS_H */
