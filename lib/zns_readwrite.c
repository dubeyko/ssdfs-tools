/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * lib/zns_readwrite.c - ZNS read/write operations.
 *
 * Copyright (c) 2022-2026 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#include <linux/fs.h>
#include <linux/blkzoned.h>

#include "ssdfs_tools.h"

/************************************************************************
 *                        Zone details                                  *
 ************************************************************************/

/*
 * zns_report_zone() - retrieve descriptor of the zone
 * @fd: file descriptor
 * @offset: offset in bytes inside of the zone
 * @zone: zone descriptor [out]
 * @is_debug: show debug output
 *
 * A zoned device can contain conventional zones (for example,
 * SMR HDD keeps some number of conventional zones at the beginning
 * of the device). Conventional zone has no write pointer and it can be
 * written randomly. It needs to use the regular block device logic
 * for conventional zones and the ZNS logic for sequential zones.
 *
 * RETURN:
 * [success]
 * [failure] - error code:
 *
 * %-ENOMEM     - fail to allocate memory.
 * %-EIO        - fail to retrieve the zone's descriptor.
 * %-ERANGE     - no zone for requested offset.
 */
static int zns_report_zone(int fd, u64 offset, struct blk_zone *zone,
			   int is_debug)
{
	struct blk_zone_report *report;
	size_t buf_size = sizeof(struct blk_zone_report) +
				sizeof(struct blk_zone);
	int err = 0;

	report = calloc(1, buf_size);
	if (!report) {
		SSDFS_ERR("fail to allocate buffer\n");
		return -ENOMEM;
	}

	report->sector = offset / SSDFS_512B;
	report->nr_zones = 1;

	if (ioctl(fd, BLKREPORTZONE, report) < 0) {
		SSDFS_ERR("fail to report zone: offset %llu: %s\n",
			  offset, strerror(errno));
		err = -EIO;
		goto free_buffer;
	}

	if (report->nr_zones == 0) {
		SSDFS_ERR("zone report contains nothing: offset %llu\n",
			  offset);
		err = -ERANGE;
		goto free_buffer;
	}

	*zone = *(struct blk_zone *)(report + 1);

	SSDFS_DBG(is_debug,
		  "zone: start %llu, len %llu, wp %llu, "
		  "type %#x, cond %#x, non_seq %#x, "
		  "reset %#x, capacity %llu\n",
		  zone->start, zone->len, zone->wp,
		  zone->type, zone->cond, zone->non_seq,
		  zone->reset, zone->capacity);

free_buffer:
	free(report);
	return err;
}

static inline
int zns_is_conventional_zone(struct blk_zone *zone)
{
	return zone->type == BLK_ZONE_TYPE_CONVENTIONAL;
}

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
	struct blk_zone zone;
	u64 zone_start = (offset / info->erasesize) * info->erasesize;
	int err;

	SSDFS_DBG(is_debug,
		  "trying write: offset %llu, size %zu, "
		  "zone_start %llu, erasesize %u\n",
		   offset, size, zone_start, info->erasesize);

	if (zone_start == offset || offset == SSDFS_RESERVED_VBR_SIZE) {
		err = zns_report_zone(fd, zone_start, &zone, is_debug);
		if (err)
			return err;

		if (zns_is_conventional_zone(&zone)) {
			SSDFS_DBG(is_debug,
				  "conventional zone: no need to open: "
				  "offset %llu, zone_start %llu\n",
				  offset, zone_start);
			goto check_write_request;
		}

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

check_write_request:
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

/*
 * zns_erase() - erase range of zoned device
 *
 * The range can include conventional and sequential zones.
 * Conventional zones are erased by means of regular block device
 * logic. Sequential zones are erased by means of zone reset.
 */
int zns_erase(int fd, u64 offset, size_t size,
		void *buf, size_t buf_size, int is_debug)
{
	struct blk_zone_range range;
	struct blk_zone zone;
	u64 end = offset + size;
	u64 cur = offset;
	u64 zone_end;
	u64 len;
	int err;

	while (cur < end) {
		err = zns_report_zone(fd, cur, &zone, is_debug);
		if (err)
			return err;

		zone_end = (zone.start + zone.len) * SSDFS_512B;
		if (zone.len == 0 || zone_end <= cur) {
			SSDFS_ERR("invalid zone: offset %llu, "
				  "zone (start %llu, len %llu)\n",
				  cur, zone.start, zone.len);
			return -ERANGE;
		}

		len = min_t(u64, zone_end, end) - cur;

		if (zns_is_conventional_zone(&zone)) {
			SSDFS_DBG(is_debug,
				  "erase conventional zone: "
				  "offset %llu, len %llu\n",
				  cur, len);

			err = bdev_erase(fd, cur, len, buf, buf_size,
					 is_debug);
			if (err) {
				SSDFS_ERR("fail to erase conventional zone "
					  "(offset %llu, len %llu): err %d\n",
					  cur, len, err);
				return err;
			}
		} else {
			range.sector = cur / SSDFS_512B;
			range.nr_sectors = (len + SSDFS_512B - 1) / SSDFS_512B;

			SSDFS_DBG(is_debug,
				  "erase zone: offset %llu, len %llu, "
				  "range (sector %llu, nr_sectors %llu)\n",
				   cur, len,
				   range.sector, range.nr_sectors);

			if (ioctl(fd, BLKRESETZONE, &range) < 0) {
				SSDFS_ERR("fail to reset zone "
					  "(offset %llu, len %llu): %s\n",
					  cur, len, strerror(errno));
				return -EIO;
			}
		}

		cur += len;
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
	struct blk_zone zone;
	struct blk_zone_range range;
	u64 zone_start = (offset / erasesize) * erasesize;
	int err;

	if (!is_debug && !need_close_zone)
		return 0;

	err = zns_report_zone(fd, zone_start, &zone, is_debug);
	if (err)
		return err;

	if (need_close_zone && zns_is_conventional_zone(&zone)) {
		SSDFS_DBG(is_debug,
			  "conventional zone: no need to finish: "
			  "offset %llu\n", offset);
		return 0;
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

/*
 * zns_is_conventional_peb() - check that PEB is a conventional zone
 * @fd: file descriptor
 * @offset: offset in bytes inside of the PEB (zone)
 * @is_debug: show debug output
 *
 * Conventional zone has no write pointer and it doesn't consume
 * the open zones resource of zoned device.
 *
 * RETURN:
 * [SSDFS_TRUE]  - PEB is a conventional zone.
 * [SSDFS_FALSE] - PEB is a sequential zone.
 * [failure]     - negative error code.
 */
int zns_is_conventional_peb(int fd, u64 offset, int is_debug)
{
	struct blk_zone zone;
	int err;

	err = zns_report_zone(fd, offset, &zone, is_debug);
	if (err)
		return err;

	return zns_is_conventional_zone(&zone) ? SSDFS_TRUE : SSDFS_FALSE;
}
