//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/dump.ssdfs/show_raw_dump.c - show raw dump command.
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

#include <ctype.h>

#include "dumpfs.h"

/************************************************************************
 *                       Show raw dump command                          *
 ************************************************************************/

#define SSDFS_DUMPFS_RAW_STRING_LEN	(16)

#define IS_PRINT(ptr) \
	(isprint(*(ptr)) ? *(ptr) : '.')

int ssdfs_dumpfs_show_raw_string(u64 offset, const u8 *ptr, u32 len)
{
	ssize_t printed = 0;
	int res = 0;
	int i;

	/* Show offset */
	res = printf("%08llX  ", offset);
	if (res < 0)
		return res;

	len = min_t(u32, len, SSDFS_DUMPFS_RAW_STRING_LEN);

	for (i = 0; i < SSDFS_DUMPFS_RAW_STRING_LEN; i++) {
		if (i == SSDFS_DUMPFS_RAW_STRING_LEN / 2) {
			res = printf(" ");
			if (res < 0)
				return res;
		}

		if (i >= len) {
			res = printf("   ");
			if (res < 0)
				return res;
		} else {
			res = printf("%02x ", *(ptr + i));
			if (res < 0)
				return res;
		}

		printed++;
	}

	res = printf(" |");
	if (res < 0)
		return res;

	for (i = 0; i < SSDFS_DUMPFS_RAW_STRING_LEN; i++) {
		if (i >= len) {
			res = printf(" ");
			if (res < 0)
				return res;
		} else {
			res = printf("%c", IS_PRINT(ptr + i));
			if (res < 0)
				return res;
		}
	}

	res = printf("|\n");
	if (res < 0)
		return res;

	return printed;
}

int ssdfs_dumpfs_show_raw_dump(struct ssdfs_dumpfs_environment *env)
{
	u64 offset;
	u32 len;
	u32 read_bytes = 0, displayed_bytes = 0;
	int err = 0, res = 0;

	SSDFS_DBG(env->base.show_debug,
		  "command %#x, offset %llu, len %u\n",
		  env->command,
		  env->raw_dump.offset,
		  env->raw_dump.size);

	offset = env->raw_dump.offset;
	len = env->raw_dump.size;

	if (!env->raw_dump.buf) {
		SSDFS_ERR("empty buffer\n");
		return -ENOSPC;
	}

	if (offset >= env->base.fs_size) {
		SSDFS_ERR("offset %llu >= fs_size %llu\n",
			  offset, env->base.fs_size);
		return -EINVAL;
	}

	len = (u32)min_t(u64, (u64)len, env->base.fs_size - offset);

	do {
		u32 size = min_t(u32, env->raw_dump.buf_size, len - read_bytes);

		err = env->base.dev_ops->read(env->base.fd, offset, size,
					      env->raw_dump.buf);
		if (err) {
			SSDFS_ERR("fail to read dump: "
				  "offset %llu, size %u, err %d\n",
				  offset, size, err);
			return err;
		}

		read_bytes += size;
		displayed_bytes = 0;

		do {
			u8 *ptr = env->raw_dump.buf + displayed_bytes;

			res = ssdfs_dumpfs_show_raw_string(offset, ptr,
							   size - displayed_bytes);
			if (res < 0) {
				SSDFS_ERR("fail to show raw dump's string: "
					  "offset %llu, err %d\n",
					  offset, res);
				return res;
			}

			offset += res;
			displayed_bytes += res;
		} while (displayed_bytes < size);
	} while (read_bytes < len);

	return 0;
}
