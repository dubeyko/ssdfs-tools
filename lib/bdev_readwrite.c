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

int bdev_read(int fd, u64 offset, size_t size, void *buf, int is_debug)
{
	return ssdfs_pread(fd, offset, size, buf);
}

int bdev_write(int fd, struct ssdfs_nand_geometry *info,
		u64 offset, size_t size, void *buf,
		u32 *open_zones, int is_debug)
{
	return ssdfs_pwrite(fd, offset, size, buf);
}

int bdev_erase(int fd, u64 offset, size_t size,
		void *buf, size_t buf_size, int is_debug)
{
	u64 range[2] = {offset, size};
	u64 erased_bytes = 0;
	int err;

	if (ioctl(fd, BLKSECDISCARD, &range) < 0) {
		SSDFS_DBG(is_debug,
			  "BLKSECDISCARD is not supported: "
			  "offset %llu, size %zu\n",
			   offset, size);

		if (ioctl(fd, BLKZEROOUT, &range) < 0) {
			SSDFS_DBG(is_debug,
				  "BLKZEROOUT is not supported: "
				  "trying write: offset %llu, size %zu\n",
				   offset, size);

			do {
				err = ssdfs_pwrite(fd,
						   offset + erased_bytes,
						   buf_size, buf);
				if (err) {
					SSDFS_ERR("fail to erase: "
						  "offset %llu, "
						  "erased_bytes %llu, "
						  "size %zu, "
						  "buf_size %zu, "
						  "err %d\n",
						  offset, erased_bytes,
						  size, buf_size, err);
					return err;
				}

				erased_bytes += buf_size;
			} while (erased_bytes < size);
		}
	}

	return 0;
}

int bdev_check_nand_geometry(int fd, struct ssdfs_nand_geometry *info,
			     int is_debug)
{
	return -EOPNOTSUPP;
}

int bdev_check_peb(int fd, u64 offset, u32 erasesize,
		   int need_close_zone, int is_debug)
{
	return -EOPNOTSUPP;
}
