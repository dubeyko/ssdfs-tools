/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * lib/mtd_readwrite.c - MTD read/write operations.
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

#include <sys/ioctl.h>
#include <mtd/mtd-abi.h>

#include "ssdfs_tools.h"

/************************************************************************
 *                       Write/Erase operations                         *
 ************************************************************************/

int mtd_read(int fd, u64 offset, size_t size, void *buf, int is_debug)
{
	return ssdfs_pread(fd, offset, size, buf);
}

int mtd_write(int fd, struct ssdfs_nand_geometry *info,
		u64 offset, size_t size, void *buf,
		u32 *open_zones, int is_debug)
{
	return ssdfs_pwrite(fd, offset, size, buf);
}

int mtd_erase(int fd, u64 offset, size_t size,
		void *buf, size_t buf_size, int is_debug)
{
	if (offset >= 0x100000000ull) {
		struct erase_info_user64 ei;

		ei.start = offset;
		ei.length = size;

		return ioctl(fd, MEMERASE64, &ei);
	} else {
		struct erase_info_user ei;

		ei.start = offset;
		ei.length = size;

		return ioctl(fd, MEMERASE, &ei);
	}
}

int mtd_check_nand_geometry(int fd, struct ssdfs_nand_geometry *info,
			    int is_debug)
{
	struct mtd_info_user meminfo;
	int err;

	err = ioctl(fd, MEMGETINFO, &meminfo);
	if (err != 0) {
		SSDFS_ERR("fail to get MTD characteristics info: err %d\n",
			  err);
		return err;
	}

	if (meminfo.erasesize != info->erasesize) {
		SSDFS_ERR("meminfo.erasesize %u != erasesize %u\n",
			  meminfo.erasesize, info->erasesize);
		return -EINVAL;
	}

	if (meminfo.writesize != info->writesize) {
		SSDFS_ERR("meminfo.writesize %u != writesize %u\n",
			  meminfo.writesize, info->writesize);
		return -EINVAL;
	}

	return 0;
}

int mtd_check_peb(int fd, u64 offset, u32 erasesize,
		  int need_close_zone, int is_debug)
{
	int res;

	if ((res = ioctl(fd, MEMGETBADBLOCK, &offset)) < 0) {
		SSDFS_ERR("fail to check PEB: offset %llu, err %d\n",
			  offset, res);
		return res;
	}

	if (res > 0)
		return SSDFS_PEB_IS_BAD;

	res = mtd_erase(fd, offset, (size_t)erasesize, NULL, 0, SSDFS_FALSE);
	if (res != 0)
		return SSDFS_RECOVERING_PEB;

	return SSDFS_PEB_ERASURE_OK;
}
