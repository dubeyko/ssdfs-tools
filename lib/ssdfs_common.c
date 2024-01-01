/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * lib/ssdfs_common.c - common usefull functionality.
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

void ssdfs_nanoseconds_to_localtime(u64 nanoseconds,
				    struct tm *local_time)
{
	time_t time = (time_t)(nanoseconds / BILLION);
	localtime_r(&time, local_time);
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

int ssdfs_create_raw_buffer(struct ssdfs_raw_buffer *buf,
			    size_t buf_size)
{
	if (!buf) {
		SSDFS_ERR("empty pointer on buffer\n");
		return -EINVAL;
	}

	if (buf->ptr == NULL) {
		if (buf_size == 0) {
			buf->ptr = NULL;
			buf->size = 0;
		} else {
			buf->ptr = calloc(1, buf_size);
			if (!buf->ptr) {
				SSDFS_ERR("fail to allocate buffer: "
					  "buf_size %zu, err: %s\n",
					  buf_size, strerror(errno));
				return errno;
			}

			buf->size = buf_size;
		}
	} else {
		if (buf_size == 0) {
			free(buf->ptr);
			buf->ptr = NULL;
			buf->size = 0;
		} else if (buf->size < buf_size) {
			buf->ptr = realloc(buf->ptr, buf_size);
			if (!buf->ptr) {
				SSDFS_ERR("fail to re-allocate buffer: "
					  "size %zu, err: %s\n",
					  buf_size,
					  strerror(errno));
				return errno;
			}
			buf->size = buf_size;

			memset(buf->ptr, 0, buf->size);
		}
	}

	return 0;
}

int ssdfs_create_raw_area(struct ssdfs_raw_area *area,
			  u64 offset, u32 size)
{
	if (!area) {
		SSDFS_ERR("empty pointer on area\n");
		return -EINVAL;
	}

	area->offset = offset;
	area->size = size;

	SSDFS_CREATE_CONTENT_ITERATOR(SSDFS_CONTENT_ITER(area));
	memset(&area->content.metadata, 0, sizeof(area->content.metadata));

	ssdfs_create_raw_buffer(SSDFS_CONTENT_BUFFER(area), 0);
	ssdfs_create_raw_buffer(SSDFS_CONTENT_DELTA_BUFFER(area), 0);

	return 0;
}

int ssdfs_create_raw_area_environment(struct ssdfs_raw_area_environment *env,
				      u64 area_offset, u32 area_size,
				      size_t raw_buffer_size)
{
	int err;

	if (!env) {
		SSDFS_ERR("empty pointer on area environment\n");
		return -EINVAL;
	}

	err = ssdfs_create_raw_area(&env->area, area_offset, area_size);
	if (err) {
		SSDFS_ERR("fail to create raw area: "
			  "area_offset %llu, area_size %u, err %d\n",
			  area_offset, area_size, err);
		return err;
	}

	err = ssdfs_create_raw_buffer(&env->buffer, raw_buffer_size);
	if (err) {
		SSDFS_ERR("fail to create raw buffer: "
			  "raw_buffer_size %zu, err %d\n",
			  raw_buffer_size, err);
		return err;
	}

	return 0;
}

int ssdfs_create_raw_dump_environment(struct ssdfs_environment *env,
				      struct ssdfs_raw_dump_environment *raw_dump)
{
	u64 area_offset;
	u32 area_size;
	size_t raw_buffer_size;
	int i;
	int err;

	SSDFS_DBG(env->show_debug, "base %p, raw_dump %p\n",
		  env, raw_dump);

	if (!raw_dump) {
		SSDFS_ERR("empty pointer on raw dump environment\n");
		return -EINVAL;
	}

	memset(raw_dump, 0, sizeof(struct ssdfs_raw_dump_environment));

	raw_dump->peb_offset = U64_MAX;

	area_offset = 0;
	area_size = sizeof(struct ssdfs_segment_header);
	raw_buffer_size = SSDFS_4KB;

	err = ssdfs_create_raw_area_environment(&raw_dump->seg_hdr,
						area_offset, area_size,
						raw_buffer_size);
	if (err) {
		SSDFS_ERR("fail to create segment header area: "
			  "area_offset %llu, area_size %u, "
			  "raw_buffer_size %zu, err %d\n",
			  area_offset, area_size,
			  raw_buffer_size, err);
		goto free_buffers;
	}

	for (i = 0; i < SSDFS_SEG_HDR_DESC_MAX; i++) {
		area_offset = U64_MAX;
		area_size = U32_MAX;
		raw_buffer_size = SSDFS_AREA2BUFFER_SIZE(i);

		err = ssdfs_create_raw_area_environment(&raw_dump->desc[i],
							area_offset, area_size,
							raw_buffer_size);
		if (err) {
			SSDFS_ERR("fail to create area %d: "
				  "area_offset %llu, area_size %u, "
				  "raw_buffer_size %zu, err %d\n",
				  i,
				  area_offset, area_size,
				  raw_buffer_size, err);
			goto free_buffers;
		}
	}

	ssdfs_create_raw_buffer(&raw_dump->content, 0);

	return 0;

free_buffers:
	ssdfs_destroy_raw_dump_environment(raw_dump);

	return errno;
}

void ssdfs_destroy_raw_buffer(struct ssdfs_raw_buffer *buf)
{
	if (buf) {
		if (buf->ptr) {
			free(buf->ptr);
			buf->ptr = NULL;
			buf->size = 0;
		}
	}
}

void ssdfs_destroy_raw_area(struct ssdfs_raw_area *area)
{
	if (area) {
		ssdfs_destroy_raw_buffer(&area->content.uncompressed);
		ssdfs_destroy_raw_buffer(&area->content.delta);
		SSDFS_CREATE_CONTENT_ITERATOR(SSDFS_CONTENT_ITER(area));
	}
}

void ssdfs_destroy_raw_area_environment(struct ssdfs_raw_area_environment *env)
{
	if (env) {
		ssdfs_destroy_raw_area(&env->area);
		ssdfs_destroy_raw_buffer(&env->buffer);
	}
}

void ssdfs_destroy_raw_dump_environment(struct ssdfs_raw_dump_environment *env)
{
	int i;

	if (env) {
		ssdfs_destroy_raw_area_environment(&env->seg_hdr);

		for (i = 0; i < SSDFS_SEG_HDR_DESC_MAX; i++) {
			ssdfs_destroy_raw_area_environment(&env->desc[i]);
		}

		ssdfs_destroy_raw_buffer(&env->content);

		memset(env, 0, sizeof(struct ssdfs_raw_dump_environment));
	}
}

int ssdfs_read_area_content(struct ssdfs_environment *env,
			      u64 peb_id, u32 peb_size,
			      u32 area_offset, u32 size,
			      void *buf)
{
	u64 offset;
	int err;

	SSDFS_DBG(env->show_debug,
		  "peb_id: %llu, peb_size %u, "
		  "area_offset %u, size %u\n",
		  peb_id, peb_size,
		  area_offset, size);

	offset = peb_id * peb_size;
	offset += area_offset;

	err = env->dev_ops->read(env->fd, offset, size, buf,
				 env->show_debug);
	if (err) {
		SSDFS_ERR("fail to read area content: "
			  "offset %llu, size %u, err %d\n",
			  offset, size, err);
		return err;
	}

	return 0;
}

int ssdfs_read_blk_desc_array(struct ssdfs_environment *env,
			      u64 peb_id, u32 peb_size,
			      u32 area_offset, u32 size,
			      void *buf)
{
	return ssdfs_read_area_content(env,
					peb_id, peb_size,
					area_offset, size,
					buf);
}

int ssdfs_read_blk2off_table(struct ssdfs_environment *env,
			     u64 peb_id, u32 peb_size,
			     u32 area_offset, u32 size,
			     void *buf)
{
	return ssdfs_read_area_content(env,
					peb_id, peb_size,
					area_offset, size,
					buf);
}

int ssdfs_read_block_bitmap(struct ssdfs_environment *env,
			    u64 peb_id, u32 peb_size,
			    u32 area_offset, u32 size,
			    void *buf)
{
	return ssdfs_read_area_content(env,
					peb_id, peb_size,
					area_offset, size,
					buf);
}

int ssdfs_read_log_footer(struct ssdfs_environment *env,
			  u64 peb_id, u32 peb_size,
			  u32 area_offset, u32 size,
			  void *buf)
{
	u64 offset;
	int err;

	SSDFS_DBG(env->show_debug,
		  "peb_id: %llu, peb_size %u\n",
		  peb_id, peb_size);

	offset = peb_id * peb_size;
	offset += area_offset;

	err = env->dev_ops->read(env->fd, offset, size, buf,
				 env->show_debug);
	if (err) {
		SSDFS_ERR("fail to read log footer: "
			  "offset %llu, err %d\n",
			  offset, err);
		return err;
	}

	return 0;
}

int ssdfs_read_partial_log_footer(struct ssdfs_environment *env,
				  u64 peb_id, u32 peb_size,
				  u32 area_offset, u32 size,
				  void *buf)
{
	u64 offset;
	int err;

	SSDFS_DBG(env->show_debug,
		  "peb_id: %llu, peb_size %u\n",
		  peb_id, peb_size);

	offset = peb_id * peb_size;
	offset += area_offset;

	err = env->dev_ops->read(env->fd, offset, size, buf,
				 env->show_debug);
	if (err) {
		SSDFS_ERR("fail to read partial log footer: "
			  "offset %llu, err %d\n",
			  offset, err);
		return err;
	}

	return 0;
}

int ssdfs_read_segment_header(struct ssdfs_environment *env,
			      u64 peb_id, u32 peb_size,
			      u32 log_offset, u32 size,
			      void *buf)
{
	size_t sg_size = max_t(size_t,
				sizeof(struct ssdfs_segment_header),
				sizeof(struct ssdfs_partial_log_header));
	u64 offset = SSDFS_RESERVED_VBR_SIZE;
	int err;

	SSDFS_DBG(env->show_debug,
		  "peb_id %llu, peb_size %u, "
		  "log_offset %u, size %u\n",
		  peb_id, peb_size, log_offset, size);

	if (peb_id != SSDFS_INITIAL_SNAPSHOT_SEG)
		offset = peb_id * peb_size;

	offset += log_offset;

	SSDFS_DBG(env->show_debug,
		  "offset %llu, size %zu\n",
		  offset, sg_size);

	err = env->dev_ops->read(env->fd, offset, sg_size, buf,
				 env->show_debug);
	if (err) {
		SSDFS_ERR("fail to read segment header: "
			  "offset %llu, err %d\n",
			  offset, err);
		return err;
	}

	SSDFS_DBG(env->show_debug,
		  "successful read\n");

	return 0;
}

int ssdfs_read_partial_log_header(struct ssdfs_environment *env,
				  u64 peb_id, u32 peb_size,
				  u32 log_offset, u32 size,
				  void *buf)
{
	u64 offset;
	int err;

	SSDFS_DBG(env->show_debug,
		  "peb_id: %llu, peb_size %u, "
		  "log_offset %u, size %u\n",
		  peb_id, peb_size,
		  log_offset, size);

	offset = peb_id * peb_size;
	offset += log_offset;

	err = env->dev_ops->read(env->fd, offset, size, buf,
				 env->show_debug);
	if (err) {
		SSDFS_ERR("fail to read partial log header: "
			  "offset %llu, err %d\n",
			  offset, err);
		return err;
	}

	return 0;
}

#define SSDFS_TOOLS_PEB_SEARCH_SHIFT	(1)

int ssdfs_find_any_valid_peb(struct ssdfs_environment *env,
			     struct ssdfs_segment_header *hdr)
{
	size_t sg_size = sizeof(struct ssdfs_segment_header);
	u64 offset = SSDFS_RESERVED_VBR_SIZE;
	u32 peb_size = env->erase_size;
	u64 factor = 1;
	int err = -ENODATA;

	do {
		struct ssdfs_signature *magic = NULL;
		struct ssdfs_metadata_check *check = NULL;

		SSDFS_DBG(env->show_debug,
			  "try to read the offset %llu\n",
			  offset);

		err = ssdfs_read_segment_header(env,
						offset / peb_size,
						peb_size,
						0, peb_size,
						hdr);
		if (err) {
			SSDFS_ERR("fail to read segment header: "
				  "offset %llu, err %d\n",
				  offset, err);
			return err;
		}

		magic = &hdr->volume_hdr.magic;
		check = &hdr->volume_hdr.check;

		if (le32_to_cpu(magic->common) == SSDFS_SUPER_MAGIC &&
		    le16_to_cpu(magic->key) == SSDFS_SEGMENT_HDR_MAGIC) {
			if (!is_csum_valid(check, hdr, sg_size))
				err = -ENODATA;
		} else
			err = -ENODATA;

		if (!err)
			break;

		if (offset == SSDFS_RESERVED_VBR_SIZE)
			offset = env->erase_size;
		else {
			factor <<= SSDFS_TOOLS_PEB_SEARCH_SHIFT;
			offset += factor * env->erase_size;
		}
	} while (offset < env->fs_size);

	if (err) {
		SSDFS_ERR("SSDFS has not been found on the device %s\n",
			  env->dev_name);
		return err;
	}

	return 0;
}
