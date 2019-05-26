//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * lib/blk_readwrite.c - block layer read/write operations.
 *
 * Copyright (c) 2014-2018 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 *
 * HGST Confidential
 * (C) Copyright 2009-2018, HGST, Inc., All rights reserved.
 *
 * Created by HGST, San Jose Research Center, Storage Architecture Group
 * Authors: Vyacheslav Dubeyko <Vyacheslav.Dubeyko@wdc.com>
 *
 * Acknowledgement: Cyril Guyot <Cyril.Guyot@wdc.com>
 *                  Zvonimir Bandic <Zvonimir.Bandic@wdc.com>
 */

#include "ssdfs_utils.h"

/************************************************************************
 *                       Write/Erase operations                         *
 ************************************************************************/

int bdev_read(int fd, u64 offset, size_t size, void *buf)
{
	return ssdfs_pread(fd, offset, size, buf);
}

int bdev_write(int fd, u64 offset, size_t size, void *buf)
{
	return ssdfs_pwrite(fd, offset, size, buf);
}

int bdev_erase(int fd, u64 offset, size_t size, void *buf)
{
	return ssdfs_pwrite(fd, offset, size, buf);
}

int bdev_check_nand_geometry(int fd, u32 erasesize, u32 writesize)
{
	return -EOPNOTSUPP;
}

int bdev_check_peb(int fd, u64 offset, u32 erasesize)
{
	return -EOPNOTSUPP;
}
