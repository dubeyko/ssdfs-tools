//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * lib/ssdfs_common.c - common usefull functionality.
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

#define _LARGEFILE64_SOURCE
#define __USE_FILE_OFFSET64
#define _GNU_SOURCE
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>
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
#include <time.h>
#include <zlib.h>
#include <mtd/mtd-abi.h>
#include <linux/blkzoned.h>

#include "ssdfs_tools.h"

const char *uuid_string(const unsigned char *uuid)
{
	static char buf[256];

	sprintf(buf, "%02x%02x%02x%02x-%02x%02x-%02x%02x-" \
		"%02x%02x-%02x%02x%02x%02x%02x%02x", uuid[0], uuid[1],
		uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7],
		uuid[8], uuid[9], uuid[10], uuid[11], uuid[12],
		uuid[13], uuid[14], uuid[15]);
	return buf;
}

__le32 ssdfs_crc32_le(void *data, size_t len)
{
	return cpu_to_le32(~crc32(0, data, len));
}

int ssdfs_calculate_csum(struct ssdfs_metadata_check *check,
			  void *buf, size_t buf_size)
{
	u16 bytes;
	u16 flags;

	bytes = le16_to_cpu(check->bytes);
	flags = le16_to_cpu(check->flags);

	if (bytes > buf_size) {
		SSDFS_ERR("corrupted size %d of checked data\n", bytes);
		return -EINVAL;
	}

	if (flags & SSDFS_CRC32) {
		check->csum = 0;
		check->csum = ssdfs_crc32_le(buf, bytes);
	} else {
		SSDFS_ERR("unknown flags set %#x\n", flags);
		return -EINVAL;
	}

	return 0;
}

int is_csum_valid(struct ssdfs_metadata_check *check,
		   void *buf, size_t buf_size)
{
	__le32 old_csum;
	__le32 calc_csum;
	int err;

	old_csum = check->csum;

	err = ssdfs_calculate_csum(check, buf, buf_size);
	if (err) {
		SSDFS_ERR("fail to calculate checksum\n");
		return SSDFS_FALSE;
	}

	calc_csum = check->csum;
	check->csum = old_csum;

	if (old_csum != calc_csum) {
		SSDFS_ERR("old_csum %#x != calc_csum %#x\n",
			  le32_to_cpu(old_csum),
			  le32_to_cpu(calc_csum));
		return SSDFS_FALSE;
	}

	return SSDFS_TRUE;
}

#define BILLION		1000000000L

u64 ssdfs_current_time_in_nanoseconds(void)
{
	time_t ctime;
	u64 nanoseconds;

	ctime = time(NULL);
	nanoseconds = (u64)ctime * BILLION;
	return nanoseconds;
}

char *ssdfs_nanoseconds_to_time(u64 nanoseconds)
{
	time_t time = (time_t)(nanoseconds / BILLION);
	return ctime(&time);
}

int ssdfs_pwrite(int fd, u64 offset, size_t size, void *buf)
{
	ssize_t ret;
	size_t rest;

	rest = size;
	while (rest > 0) {
		ret = pwrite(fd, buf, rest, offset);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			SSDFS_ERR("write failed: %s\n", strerror(errno));
			return errno;
		}

		rest -= ret;
		offset += ret;
		buf += ret;
	}
	return 0;
}

int ssdfs_pread(int fd, u64 offset, size_t size, void *buf)
{
	ssize_t ret;
	size_t rest;

	rest = size;
	while (rest > 0) {
		ret = pread(fd, buf, rest, offset);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			SSDFS_ERR("read failed: %s\n", strerror(errno));
			return errno;
		}

		rest -= ret;
		offset += ret;
		buf += ret;
	}
	return 0;
}

int is_zoned_device(int fd)
{
	struct stat stat;
	int is_zoned = SSDFS_FALSE;
	u32 sectors_per_zone;
	int res;
	int err;

	if (fd < -1) {
		SSDFS_ERR("invalid file descriptor %d\n",
			  fd);
		return -EINVAL;
	}

	err = fstat(fd, &stat);
	if (err) {
		SSDFS_ERR("unable to get file status: %s\n",
			  strerror(errno));
		return errno;
	}

	switch (stat.st_mode & S_IFMT) {
	case S_IFBLK:
		res = ioctl(fd, BLKGETZONESZ, &sectors_per_zone);
		if (res < 0) {
			if (errno == ENOTTY || errno == EINVAL) {
				/*
				 * No kernel support, assuming non-zoned device.
				 */
			} else {
				SSDFS_ERR("fail to retrieve zone size: %s\n",
					  strerror(errno));
			}

			is_zoned = SSDFS_FALSE;
		} else {
			if (sectors_per_zone == 0)
				is_zoned = SSDFS_FALSE;
			else
				is_zoned = SSDFS_TRUE;
		}
		break;

	default:
		/* do nothing */
		break;
	}

	return is_zoned;
}

int open_device(struct ssdfs_environment *env, u32 flags)
{
	struct mtd_info_user mtd;
	struct stat stat;
	u32 default_flags = O_RDWR | O_LARGEFILE;
	int err;

	SSDFS_DBG(env->show_debug, "dev_name %s, flags %#x\n",
		  env->dev_name, flags);

	flags = default_flags | flags;

	env->fd = open(env->dev_name, flags);
	if (env->fd == -1) {
		SSDFS_ERR("unable to open %s: %s\n",
			  env->dev_name, strerror(errno));
		return errno;
	}

	err = fstat(env->fd, &stat);
	if (err) {
		SSDFS_ERR("unable to get file status %s: %s\n",
			  env->dev_name, strerror(errno));
		return errno;
	}

	switch (stat.st_mode & S_IFMT) {
	case S_IFSOCK:
	case S_IFLNK:
	case S_IFDIR:
	case S_IFIFO:
		SSDFS_ERR("device %s has invalid type\n",
			  env->dev_name);
		return -EOPNOTSUPP;

	case S_IFCHR:
		/* mtd device type */
		if (major(stat.st_rdev) != SSDFS_MTD_MAJOR_DEV) {
			SSDFS_ERR("non-mtd character device number %u\n",
				  major(stat.st_rdev));
			return -EOPNOTSUPP;
		}

		err = ioctl(env->fd, MEMGETINFO, &mtd);
		if (err) {
			SSDFS_ERR("mtd ioctl failed for %s: %s\n",
				  env->dev_name, strerror(errno));
			return errno;
		}

		env->erase_size = mtd.erasesize;
		/* check erase size */
		{
			int shift = ffs(mtd.erasesize) - 1;
			if (mtd.erasesize != 1 << shift) {
				SSDFS_ERR("erasesize must be a power of 2\n");
				return -EINVAL;
			}
		}
		/* check writesize */
		{
			int shift = ffs(mtd.writesize) - 1;
			if (mtd.writesize != 1 << shift) {
				SSDFS_ERR("writesize must be a power of 2\n");
				return -EINVAL;
			}
		}

		env->fs_size = mtd.size;
		env->dev_ops = &mtd_ops;
		env->device_type = SSDFS_MTD_DEVICE;
		break;

	case S_IFREG:
		/* regular file */
		env->fs_size = stat.st_size;
		env->dev_ops = &bdev_ops;
		env->device_type = SSDFS_BLK_DEVICE;
		break;

	case S_IFBLK:
		err = ioctl(env->fd, BLKGETSIZE64, &env->fs_size);
		if (err) {
			SSDFS_ERR("block ioctl failed for %s: %s\n",
				  env->dev_name, strerror(errno));
			return errno;
		}

		if (is_zoned_device(env->fd)) {
			/* ZNS device */
			env->dev_ops = &zns_ops;
			env->device_type = SSDFS_ZNS_DEVICE;
		} else {
			/* block device type */
			env->dev_ops = &bdev_ops;
			env->device_type = SSDFS_BLK_DEVICE;
		}
		break;

	default:
		SSDFS_ERR("device %s has unknown type\n",
			  env->dev_name);
		BUG();
	}

	return 0;
}
