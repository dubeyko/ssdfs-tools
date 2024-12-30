/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/tune.ssdfs/tunefs.h - declarations of tunefs utility.
 *
 * Copyright (c) 2023-2024 Viacheslav Dubeyko <slava@dubeyko.com>
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
