//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * lib/zns_readwrite.c - ZNS read/write operations.
 *
 * Copyright (c) 2022 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#include <linux/fs.h>
#include <linux/blkzoned.h>

#include "ssdfs_tools.h"

/************************************************************************
 *                       Write/Erase operations                         *
 ************************************************************************/

int zns_read(int fd, u64 offset, size_t size, void *buf)
{
	return ssdfs_pread(fd, offset, size, buf);
}

int zns_write(int fd, struct ssdfs_nand_geometry *info,
	      u64 offset, size_t size, void *buf)
{
	struct blk_zone_range range;
	u64 zone_start = (offset / info->erasesize) * info->erasesize;

	if (zone_start == offset) {
		range.sector = zone_start / SSDFS_512B;
		range.nr_sectors = info->erasesize / SSDFS_512B;

		if (ioctl(fd, BLKOPENZONE, &range) < 0) {
			SSDFS_ERR("fail to open zone "
				  "range (start %llu, sectors %llu): %s\n",
				  range.sector, range.nr_sectors,
				  strerror(errno));
			return -EIO;
		}
	}

	if ((zone_start + info->erasesize) < (offset + size)) {
		SSDFS_ERR("invalid write request: "
			  "zone (start %llu, erasesize %u), "
			  "request (offset %llu, size %zu)\n",
			  zone_start, info->erasesize,
			  offset, size);
		return -ERANGE;
	}

	return ssdfs_pwrite(fd, offset, size, buf);
}

int zns_erase(int fd, u64 offset, size_t size, void *buf)
{
	struct blk_zone_range range;

	range.sector = offset / SSDFS_512B;
	range.nr_sectors = (size + SSDFS_512B - 1) / SSDFS_512B;

	if (ioctl(fd, BLKRESETZONE, &range) < 0) {
		SSDFS_ERR("fail to reset zone (offset %llu, size %zu): %s\n",
			  offset, size, strerror(errno));
		return -EIO;
	}

	return 0;
}

int zns_check_nand_geometry(int fd, struct ssdfs_nand_geometry *info)
{
	u32 sectors_per_zone = 0;
	u64 zone_size;
	int res = 0;

	res = ioctl(fd, BLKGETZONESZ, &sectors_per_zone);
	if (res < 0) {
		if (errno == ENOTTY || errno == EINVAL) {
			/*
			 * No kernel support, assuming non-zoned device.
			 */
			SSDFS_ERR("no kernel support for ZNS device\n");
		} else {
			SSDFS_ERR("fail to retrieve zone size: %s\n",
				  strerror(errno));
		}

		return -ERANGE;
	}

	if (sectors_per_zone == 0) {
		SSDFS_ERR("unexpected value: sectors_per_zone %u\n",
			  sectors_per_zone);
		return -ERANGE;
	}

	zone_size = sectors_per_zone * SSDFS_512B;

	if (zone_size >= U32_MAX) {
		SSDFS_ERR("unsupported zone size %llu\n",
			  zone_size);
		return -EOPNOTSUPP;
	}

	if (zone_size != info->erasesize) {
		res = -ENOENT;
		info->erasesize = (u32)zone_size;
	}

	if (zone_size % info->writesize) {
		res = -ENOENT;

		if (info->writesize <= SSDFS_4KB)
			info->writesize = SSDFS_4KB;
		else if (info->writesize <= SSDFS_8KB)
			info->writesize = SSDFS_8KB;
		else if (info->writesize <= SSDFS_16KB)
			info->writesize = SSDFS_16KB;
		else
			info->writesize = SSDFS_32KB;
	}

	return res;
}

int zns_check_peb(int fd, u64 offset, u32 erasesize)
{
	return -EOPNOTSUPP;
}
