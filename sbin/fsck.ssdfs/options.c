/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/fsck.ssdfs/options.c - parsing command line options functionality.
 *
 * Copyright (c) 2025-2026 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#include <sys/types.h>
#include <getopt.h>

#include "fsck.h"

/************************************************************************
 *                    Options parsing functionality                     *
 ************************************************************************/

static void print_version(void)
{
	SSDFS_INFO("fsck.ssdfs, part of %s\n", SSDFS_UTILS_VERSION);
}

void print_usage(void)
{
	SSDFS_FSCK_INFO(SSDFS_TRUE, "check and recover SSDFS file system\n\n");
	SSDFS_INFO("Usage: fsck.ssdfs <options> device\n");
	SSDFS_INFO("Options:\n");
	SSDFS_INFO("\t [-B|--pagesize size]\t  page size of target device "
		   "(4KB|8KB|16KB|32KB).\n");
	SSDFS_INFO("\t [-d|--debug]\t\t  show debug output.\n");
	SSDFS_INFO("\t [-e|--erasesize size]\t  erase size of target device "
		   "(128KB|256KB|512KB|1MB|2MB|4MB|8MB|...).\n");
	SSDFS_INFO("\t [-f|--force]\t\t  force checking even if filesystem is marked clean.\n");
	SSDFS_INFO("\t [-h|--help]\t\t  display help message and exit.\n");
	SSDFS_INFO("\t [-j|--threads]\t\t  define threads number.\n");
	SSDFS_INFO("\t [-n|--no-change]\t  make no changes to the filesystem.\n");
	SSDFS_INFO("\t [-p|--auto-repair]\t  automatic repair.\n");
	SSDFS_INFO("\t [-q|--quiet]\t\t  quiet execution "
		   "(useful for scripts).\n");
	SSDFS_INFO("\t [-s|--segsize size]\t  segment size of target device "
		   "(128KB|256KB|512KB|1MB|2MB|4MB|8MB|16MB|32MB|64MB|...).\n");
	SSDFS_INFO("\t [-y|--yes-all-questions]\t  assume YES to all questions.\n");
	SSDFS_INFO("\t [-v|--be-verbose]\t  be verbose.\n");
	SSDFS_INFO("\t [-V|--version]\t\t  print version and exit.\n");
}

static void check_pagesize(int pagesize)
{
	int err;

	err = __check_pagesize(pagesize);

	if (err) {
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static void check_segsize(u64 segsize)
{
	int err;

	err = __check_segsize(segsize);

	if (err) {
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static void check_erasesize(u64 erasesize)
{
	int err;

	err = __check_erasesize(erasesize);

	if (err) {
		print_usage();
		exit(EXIT_FAILURE);
	}
}

void parse_options(int argc, char *argv[],
		   struct ssdfs_fsck_environment *env)
{
	int c;
	int oi = 1;
	u64 granularity;
	char sopts[] = "B:de:fhj:npqs:yvV";
	static const struct option lopts[] = {
		{"pagesize", 1, NULL, 'B'},
		{"debug", 0, NULL, 'd'},
		{"erasesize", 1, NULL, 'e'},
		{"force", 0, NULL, 'f'},
		{"help", 0, NULL, 'h'},
		{"threads", 1, NULL, 'j'},
		{"no-change", 0, NULL, 'n'},
		{"auto-repair", 0, NULL, 'p'},
		{"quiet", 0, NULL, 'q'},
		{"segsize", 1, NULL, 's'},
		{"yes-all-questions", 0, NULL, 'y'},
		{"be-verbose", 0, NULL, 'v'},
		{"version", 0, NULL, 'V'},
		{ }
	};

	while ((c = getopt_long(argc, argv, sopts, lopts, &oi)) != EOF) {
		switch (c) {
		case 'B':
			granularity = detect_granularity(optarg);
			if (granularity >= U64_MAX) {
				env->base.page_size = atoi(optarg);
				check_pagesize(env->base.page_size);
			} else {
				check_pagesize(granularity);
				env->base.page_size = (u32)granularity;
			}
			break;
		case 'd':
			env->base.show_debug = SSDFS_TRUE;
			break;
		case 'e':
			granularity = detect_granularity(optarg);
			if (granularity >= U64_MAX) {
				env->base.erase_size = atol(optarg);
				check_erasesize(env->base.erase_size);
			} else {
				check_erasesize(granularity);
				env->base.erase_size = (long)granularity;
			}
			break;
		case 'f':
			env->force_checking = SSDFS_TRUE;
			break;
		case 'h':
			print_usage();
			exit(EXIT_SUCCESS);
		case 'j':
			env->threads.capacity = atoi(optarg);
			break;
		case 'n':
			env->no_change = SSDFS_TRUE;
			break;
		case 'p':
			env->auto_repair = SSDFS_TRUE;
			break;
		case 'q':
			env->base.show_info = SSDFS_FALSE;
			break;
		case 's':
			granularity = detect_granularity(optarg);
			if (granularity >= U64_MAX) {
				env->seg_size = atoll(optarg);
				check_segsize(env->seg_size);
			} else {
				check_segsize(granularity);
				env->seg_size = granularity;
			}
			break;
		case 'y':
			env->yes_all_questions = SSDFS_TRUE;
			break;
		case 'v':
			env->be_verbose = SSDFS_TRUE;
			break;
		case 'V':
			print_version();
			exit(EXIT_SUCCESS);
		default:
			print_usage();
			exit(EXIT_FAILURE);
		}
	}

	if (optind != argc - 1) {
		print_usage();
		exit(EXIT_FAILURE);
	}
}
