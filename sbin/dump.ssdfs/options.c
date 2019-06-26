//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/dump.ssdfs/options.c - parsing command line options functionality.
 *
 * Copyright (c) 2014-2018 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 *
 * HGST Confidential
 * (C) Copyright 2009-2018, HGST, Inc., All rights reserved.
 *
 * Created by HGST, San Jose Research Center, Storage Architecture Group
 * Authors: Vyacheslav Dubeyko <slava@dubeyko.com>
 *
 * Acknowledgement: Cyril Guyot <Cyril.Guyot@wdc.com>
 *                  Zvonimir Bandic <Zvonimir.Bandic@wdc.com>
 */

#include <sys/types.h>
#include <getopt.h>

#include "dumpfs.h"

/************************************************************************
 *                    Options parsing functionality                     *
 ************************************************************************/

void print_version(void)
{
	SSDFS_INFO("dump.ssdfs, part of %s\n", SSDFS_UTILS_VERSION);
}

void print_usage(void)
{
	SSDFS_DUMPFS_INFO(SSDFS_TRUE, "dump volume of SSDFS file system\n\n");
	SSDFS_INFO("Usage: dump.ssdfs <options> [<device> | <image-file>]\n");
	SSDFS_INFO("Options:\n");
	SSDFS_INFO("\t [-d|--debug]\t\t  show debug output.\n");
	SSDFS_INFO("\t [-g|--granularity]\t\t  show key volume's details.\n");
	SSDFS_INFO("\t [-h|--help]\t\t  display help message and exit.\n");
	SSDFS_INFO("\t [-p|--peb id=value,size=value,"
		   "log_index=value,log_size=value,"
		   "parse_header,parse_all,raw_dump]\t  "
		   "show PEB dump.\n");
	SSDFS_INFO("\t [-q|--quiet]\t\t  quiet execution (useful for scripts).\n");
	SSDFS_INFO("\t [-r|--raw-dump show,offset=value,size=value]\t  "
		   "show raw dump.\n");
	SSDFS_INFO("\t [-V|--version]\t\t  print version and exit.\n");
}

static void ssdfs_check_log_index(int value)
{
	if (value >= U16_MAX) {
		SSDFS_ERR("invalid log index option: "
			  "log_index %d is huge\n",
			  value);
		print_usage();
		exit(EXIT_FAILURE);
	}
}

void parse_options(int argc, char *argv[],
		   struct ssdfs_dumpfs_environment *env)
{
	int c;
	int oi = 1;
	char *p;
	char sopts[] = "dghp:qr:V";
	static const struct option lopts[] = {
		{"debug", 0, NULL, 'd'},
		{"granularity", 0, NULL, 'g'},
		{"help", 0, NULL, 'h'},
		{"peb", 1, NULL, 'p'},
		{"quiet", 0, NULL, 'q'},
		{"raw-dump", 1, NULL, 'r'},
		{"version", 0, NULL, 'V'},
		{ }
	};
	enum {
		PEB_ID_OPT = 0,
		PEB_SIZE_OPT,
		PEB_LOG_INDEX_OPT,
		PEB_LOG_SIZE_OPT,
		PEB_PARSE_HEADER_OPT,
		PEB_PARSE_ALL_OPT,
		PEB_SHOW_RAW_DUMP_OPT,
	};
	char *const peb_dump_tokens[] = {
		[PEB_ID_OPT]			= "id",
		[PEB_SIZE_OPT]			= "size",
		[PEB_LOG_INDEX_OPT]		= "log_index",
		[PEB_LOG_SIZE_OPT]		= "log_size",
		[PEB_PARSE_HEADER_OPT]		= "parse_header",
		[PEB_PARSE_ALL_OPT]		= "parse_all",
		[PEB_SHOW_RAW_DUMP_OPT]		= "raw_dump",
		NULL
	};
	enum {
		RAW_DUMP_SHOW_OPT = 0,
		RAW_DUMP_OFFSET_OPT,
		RAW_DUMP_SIZE_OPT,
	};
	char *const raw_dump_tokens[] = {
		[RAW_DUMP_SHOW_OPT]		= "show",
		[RAW_DUMP_OFFSET_OPT]		= "offset",
		[RAW_DUMP_SIZE_OPT]		= "size",
		NULL
	};

	env->command = SSDFS_DUMP_GRANULARITY_COMMAND;

	while ((c = getopt_long(argc, argv, sopts, lopts, &oi)) != EOF) {
		switch (c) {
		case 'd':
			env->base.show_debug = SSDFS_TRUE;
			break;
		case 'g':
			env->command = SSDFS_DUMP_GRANULARITY_COMMAND;
			break;
		case 'h':
			print_usage();
			exit(EXIT_SUCCESS);
		case 'p':
			switch (env->command) {
			case SSDFS_DUMP_GRANULARITY_COMMAND:
			case SSDFS_RAW_DUMP_COMMAND:
				env->command = SSDFS_DUMP_PEB_COMMAND;
				break;

			case SSDFS_DUMP_PEB_COMMAND:
				/* do nothing */
				break;

			default:
				BUG();
			}

			p = optarg;
			while (*p != '\0') {
				struct ssdfs_peb_dump_environment *peb;
				char *value;
				u64 count;

				peb = &env->peb;
				switch (getsubopt(&p, peb_dump_tokens, &value)) {
				case PEB_ID_OPT:
					count = atoll(value);
					peb->id = count;
					break;
				case PEB_SIZE_OPT:
					count = atol(value);
					peb->size = (u32)count;
					break;
				case PEB_LOG_INDEX_OPT:
					count = atoi(value);
					ssdfs_check_log_index(count);
					peb->log_index = (u16)count;
					peb->show_all_logs = SSDFS_FALSE;
					break;
				case PEB_LOG_SIZE_OPT:
					count = atol(value);
					peb->log_size = (u32)count;
					break;
				case PEB_PARSE_HEADER_OPT:
					peb->parse_flags |= SSDFS_PARSE_HEADER;
					break;
				case PEB_PARSE_ALL_OPT:
					peb->parse_flags = SSDFS_PARSE_ALL_MASK;
					break;
				case PEB_SHOW_RAW_DUMP_OPT:
					env->is_raw_dump_requested = SSDFS_TRUE;
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
		case 'r':
			switch (env->command) {
			case SSDFS_DUMP_GRANULARITY_COMMAND:
				env->command = SSDFS_RAW_DUMP_COMMAND;
				break;

			case SSDFS_DUMP_PEB_COMMAND:
			case SSDFS_RAW_DUMP_COMMAND:
				/* do nothing */
				break;

			default:
				BUG();
			}

			env->is_raw_dump_requested = SSDFS_TRUE;

			p = optarg;
			while (*p != '\0') {
				struct ssdfs_raw_dump_environment *raw_dump;
				char *value;
				u64 count;

				raw_dump = &env->raw_dump;
				switch (getsubopt(&p, raw_dump_tokens, &value)) {
				case RAW_DUMP_SHOW_OPT:
					/* do nothing */
					break;
				case RAW_DUMP_OFFSET_OPT:
					count = atoll(value);
					raw_dump->offset = count;
					break;
				case RAW_DUMP_SIZE_OPT:
					count = atol(value);
					raw_dump->size = (u32)count;
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
