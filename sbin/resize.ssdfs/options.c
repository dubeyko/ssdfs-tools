/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/resize.ssdfs/options.c - parsing command line options functionality.
 *
 * Copyright (c) 2023-2025 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#include <sys/types.h>
#include <getopt.h>

#include "resizefs.h"

/************************************************************************
 *                    Options parsing functionality                     *
 ************************************************************************/

void print_version(void)
{
	SSDFS_INFO("resize.ssdfs, part of %s\n", SSDFS_UTILS_VERSION);
}

void print_usage(void)
{
	SSDFS_RESIZEFS_INFO(SSDFS_TRUE, "resize volume of SSDFS file system\n\n");
	SSDFS_INFO("Usage: resize.ssdfs <options> [<device> | <image-file>]\n");
	SSDFS_INFO("Options:\n");
	SSDFS_INFO("\t [-b|--make-snapshot]\t  "
		   "make volume snapshot before resize.\n");
	SSDFS_INFO("\t [-c|--check-by-fsck]\t  "
		   "check volume by fsck after resize.\n");
	SSDFS_INFO("\t [-d|--debug]\t\t  show debug output.\n");
	SSDFS_INFO("\t [-g|--grow-by-segments number]\t  "
		   "grow volume by segments number.\n");
	SSDFS_INFO("\t [-G|--grow-by-percentage percentage]\t  "
		   "grow volume on percentage value.\n");
	SSDFS_INFO("\t [-f|--force]\t\t  force file system volume resize.\n");
	SSDFS_INFO("\t [-h|--help]\t\t  display help message and exit.\n");
	SSDFS_INFO("\t [-L|--label string]\t\t  set a volume label.\n");
	SSDFS_INFO("\t [-n|--new_size size]\t  new volume size in bytes.\n");
	SSDFS_INFO("\t [-r|--rollback]\t\t  rollback volume resize.\n");
	SSDFS_INFO("\t [-s|--shrink-by-segments number]\t  "
		   "shrink volume by segments number.\n");
	SSDFS_INFO("\t [-S|--shrink-by-percentage percentage]\t  "
		   "shrink volume on percentage value.\n");
	SSDFS_INFO("\t [-V|--version]\t\t  print version and exit.\n");
}

void parse_options(int argc, char *argv[],
		   struct ssdfs_resizefs_environment *env)
{
	int c;
	int oi = 1;
	char sopts[] = "bcdg:G:fhL:n:rs:S:V";
	static const struct option lopts[] = {
		{"make-snapshot", 0, NULL, 'b'},
		{"check-by-fsck", 0, NULL, 'c'},
		{"debug", 0, NULL, 'd'},
		{"grow-by-segments", 1, NULL, 'g'},
		{"grow-by-percentage", 1, NULL, 'G'},
		{"force", 0, NULL, 'f'},
		{"help", 0, NULL, 'h'},
		{"label", 1, NULL, 'L'},
		{"new_size", 1, NULL, 'n'},
		{"rollback", 0, NULL, 'r'},
		{"shrink-by-segments", 1, NULL, 's'},
		{"shrink-by-percentage", 1, NULL, 'S'},
		{"version", 0, NULL, 'V'},
		{ }
	};

	while ((c = getopt_long(argc, argv, sopts, lopts, &oi)) != EOF) {
		switch (c) {
		case 'b':
			env->need_make_snapshot = SSDFS_TRUE;
			break;
		case 'c':
			env->check_by_fsck = SSDFS_TRUE;
			break;
		case 'd':
			env->generic.show_debug = SSDFS_TRUE;
			break;
		case 'g':
			if (env->rollback_resize == SSDFS_TRUE) {
				SSDFS_ERR("resize rollback cannot be requested with resize!!!\n");
				print_usage();
				exit(EXIT_FAILURE);
			}

			if (env->shrink_option.state != SSDFS_IGNORE_OPTION) {
				SSDFS_ERR("grow and shrink cannot be requested together!!!\n");
				print_usage();
				exit(EXIT_FAILURE);
			}

			env->grow_option.value.segments_difference =
								atoll(optarg);
			env->grow_option.state = SSDFS_ENABLE_OPTION;
			break;
		case 'G':
			if (env->rollback_resize == SSDFS_TRUE) {
				SSDFS_ERR("resize rollback cannot be requested with resize!!!\n");
				print_usage();
				exit(EXIT_FAILURE);
			}

			if (env->shrink_option.state != SSDFS_IGNORE_OPTION) {
				SSDFS_ERR("grow and shrink cannot be requested together!!!\n");
				print_usage();
				exit(EXIT_FAILURE);
			}

			env->grow_option.value.percentage_change = atoi(optarg);
			env->grow_option.state = SSDFS_ENABLE_OPTION;
			break;
		case 'f':
			if (env->rollback_resize == SSDFS_TRUE) {
				SSDFS_ERR("resize rollback cannot be requested with resize!!!\n");
				print_usage();
				exit(EXIT_FAILURE);
			}

			env->force_resize = SSDFS_TRUE;
			break;
		case 'h':
			print_usage();
			exit(EXIT_SUCCESS);
		case 'L':
			if (env->rollback_resize == SSDFS_TRUE) {
				SSDFS_ERR("resize rollback cannot be requested with resize!!!\n");
				print_usage();
				exit(EXIT_FAILURE);
			}

			strncpy(env->volume_label.string, optarg,
				sizeof(env->volume_label.string) - 1);
			env->volume_label.state = SSDFS_ENABLE_OPTION;
			break;
		case 'n':
			if (env->rollback_resize == SSDFS_TRUE) {
				SSDFS_ERR("resize rollback cannot be requested with resize!!!\n");
				print_usage();
				exit(EXIT_FAILURE);
			}

			env->new_size.value = atoll(optarg);
			env->new_size.state = SSDFS_ENABLE_OPTION;
			break;
		case 'r':
			if (env->shrink_option.state != SSDFS_IGNORE_OPTION) {
				SSDFS_ERR("resize rollback cannot be requested with resize!!!\n");
				print_usage();
				exit(EXIT_FAILURE);
			}

			if (env->grow_option.state != SSDFS_IGNORE_OPTION) {
				SSDFS_ERR("resize rollback cannot be requested with resize!!!\n");
				print_usage();
				exit(EXIT_FAILURE);
			}

			if (env->new_size.state != SSDFS_IGNORE_OPTION) {
				SSDFS_ERR("resize rollback cannot be requested with resize!!!\n");
				print_usage();
				exit(EXIT_FAILURE);
			}

			if (env->force_resize == SSDFS_TRUE) {
				SSDFS_ERR("resize rollback cannot be requested with resize!!!\n");
				print_usage();
				exit(EXIT_FAILURE);
			}

			env->rollback_resize = SSDFS_TRUE;
			break;
		case 's':
			if (env->rollback_resize == SSDFS_TRUE) {
				SSDFS_ERR("resize rollback cannot be requested with resize!!!\n");
				print_usage();
				exit(EXIT_FAILURE);
			}

			if (env->grow_option.state != SSDFS_IGNORE_OPTION) {
				SSDFS_ERR("grow and shrink cannot be requested together!!!\n");
				print_usage();
				exit(EXIT_FAILURE);
			}

			env->shrink_option.value.segments_difference =
								atoll(optarg);
			env->shrink_option.state = SSDFS_ENABLE_OPTION;
			break;
		case 'S':
			if (env->rollback_resize == SSDFS_TRUE) {
				SSDFS_ERR("resize rollback cannot be requested with resize!!!\n");
				print_usage();
				exit(EXIT_FAILURE);
			}

			if (env->grow_option.state != SSDFS_IGNORE_OPTION) {
				SSDFS_ERR("grow and shrink cannot be requested together!!!\n");
				print_usage();
				exit(EXIT_FAILURE);
			}

			env->shrink_option.value.percentage_change = atoi(optarg);
			env->shrink_option.state = SSDFS_ENABLE_OPTION;
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
