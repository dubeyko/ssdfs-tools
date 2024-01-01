/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/dump.ssdfs/dumpfs.c - implementation of dumpfs.ssdfs (volume dumping).
 *
 * Copyright (c) 2014-2019 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 * Copyright (c) 2014-2024 Viacheslav Dubeyko <slava@dubeyko.com>
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

#define _LARGEFILE64_SOURCE
#define __USE_FILE_OFFSET64
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <getopt.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <paths.h>
#include <blkid/blkid.h>
#include <uuid/uuid.h>
#include <limits.h>

#include "dumpfs.h"
#include <mtd/mtd-abi.h>

/* Environment description */
static struct ssdfs_dumpfs_environment environment = {
	.base.show_debug = SSDFS_FALSE,
	.base.show_info = SSDFS_TRUE,
	.base.erase_size = SSDFS_128KB,
	.base.fs_size = 0,
	.peb.id = U64_MAX,
	.peb.pebs_count = U64_MAX,
	.peb.peb_size = U32_MAX,
	.peb.show_all_logs = SSDFS_TRUE,
	.peb.log_offset = 0,
	.peb.log_size = U32_MAX,
	.peb.log_index = 0,
	.peb.logs_count = U32_MAX,
	.peb.parse_flags = 0,
	.raw_dump.offset = U64_MAX,
	.raw_dump.size = 0,
	.raw_dump.buf = NULL,
	.raw_dump.buf_size = U32_MAX,
	.command = SSDFS_DUMP_COMMAND_MAX,
	.is_raw_dump_requested = SSDFS_FALSE,
	.fd = -1,
	.stream = NULL,
	.output_folder = NULL,
	.dump_into_files = SSDFS_FALSE,
};

/************************************************************************
 *                     Base dumpfs algorithm                            *
 ************************************************************************/

static
int ssdfs_dumpfs_create_buffers(struct ssdfs_dumpfs_environment *env)
{
	if (env->is_raw_dump_requested) {
		env->raw_dump.buf_size = env->base.erase_size;

		env->raw_dump.buf = calloc(1, env->raw_dump.buf_size);
		if (!env->raw_dump.buf) {
			SSDFS_ERR("fail to allocate raw dump's buffer: "
				  "size %u, err: %s\n",
				  env->raw_dump.buf_size,
				  strerror(errno));
			return errno;
		}
	}

	return 0;
}

static
void ssdfs_dumpfs_destroy_buffers(struct ssdfs_dumpfs_environment *env)
{
	if (env->is_raw_dump_requested)
		free(env->raw_dump.buf);
}

int main(int argc, char *argv[])
{
	struct ssdfs_dumpfs_environment *env_ptr;
	int err = 0;

	env_ptr = &environment;

	parse_options(argc, argv, env_ptr);

	env_ptr->base.dev_name = argv[optind];

	SSDFS_DUMPFS_INFO(env_ptr->base.show_info,
			  "[001]\tOPEN DEVICE...\n");

	err = open_device(&env_ptr->base, 0);
	if (err)
		exit(EXIT_FAILURE);

	err = ssdfs_dumpfs_create_buffers(env_ptr);
	if (err)
		goto close_device;

	SSDFS_DUMPFS_INFO(env_ptr->base.show_info,
			  "[001]\t[SUCCESS]\n");

	if (env_ptr->peb.id >= U64_MAX)
		env_ptr->peb.id = 0;

	if (env_ptr->peb.pebs_count >= U64_MAX) {
		env_ptr->peb.pebs_count =
			env_ptr->base.fs_size / env_ptr->base.erase_size;
	}

	switch (env_ptr->command) {
	case SSDFS_DUMP_GRANULARITY_COMMAND:
		err = ssdfs_dumpfs_show_granularity(env_ptr);
		break;

	case SSDFS_DUMP_PEB_COMMAND:
		err = ssdfs_dumpfs_show_peb_dump(env_ptr);
		break;

	case SSDFS_RAW_DUMP_COMMAND:
		err = ssdfs_dumpfs_open_file(env_ptr, "raw_dump.bin");
		if (err) {
			SSDFS_ERR("fail to open output file: "
				  "err %d\n", err);
			goto destroy_buffers;
		}

		err = ssdfs_dumpfs_show_raw_dump(env_ptr);
		if (err) {
			SSDFS_ERR("fail to show raw dump: "
				  "err %d\n", err);
			goto close_opened_file;
		}

close_opened_file:
		ssdfs_dumpfs_close_file(env_ptr);
		break;

	default:
		err = -EOPNOTSUPP;
	}

destroy_buffers:
	ssdfs_dumpfs_destroy_buffers(env_ptr);

close_device:
	close(env_ptr->base.fd);
	exit(err ? EXIT_FAILURE : EXIT_SUCCESS);
}
