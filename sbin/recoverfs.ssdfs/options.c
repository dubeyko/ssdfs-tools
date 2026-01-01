/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/recoverfs.ssdfs/options.c - parsing command line options functionality.
 *
 * Copyright (c) 2022-2026 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
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
	SSDFS_INFO("\t [-t|--timestamp minute=value, "
		   "hour=value, day=value, month=value, "
		   "year=value]\t\t  define timestamp of files state.\n");
	SSDFS_INFO("\t [-q|--quiet]\t\t  quiet execution "
		   "(useful for scripts).\n");
	SSDFS_INFO("\t [-V|--version]\t\t  print version and exit.\n");
}

static inline
void check_minute(int minute)
{
	if (minute < 0 || minute > 60) {
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static inline
void check_hour(int hour)
{
	if (hour < 0 || hour > 24) {
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static inline
void check_day(int day)
{
	if (day <= 0 || day > 31) {
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static inline
void check_month(int month)
{
	if (month <= 0 || month > 12) {
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static inline
void check_year(int year)
{
	if (year < 1970) {
		print_usage();
		exit(EXIT_FAILURE);
	}
}

void parse_options(int argc, char *argv[],
		   struct ssdfs_recoverfs_environment *env)
{
	int c;
	char *p;
	int oi = 1;
	char sopts[] = "dhj:t:qV";
	static const struct option lopts[] = {
		{"debug", 0, NULL, 'd'},
		{"help", 0, NULL, 'h'},
		{"threads", 1, NULL, 'j'},
		{"timestamp", 1, NULL, 't'},
		{"quiet", 0, NULL, 'q'},
		{"version", 0, NULL, 'V'},
		{ }
	};
	enum {
		TIMESTAMP_MINUTE_OPT = 0,
		TIMESTAMP_HOUR_OPT,
		TIMESTAMP_DAY_OPT,
		TIMESTAMP_MONTH_OPT,
		TIMESTAMP_YEAR_OPT,
	};
	char *const timestamp_tokens[] = {
		[TIMESTAMP_MINUTE_OPT]		= "minute",
		[TIMESTAMP_HOUR_OPT]		= "hour",
		[TIMESTAMP_DAY_OPT]		= "day",
		[TIMESTAMP_MONTH_OPT]		= "month",
		[TIMESTAMP_YEAR_OPT]		= "year",
		NULL
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
			env->threads.capacity = atoi(optarg);
			break;
		case 't':
			p = optarg;
			while (*p != '\0') {
				char *value;

				switch (getsubopt(&p, timestamp_tokens, &value)) {
				case TIMESTAMP_MINUTE_OPT:
					env->timestamp.minute = atoi(value);
					check_minute(env->timestamp.minute);
					break;
				case TIMESTAMP_HOUR_OPT:
					env->timestamp.hour = atoi(value);
					check_hour(env->timestamp.hour);
					break;
				case TIMESTAMP_DAY_OPT:
					env->timestamp.day = atoi(value);
					check_day(env->timestamp.day);
					break;
				case TIMESTAMP_MONTH_OPT:
					env->timestamp.month = atoi(value);
					check_month(env->timestamp.month);
					break;
				case TIMESTAMP_YEAR_OPT:
					env->timestamp.year = atoi(value);
					check_year(env->timestamp.year);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
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
