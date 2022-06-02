//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * lib/blk_readwrite.c - block layer read/write operations.
 *
 * Copyright (c) 2014-2022 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 *
 * HGST Confidential
 * (C) Copyright 2014-2022, HGST, Inc., All rights reserved.
 *
 * Created by HGST, San Jose Research Center, Storage Architecture Group
 * Authors: Vyacheslav Dubeyko <slava@dubeyko.com>
 *
 * Acknowledgement: Cyril Guyot <Cyril.Guyot@wdc.com>
 *                  Zvonimir Bandic <Zvonimir.Bandic@wdc.com>
 */

#include <linux/fs.h>

#include "ssdfs_tools.h"

/************************************************************************
 *                       Write/Erase operations                         *
 ************************************************************************/

int bdev_read(int fd, u64 offset, size_t size, void *buf)
{
	return ssdfs_pread(fd, offset, size, buf);
}

int bdev_write(int fd, struct ssdfs_nand_geometry *info,
		u64 offset, size_t size, void *buf)
{
	return ssdfs_pwrite(fd, offset, size, buf);
}

int bdev_erase(int fd, u64 offset, size_t size, void *buf)
{
	u64 range[2] = {offset, size};

	if (ioctl(fd, BLKDISCARD, &range) < 0) {
		SSDFS_INFO("BLKDISCARD is not supported: "
			   "trying write: offset %llu, size %zu\n",
			   offset, size);
		return ssdfs_pwrite(fd, offset, size, buf);
	}

	return 0;
}

int bdev_check_nand_geometry(int fd, struct ssdfs_nand_geometry *info)
{
	return -EOPNOTSUPP;
}

int bdev_check_peb(int fd, u64 offset, u32 erasesize)
{
	return -EOPNOTSUPP;
}
