//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/recoverfs.ssdfs/options.c - parsing command line options functionality.
 *
 * Copyright (c) 2022 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#include <sys/types.h>
#include <getopt.h>

#include "recoverfs.h"

/************************************************************************
 *                    Options parsing functionality                     *
 ************************************************************************/

static void print_version(void)
{
	SSDFS_INFO("recoverfs.ssdfs, part of %s\n", SSDFS_UTILS_VERSION);
}

void print_usage(void)
{
	SSDFS_RECOVERFS_INFO(SSDFS_TRUE, "recover SSDFS file system\n\n");
	SSDFS_INFO("Usage: recoverfs.ssdfs <options> device root-folder\n");
	SSDFS_INFO("Options:\n");
	SSDFS_INFO("\t [-d|--debug]\t\t  show debug output.\n");
	SSDFS_INFO("\t [-h|--help]\t\t  display help message and exit.\n");
	SSDFS_INFO("\t [-j|--threads]\t\t  define threads number.\n");
	SSDFS_INFO("\t [-q|--quiet]\t\t  quiet execution "
		   "(useful for scripts).\n");
	SSDFS_INFO("\t [-V|--version]\t\t  print version and exit.\n");
}

void parse_options(int argc, char *argv[],
		   struct ssdfs_recoverfs_environment *env)
{
	int c;
	int oi = 1;
	char sopts[] = "dhj:qV";
	static const struct option lopts[] = {
		{"debug", 0, NULL, 'd'},
		{"help", 0, NULL, 'h'},
		{"threads", 1, NULL, 'j'},
		{"quiet", 0, NULL, 'q'},
		{"version", 0, NULL, 'V'},
		{ }
	};

	while ((c = getopt_long(argc, argv, sopts, lopts, &oi)) != EOF) {
		switch (c) {
		case 'd':
			env->base.show_debug = SSDFS_TRUE;
			break;
		case 'h':
			print_usage();
			exit(EXIT_SUCCESS);
		case 'j':
			env->threads = atoi(optarg);
			break;
		case 'q':
			env->base.show_info = SSDFS_FALSE;
			break;
		case 'V':
			print_version();
			exit(EXIT_SUCCESS);
		default:
			print_usage();
			exit(EXIT_FAILURE);
		}
	}

	if (optind != argc - 2) {
		print_usage();
		exit(EXIT_FAILURE);
	}
}
