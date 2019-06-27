//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/dump.ssdfs/mkfs.h - declarations of dump.ssdfs utility.
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

#ifndef _SSDFS_UTILS_DUMPFS_H
#define _SSDFS_UTILS_DUMPFS_H

#ifdef dumpfs_fmt
#undef dumpfs_fmt
#endif

#include "version.h"

#define dumpfs_fmt(fmt) "dump.ssdfs: " SSDFS_UTILS_VERSION ": " fmt

#include "ssdfs_tools.h"

#define SSDFS_DUMPFS_INFO(show, fmt, ...) \
	do { \
		if (show) { \
			fprintf(stdout, dumpfs_fmt(fmt), ##__VA_ARGS__); \
		} \
	} while (0)

enum SSDFS_DUMPFS_COMMANDS {
	SSDFS_DUMP_GRANULARITY_COMMAND,
	SSDFS_DUMP_PEB_COMMAND,
	SSDFS_RAW_DUMP_COMMAND,
	SSDFS_DUMP_COMMAND_MAX
};

enum SSDFS_DUMPFS_PARSE_FLAGS {
	SSDFS_PARSE_HEADER,
	SSDFS_PARSE_FLAGS_MAX
};

#define SSDFS_PARSE_ALL_MASK \
	(SSDFS_PARSE_HEADER)

/*
 * struct ssdfs_peb_dump_environment - PEB dump environment
 * @id: PEB's identification number
 * @size: PEB size in bytes
 * @show_all_logs: should all logs be shown?
 * @log_index: index of extracting log
 * @log_size: full log's size in bytes
 * @parse_flags: what should be parsed?
 */
struct ssdfs_peb_dump_environment {
	u64 id;
	u32 size;

	int show_all_logs;
	u16 log_index;
	u32 log_size;

	u32 parse_flags;
};

/*
 * struct ssdfs_raw_dump_environment - raw dump environment
 * @offset: offset in bytes
 * @size: size of raw dump in bytes
 * @buf: pointer on buffer
 * @buf_size: size of buffer in bytes
 */
struct ssdfs_raw_dump_environment {
	u64 offset;
	u32 size;

	u8 *buf;
	u32 buf_size;
};

/*
 * struct ssdfs_dumpfs_environment - dumpfs environment
 * @base: basic environment
 * @peb: PEB dump environment
 * @raw_dump: raw dump environment
 * @command: concrete dumpfs execution command
 * @is_raw_dump_requested: is raw dump requested?
 */
struct ssdfs_dumpfs_environment {
	struct ssdfs_environment base;
	struct ssdfs_peb_dump_environment peb;
	struct ssdfs_raw_dump_environment raw_dump;

	int command;
	int is_raw_dump_requested;
};

/* common.c */
int ssdfs_dumpfs_read_segment_header(struct ssdfs_dumpfs_environment *env,
				     u64 peb_id, u32 peb_size,
				     int log_index, u32 log_size,
				     struct ssdfs_segment_header *hdr);
int ssdfs_dumpfs_read_block_bitmap(struct ssdfs_dumpfs_environment *env,
				   u64 peb_id, u32 peb_size,
				   int log_index, u32 log_size,
				   u32 area_offset, u32 size,
				   void *buf);
int ssdfs_dumpfs_read_blk2off_table(struct ssdfs_dumpfs_environment *env,
				   u64 peb_id, u32 peb_size,
				   int log_index, u32 log_size,
				   u32 area_offset, u32 size,
				   void *buf);
int ssdfs_dumpfs_find_any_valid_peb(struct ssdfs_dumpfs_environment *env,
				    struct ssdfs_segment_header *hdr);
void ssdfs_dumpfs_show_key_volume_details(struct ssdfs_dumpfs_environment *env,
					  struct ssdfs_segment_header *hdr);

/* options.c */
void print_version(void);
void print_usage(void);
void parse_options(int argc, char *argv[],
		   struct ssdfs_dumpfs_environment *env);

/* show_granularity.c */
int ssdfs_dumpfs_show_granularity(struct ssdfs_dumpfs_environment *env);

/* show_peb_dump.c */
int ssdfs_dumpfs_show_peb_dump(struct ssdfs_dumpfs_environment *env);

/* show_raw_dump.c */
int ssdfs_dumpfs_show_raw_string(u64 offset, const u8 *ptr, u32 len);
int ssdfs_dumpfs_show_raw_dump(struct ssdfs_dumpfs_environment *env);

#endif /* _SSDFS_UTILS_DUMPFS_H */
