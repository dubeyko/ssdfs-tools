//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * include/ssdfs_tools.h - SSDFS tools' declarations.
 *
 * Copyright (c) 2019 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#ifndef _SSDFS_TOOLS_H
#define _SSDFS_TOOLS_H

#ifdef pr_fmt
#undef pr_fmt
#endif

#include "version.h"

#define pr_fmt(fmt) SSDFS_UTILS_VERSION ": " fmt

#include "kerncompat.h"
#include "ssdfs_abi.h"
#include "ssdfs_constants.h"

#define SSDFS_ERR(fmt, ...) \
	fprintf(stderr, pr_fmt("%s:%d:%s(): " fmt), \
		__FILE__, __LINE__, __func__, ##__VA_ARGS__)

#define SSDFS_WARN(fmt, ...) \
	fprintf(stderr, pr_fmt("WARNING: " fmt), ##__VA_ARGS__)

#define SSDFS_INFO(fmt, ...) \
	fprintf(stdout, fmt, ##__VA_ARGS__)

#define SSDFS_DBG(show, fmt, ...) \
	do { \
		if (show) { \
			fprintf(stderr, pr_fmt("%s:%d:%s(): " fmt), \
				__FILE__, __LINE__, __func__, ##__VA_ARGS__); \
		} \
	} while (0)

/*
 * struct ssdfs_device_ops - operations set
 * @fd: file descriptor
 * @offset: offset in bytes
 * @size: size in bytes
 * @buf: pointer on data buffer
 * @erasesize: PEB size in bytes
 * @writesize: NAND flash page size in bytes
 */
struct ssdfs_device_ops {
	/* read method */
	int (*read)(int fd, u64 offset, size_t size, void *buf);
	/* write method */
	int (*write)(int fd, u64 offset, size_t size, void *buf);
	/* erase method */
	int (*erase)(int fd, u64 offset, size_t size, void *buf);
	/* check NAND features */
	int (*check_nand_geometry)(int fd, u32 erasesize, u32 writesize);
	/* check PEB */
	int (*check_peb)(int fd, u64 offset, u32 erasesize);
};

/*
 * struct ssdfs_environment - tool's environment
 * @show_info: show info messages
 * @show_debug: show debug messages
 * @fs_size: size in bytes of selected partition
 * @erase_size: PEB size in bytes
 * @device_type: opened device type
 * @dev_name: name of device
 * @fd: device's file descriptor
 * @dev_ops: device's operations
 */
struct ssdfs_environment {
	int show_info;
	int show_debug;

	u64 fs_size;
	u32 erase_size;

	int device_type;
	const char *dev_name;
	int fd;
	const struct ssdfs_device_ops *dev_ops;
};

/* lib/ssdfs_common.c */
__le32 ssdfs_crc32_le(void *data, size_t len);
int ssdfs_calculate_csum(struct ssdfs_metadata_check *check,
			 void *buf, size_t buf_size);
int is_csum_valid(struct ssdfs_metadata_check *check,
		   void *buf, size_t buf_size);
int open_device(struct ssdfs_environment *env);
int ssdfs_pread(int fd, u64 offset, size_t size, void *buf);
int ssdfs_pwrite(int fd, u64 offset, size_t size, void *buf);
u64 ssdfs_current_time_in_nanoseconds(void);
char *ssdfs_nanoseconds_to_time(u64 nanoseconds);

/* lib/mtd_readwrite.c */
int mtd_read(int fd, u64 offset, size_t size, void *buf);
int mtd_write(int fd, u64 offset, size_t size, void *buf);
int mtd_erase(int fd, u64 offset, size_t size, void *buf);
int mtd_check_nand_geometry(int fd, u32 erasesize, u32 writesize);
int mtd_check_peb(int fd, u64 offset, u32 erasesize);

/* lib/bdev_readwrite.c */
int bdev_read(int fd, u64 offset, size_t size, void *buf);
int bdev_write(int fd, u64 offset, size_t size, void *buf);
int bdev_erase(int fd, u64 offset, size_t size, void *buf);
int bdev_check_nand_geometry(int fd, u32 erasesize, u32 writesize);
int bdev_check_peb(int fd, u64 offset, u32 erasesize);

static const struct ssdfs_device_ops mtd_ops = {
	.read = mtd_read,
	.write = mtd_write,
	.erase = mtd_erase,
	.check_nand_geometry = mtd_check_nand_geometry,
	.check_peb = mtd_check_peb,
};

static const struct ssdfs_device_ops bdev_ops = {
	.read = bdev_read,
	.write = bdev_write,
	.erase = bdev_erase,
	.check_nand_geometry = bdev_check_nand_geometry,
	.check_peb = bdev_check_peb,
};

#endif /* _SSDFS_TOOLS_H */
