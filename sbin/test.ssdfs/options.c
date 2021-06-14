//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/test.ssdfs/options.c - parsing command line options functionality.
 *
 * Copyright (c) 2021 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#include <sys/types.h>
#include <getopt.h>

#include "testing.h"

/************************************************************************
 *                    Options parsing functionality                     *
 ************************************************************************/

static void print_version(void)
{
	SSDFS_INFO("test.ssdfs, part of %s\n", SSDFS_UTILS_VERSION);
}

void print_usage(void)
{
	SSDFS_TESTFS_INFO(SSDFS_TRUE, "test SSDFS file system\n\n");
	SSDFS_INFO("Usage: test.ssdfs <options> [<device> | <image-file>]\n");
	SSDFS_INFO("Options:\n");
	SSDFS_INFO("\t [-a|--all]\t\t  test all subsystems.\n");
	SSDFS_INFO("\t [-e|--extent max_len=value]\t  "
		   "define extent related thresholds.\n");
	SSDFS_INFO("\t [-f|--file max_count=value,max_size=value]\t  "
		   "define file related thresholds.\n");
	SSDFS_INFO("\t [-h|--help]\t\t  display help message and exit.\n");
	SSDFS_INFO("\t [-p|--pagesize size]\t  page size of target device "
		   "(4096|8192|16384|32768 bytes).\n");
	SSDFS_INFO("\t [-s|--subsystem dentries_tree,extents_tree]\t  "
		   "define testing subsystems.\n");
	SSDFS_INFO("\t [-V|--version]\t\t  print version and exit.\n");
}

static void check_pagesize(int pagesize)
{
	switch (pagesize) {
	case SSDFS_4KB:
	case SSDFS_8KB:
	case SSDFS_16KB:
	case SSDFS_32KB:
		/* do nothing: proper value */
		break;

	default:
		print_usage();
		exit(EXIT_FAILURE);
	}
}

void parse_options(int argc, char *argv[],
		   struct ssdfs_testing_environment *env)
{
	int c;
	int oi = 1;
	char *p;
	char sopts[] = "ae:f:hp:s:V";
	static const struct option lopts[] = {
		{"all", 0, NULL, 'a'},
		{"extent", 1, NULL, 'e'},
		{"file", 1, NULL, 'f'},
		{"help", 0, NULL, 'h'},
		{"pagesize", 1, NULL, 'p'},
		{"subsystem", 1, NULL, 's'},
		{"version", 0, NULL, 'V'},
		{ }
	};
	enum {
		EXTENT_MAX_LEN_OPT = 0,
	};
	char *const extent_tokens[] = {
		[EXTENT_MAX_LEN_OPT]		= "max_len",
		NULL
	};
	enum {
		FILE_MAX_COUNT_OPT = 0,
		FILE_MAX_SIZE_OPT,
	};
	char *const file_tokens[] = {
		[FILE_MAX_COUNT_OPT]		= "max_count",
		[FILE_MAX_SIZE_OPT]		= "max_size",
		NULL
	};
	enum {
		DENTRIES_TREE_SUBSYSTEM_OPT = 0,
		EXTENTS_TREE_SUBSYSTEM_OPT,
	};
	char *const subsystem_tokens[] = {
		[DENTRIES_TREE_SUBSYSTEM_OPT]	= "dentries_tree",
		[EXTENTS_TREE_SUBSYSTEM_OPT]	= "extents_tree",
		NULL
	};

	while ((c = getopt_long(argc, argv, sopts, lopts, &oi)) != EOF) {
		switch (c) {
		case 'a':
			env->subsystems |= SSDFS_ENABLE_EXTENTS_TREE_TESTING;
			env->subsystems |= SSDFS_ENABLE_DENTRIES_TREE_TESTING;
			break;
		case 'e':
			p = optarg;
			while (*p != '\0') {
				char *value;
				int len;

				switch (getsubopt(&p, extent_tokens, &value)) {
				case EXTENT_MAX_LEN_OPT:
					len = atoi(value);

					if (len >= U16_MAX) {
						print_usage();
						exit(EXIT_FAILURE);
					}

					env->extent_len_threshold = (u16)len;
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'f':
			p = optarg;
			while (*p != '\0') {
				char *value;
				u64 count;
				u64 size;

				switch (getsubopt(&p, file_tokens, &value)) {
				case FILE_MAX_COUNT_OPT:
					count = atoll(value);
					env->files_number_threshold = count;
					break;
				case FILE_MAX_SIZE_OPT:
					size = atoll(value);
					env->file_size_threshold = size;
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'h':
			print_usage();
			exit(EXIT_SUCCESS);
		case 'p':
			env->page_size = atoi(optarg);
			check_pagesize(env->page_size);
			break;
		case 's':
			p = optarg;
			while (*p != '\0') {
				char *value;

				switch (getsubopt(&p, subsystem_tokens, &value)) {
				case DENTRIES_TREE_SUBSYSTEM_OPT:
					env->subsystems |=
					    SSDFS_ENABLE_DENTRIES_TREE_TESTING;
					break;
				case EXTENTS_TREE_SUBSYSTEM_OPT:
					env->subsystems |=
					    SSDFS_ENABLE_EXTENTS_TREE_TESTING;
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
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
