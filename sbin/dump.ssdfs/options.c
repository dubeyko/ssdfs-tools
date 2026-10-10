/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/dump.ssdfs/options.c - parsing command line options functionality.
 *
 * Copyright (c) 2014-2019 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 * Copyright (c) 2014-2026 Viacheslav Dubeyko <slava@dubeyko.com>
 *              http://www.ssdfs.org/
 *
 * (C) Copyright 2014-2019, HGST, Inc., All rights reserved.
 *
 * Created by HGST, San Jose Research Center, Storage Architecture Group
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 *
 * Acknowledgement: Cyril Guyot
 *                  Zvonimir Bandic
 */

#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
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
	SSDFS_INFO("\t [-o|--output-folder]\t\t  define output folder.\n");
	SSDFS_INFO("\t [-p|--peb id=value,peb_count=value,size=value,"
		   "log_index=value,log_count=value,log_size=value,"
		   "parse_header,parse_log_footer,parse_block_bitmap,"
		   "parse_blk2off_table,parse_block_state_area,"
		   "parse_maptbl_cache,parse_meta_extents,parse_maptbl,"
		   "parse_segbmap,"
		   "parse_all,raw_dump]\t  show PEB dump.\n");
	SSDFS_INFO("\t [-q|--quiet]\t\t  quiet execution (useful for scripts).\n");
	SSDFS_INFO("\t [-r|--raw-dump show,offset=value,size=value]\t  "
		   "show raw dump.\n");
	SSDFS_INFO("\t [-V|--version]\t\t  print version and exit.\n");
}

static void ssdfs_check_log_index(u32 value)
{
	if (value >= U32_MAX) {
		SSDFS_ERR("invalid log index option: "
			  "log_index %u is huge\n",
			  value);
		print_usage();
		exit(EXIT_FAILURE);
	}
}

/*
 * Parse the numeric value of a sub-option. The value must be
 * a non-negative decimal number in the range [min, max].
 */
static u64 ssdfs_parse_value(const char *opt, const char *value,
			     u64 min, u64 max)
{
	char *end = NULL;
	u64 res;

	if (!value || !isdigit((unsigned char)*value)) {
		SSDFS_ERR("option %s requires a numeric value: %s\n",
			  opt, value ? value : "(missing)");
		goto fail;
	}

	errno = 0;
	res = strtoull(value, &end, 10);
	if (errno || *end != '\0') {
		SSDFS_ERR("invalid value of option %s: %s\n", opt, value);
		goto fail;
	}

	if (res < min || res > max) {
		SSDFS_ERR("value of option %s is out of range "
			  "[%llu, %llu]: %s\n",
			  opt, min, max, value);
		goto fail;
	}

	return res;

fail:
	print_usage();
	exit(EXIT_FAILURE);
}

static u32 ssdfs_parse_peb_size(const char *value)
{
	u64 size = ssdfs_parse_value("size", value, SSDFS_4KB, 1ULL << 31);

	if (size & (size - 1)) {
		SSDFS_ERR("PEB size must be a power of two: %s\n", value);
		print_usage();
		exit(EXIT_FAILURE);
	}

	return (u32)size;
}

void parse_options(int argc, char *argv[],
		   struct ssdfs_dumpfs_environment *env)
{
	int c;
	int oi = 1;
	char *p;
	char sopts[] = "dgho:p:qr:V";
	static const struct option lopts[] = {
		{"debug", 0, NULL, 'd'},
		{"granularity", 0, NULL, 'g'},
		{"help", 0, NULL, 'h'},
		{"output-folder", 1, NULL, 'o'},
		{"peb", 1, NULL, 'p'},
		{"quiet", 0, NULL, 'q'},
		{"raw-dump", 1, NULL, 'r'},
		{"version", 0, NULL, 'V'},
		{ }
	};
	enum {
		PEB_ID_OPT = 0,
		PEB_COUNT_OPT,
		PEB_SIZE_OPT,
		PEB_LOG_INDEX_OPT,
		PEB_LOG_COUNT_OPT,
		PEB_LOG_SIZE_OPT,
		PEB_PARSE_HEADER_OPT,
		PEB_PARSE_LOG_FOOTER_OPT,
		PEB_PARSE_BLOCK_BITMAP_OPT,
		PEB_PARSE_BLK2OFF_TBL_OPT,
		PEB_PARSE_BLOCK_STATE_OPT,
		PEB_PARSE_MAPTBL_CACHE_OPT,
		PEB_PARSE_META_EXTENTS_OPT,
		PEB_PARSE_MAPTBL_OPT,
		PEB_PARSE_SEGBMAP_OPT,
		PEB_PARSE_ALL_OPT,
		PEB_SHOW_RAW_DUMP_OPT,
	};
	char *const peb_dump_tokens[] = {
		[PEB_ID_OPT]			= "id",
		[PEB_COUNT_OPT]			= "peb_count",
		[PEB_SIZE_OPT]			= "size",
		[PEB_LOG_INDEX_OPT]		= "log_index",
		[PEB_LOG_COUNT_OPT]		= "log_count",
		[PEB_LOG_SIZE_OPT]		= "log_size",
		[PEB_PARSE_HEADER_OPT]		= "parse_header",
		[PEB_PARSE_LOG_FOOTER_OPT]	= "parse_log_footer",
		[PEB_PARSE_BLOCK_BITMAP_OPT]	= "parse_block_bitmap",
		[PEB_PARSE_BLK2OFF_TBL_OPT]	= "parse_blk2off_table",
		[PEB_PARSE_BLOCK_STATE_OPT]	= "parse_block_state_area",
		[PEB_PARSE_MAPTBL_CACHE_OPT]	= "parse_maptbl_cache",
		[PEB_PARSE_META_EXTENTS_OPT]	= "parse_meta_extents",
		[PEB_PARSE_MAPTBL_OPT]		= "parse_maptbl",
		[PEB_PARSE_SEGBMAP_OPT]		= "parse_segbmap",
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
		case 'o':
			env->dump_into_files = SSDFS_TRUE;
			env->output_folder = optarg;
			if (!env->output_folder) {
				print_usage();
				exit(EXIT_SUCCESS);
			}
			break;
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
					peb->id = ssdfs_parse_value("id", value,
								0, U64_MAX - 1);
					break;
				case PEB_COUNT_OPT:
					peb->pebs_count =
						ssdfs_parse_value("peb_count",
								  value, 1,
								  U64_MAX - 1);
					break;
				case PEB_SIZE_OPT:
					peb->peb_size =
						ssdfs_parse_peb_size(value);
					break;
				case PEB_LOG_INDEX_OPT:
					count = ssdfs_parse_value("log_index",
								  value, 0,
								  U32_MAX - 1);
					ssdfs_check_log_index((u32)count);
					peb->log_index = (u32)count;
					break;
				case PEB_LOG_COUNT_OPT:
					count = ssdfs_parse_value("log_count",
								  value, 0,
								  U32_MAX - 1);
					peb->logs_count = (u32)count;
					if (peb->logs_count > 1)
						peb->show_all_logs = SSDFS_TRUE;
					else
						peb->show_all_logs = SSDFS_FALSE;
					break;
				case PEB_LOG_SIZE_OPT:
					count = ssdfs_parse_value("log_size",
								  value, 1,
								  U32_MAX - 1);
					peb->log_size = (u32)count;
					break;
				case PEB_PARSE_HEADER_OPT:
					peb->parse_flags |= SSDFS_PARSE_HEADER;
					break;
				case PEB_PARSE_LOG_FOOTER_OPT:
					peb->parse_flags |= SSDFS_PARSE_LOG_FOOTER;
					break;
				case PEB_PARSE_BLOCK_BITMAP_OPT:
					peb->parse_flags |= SSDFS_PARSE_BLOCK_BITMAP;
					break;
				case PEB_PARSE_BLK2OFF_TBL_OPT:
					peb->parse_flags |= SSDFS_PARSE_BLK2OFF_TABLE;
					break;
				case PEB_PARSE_BLOCK_STATE_OPT:
					peb->parse_flags |= SSDFS_PARSE_BLOCK_STATE_AREA;
					break;
				case PEB_PARSE_MAPTBL_CACHE_OPT:
					peb->parse_flags |= SSDFS_PARSE_MAPTBL_CACHE_AREA;
					break;
				case PEB_PARSE_META_EXTENTS_OPT:
					peb->parse_flags |= SSDFS_PARSE_META_EXTENTS_AREA;
					break;
				case PEB_PARSE_MAPTBL_OPT:
					peb->parse_flags |= SSDFS_PARSE_MAPPING_TABLE;
					break;
				case PEB_PARSE_SEGBMAP_OPT:
					peb->parse_flags |= SSDFS_PARSE_SEGMENT_BITMAP;
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
				struct ssdfs_dumpfs_raw_dump_environment *raw_dump;
				char *value;

				raw_dump = &env->raw_dump;
				switch (getsubopt(&p, raw_dump_tokens, &value)) {
				case RAW_DUMP_SHOW_OPT:
					/* do nothing */
					break;
				case RAW_DUMP_OFFSET_OPT:
					raw_dump->offset =
						ssdfs_parse_value("offset",
								  value, 0,
								  U64_MAX - 1);
					break;
				case RAW_DUMP_SIZE_OPT:
					raw_dump->size =
						(u32)ssdfs_parse_value("size",
								value, 1,
								U32_MAX);
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

	if (env->command == SSDFS_RAW_DUMP_COMMAND &&
	    (env->raw_dump.offset == U64_MAX || env->raw_dump.size == 0)) {
		SSDFS_ERR("PLEASE, DEFINE RAW DUMP OFFSET AND SIZE\n");
		print_usage();
		exit(EXIT_FAILURE);
	}
}
