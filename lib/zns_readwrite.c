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

int zns_read(int fd, u64 offset, size_t size, void *buf, int is_debug)
{
	return ssdfs_pread(fd, offset, size, buf);
}

int zns_write(int fd, struct ssdfs_nand_geometry *info,
	      u64 offset, size_t size, void *buf,
	      u32 *open_zones, int is_debug)
{
	struct blk_zone_range range;
	u64 zone_start = (offset / info->erasesize) * info->erasesize;

	SSDFS_DBG(is_debug,
		  "trying write: offset %llu, size %zu, "
		  "zone_start %llu, erasesize %u\n",
		   offset, size, zone_start, info->erasesize);

	if (zone_start == offset || offset == SSDFS_RESERVED_VBR_SIZE) {
		range.sector = zone_start / SSDFS_512B;
		range.nr_sectors = info->erasesize / SSDFS_512B;

		SSDFS_DBG(is_debug,
			  "open zone: offset %llu, zone_start %llu, "
			  "range (sector %llu, nr_sectors %llu)\n",
			   offset, zone_start,
			   (u64)range.sector, range.nr_sectors);

		if (ioctl(fd, BLKOPENZONE, &range) < 0) {
			SSDFS_ERR("fail to open zone "
				  "range (start %llu, sectors %llu): %s\n",
				  range.sector, range.nr_sectors,
				  strerror(errno));
			return -EIO;
		}

		(*open_zones)++;

		SSDFS_DBG(is_debug,
			  "open_zones %u\n", *open_zones);
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

int zns_erase(int fd, u64 offset, size_t size,
		void *buf, size_t buf_size, int is_debug)
{
	struct blk_zone_range range;

	range.sector = offset / SSDFS_512B;
	range.nr_sectors = (size + SSDFS_512B - 1) / SSDFS_512B;

	SSDFS_DBG(is_debug,
		  "erase zone: offset %llu, size %zu, "
		  "range (sector %llu, nr_sectors %llu)\n",
		   offset, size,
		   range.sector, range.nr_sectors);

	if (ioctl(fd, BLKRESETZONE, &range) < 0) {
		SSDFS_ERR("fail to reset zone (offset %llu, size %zu): %s\n",
			  offset, size, strerror(errno));
		return -EIO;
	}

	return 0;
}

int zns_check_nand_geometry(int fd, struct ssdfs_nand_geometry *info,
			    int is_debug)
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

	SSDFS_DBG(is_debug,
		  "sectors_per_zone %u, zone_size %llu\n",
		   sectors_per_zone, zone_size);

	return res;
}

int zns_check_peb(int fd, u64 offset, u32 erasesize,
		  int need_close_zone, int is_debug)
{
	struct blk_zone_report *report;
	struct blk_zone *zone;
	struct blk_zone_range range;
	u64 zone_start;
	void *buf;
	size_t buf_size = sizeof(struct blk_zone_report) +
				sizeof(struct blk_zone);
	int res;

	if (is_debug) {
		zone_start = (offset / erasesize) * erasesize;

		buf = calloc(1, buf_size);
		if (!buf) {
			SSDFS_ERR("fail to allocate buffer\n");
			return -ENOMEM;
		}

		report = (struct blk_zone_report *)buf;
		report->sector = zone_start / SSDFS_512B;
		report->nr_zones = 1;

		res = ioctl(fd, BLKREPORTZONE, report);
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
		} else {
			if (!report->nr_zones) {
				SSDFS_ERR("zone report contains nothing\n");
			} else {
				zone = (struct blk_zone *)(report + 1);

				SSDFS_DBG(is_debug,
					  "zone: start %llu, len %llu, wp %llu, "
					  "type %#x, cond %#x, non_seq %#x, "
					  "reset %#x, capacity %llu\n",
					  zone->start, zone->len, zone->wp,
					  zone->type, zone->cond, zone->non_seq,
					  zone->reset, zone->capacity);
			}
		}

		free(buf);
	}

	if (need_close_zone) {
		range.sector = offset / SSDFS_512B;
		range.nr_sectors = (erasesize + SSDFS_512B - 1) / SSDFS_512B;

		if (ioctl(fd, BLKFINISHZONE, &range) < 0) {
			SSDFS_ERR("fail to finish zone (offset %llu, size %u): %s\n",
				  offset, erasesize, strerror(errno));
			return -EIO;
		}
	}

	return 0;
}
