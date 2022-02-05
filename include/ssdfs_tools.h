//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * include/ssdfs_tools.h - SSDFS tools' declarations.
 *
 * Copyright (c) 2020-2022 Viacheslav Dubeyko <slava@dubeyko.com>
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

#include <sys/ioctl.h>

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

#define SSDFS_FILE_INFO(stream, fmt, ...) \
	fprintf(stream, fmt, ##__VA_ARGS__)

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

/*
 * struct ssdfs_dentries_tree_testing - dentries tree testing environment
 * @files_number_threshold: maximum number of files
 */
struct ssdfs_dentries_tree_testing {
	u64 files_number_threshold;
};

/*
 * struct ssdfs_extents_tree_testing - extents tree testing environment
 * @file_size_threshold: maximum size of file in bytes
 * @extent_len_threshold: maximum extent length in logical blocks
 */
struct ssdfs_extents_tree_testing {
	u64 file_size_threshold;
	u16 extent_len_threshold;
};

/*
 * struct ssdfs_block_bitmap_testing - block bitmap testing environment
 * @capacity: items capacity in block bitmap
 * @pre_alloc_blks_per_iteration: pre-allocate blocks per iteration
 * @alloc_blks_per_iteration: allocate blocks per iteration
 * @invalidate_blks_per_iteration: invalidate blocks per iteration
 * @reserved_metadata_blks_per_iteration: reserve metadata blocks per iteration
 */
struct ssdfs_block_bitmap_testing {
	u32 capacity;
	u32 pre_alloc_blks_per_iteration;
	u32 alloc_blks_per_iteration;
	u32 invalidate_blks_per_iteration;
	u32 reserved_metadata_blks_per_iteration;
};

/*
 * struct ssdfs_blk2off_testing - blk2off table testing environment
 * @capacity: items capacity in the blk2off table
 */
struct ssdfs_blk2off_testing {
	u32 capacity;
};

/*
 * struct ssdfs_peb_mapping_table_testing - PEB mapping table testing environment
 * @iterations_number: total iterations number
 * @peb_mappings_per_iteration: number of mapping operations per iteration
 * @add_migrations_per_iteration: number of migrating PEBs per iteration
 * @exclude_migrations_per_iteration: number of finishing PEB migrations
 */
struct ssdfs_peb_mapping_table_testing {
	u32 iterations_number;
	u32 peb_mappings_per_iteration;
	u32 add_migrations_per_iteration;
	u32 exclude_migrations_per_iteration;
};

/*
 * struct ssdfs_segment_bitmap_testing - segment bitmap testing environment
 * @iterations_number: total iterations number
 * @using_segs_per_iteration: number of using segments per iteration
 * @used_segs_per_iteration: number of used segments per iteration
 * @pre_dirty_segs_per_iteration: number of pre-dirty segments per iteration
 * @dirty_segs_per_iteration: number of dirty segments per iteration
 * @cleaned_segs_per_iteration: number of cleaned segments per iteration
 */
struct ssdfs_segment_bitmap_testing {
	u32 iterations_number;
	u32 using_segs_per_iteration;
	u32 used_segs_per_iteration;
	u32 pre_dirty_segs_per_iteration;
	u32 dirty_segs_per_iteration;
	u32 cleaned_segs_per_iteration;
};

/*
 * struct ssdfs_shared_dictionary_testing - shared dictionary testing environment
 * @names_number: count of generated names
 * @name_len: length of the name
 * @step_factor: growing factor of symbol calulation
 */
struct ssdfs_shared_dictionary_testing {
	u32 names_number;
	u32 name_len;
	u32 step_factor;
};

/*
 * struct ssdfs_xattr_tree_testing - xattr tree testing environment
 * @xattrs_number: number of extended attributes
 * @name_len: length of the name
 * @step_factor: growing factor of symbol calulation
 * @blob_len: length of blob
 * @blob_pattern: pattern to generate the blob
 */
struct ssdfs_xattr_tree_testing {
	u32 xattrs_number;
	u32 name_len;
	u32 step_factor;
	u32 blob_len;
	u64 blob_pattern;
};

/*
 * struct ssdfs_shextree_testing - shared extents tree testing environment
 * @extents_number_threshold: maximum number of shared extents
 * @extent_len: extent length
 * @ref_count_threshold: upper bound for reference counter of shared extent
 */
struct ssdfs_shextree_testing {
	u64 extents_number_threshold;
	u32 extent_len;
	u32 ref_count_threshold;
};

/*
 * struct ssdfs_testing_environment - define testing environment
 * @subsystems: enable testing particular subsystems
 * @page_size: logical block size in bytes
 *
 * @dentries_tree: dentries tree testing environment
 * @extents_tree: extents tree testing environment
 * @block_bitmap: block bitmap testing environment
 * @blk2off_table: blk2off table testing environment
 * @segment_bitmap: segment bitmap testing environment
 * @shared_dictionary: shared dictionary testing environment
 * @xattr_tree: xattr tree testing environment
 * @shextree: shared extents tree testing environment
 */
struct ssdfs_testing_environment {
	u64 subsystems;
	u32 page_size;

	struct ssdfs_dentries_tree_testing dentries_tree;
	struct ssdfs_extents_tree_testing extents_tree;
	struct ssdfs_block_bitmap_testing block_bitmap;
	struct ssdfs_blk2off_testing blk2off_table;
	struct ssdfs_peb_mapping_table_testing mapping_table;
	struct ssdfs_segment_bitmap_testing segment_bitmap;
	struct ssdfs_shared_dictionary_testing shared_dictionary;
	struct ssdfs_xattr_tree_testing xattr_tree;
	struct ssdfs_shextree_testing shextree;
};

/* Subsystem tests */
#define SSDFS_ENABLE_EXTENTS_TREE_TESTING	(1 << 0)
#define SSDFS_ENABLE_DENTRIES_TREE_TESTING	(1 << 1)
#define SSDFS_ENABLE_BLOCK_BMAP_TESTING		(1 << 2)
#define SSDFS_ENABLE_BLK2OFF_TABLE_TESTING	(1 << 3)
#define SSDFS_ENABLE_PEB_MAPPING_TABLE_TESTING	(1 << 4)
#define SSDFS_ENABLE_SEGMENT_BITMAP_TESTING	(1 << 5)
#define SSDFS_ENABLE_SHARED_DICTIONARY_TESTING	(1 << 6)
#define SSDFS_ENABLE_XATTR_TREE_TESTING		(1 << 7)
#define SSDFS_ENABLE_SHEXTREE_TESTING		(1 << 8)

/*
 * struct ssdfs_time_range - time range definition
 * @day: day of the time range
 * @month: month of the time range
 * @year: year of the time range
 */
struct ssdfs_time_range {
	u32 day;
	u32 month;
	u32 year;
};

#define SSDFS_ANY_DAY				U32_MAX
#define SSDFS_ANY_MONTH				U32_MAX
#define SSDFS_ANY_YEAR				U32_MAX

/*
 * struct ssdfs_snapshot_info - snapshot details
 * @name: snapshot name
 * @uuid: snapshot UUID
 * @mode: snapshot mode (READ-ONLY|READ-WRITE)
 * @type: snapshot type (PERIODIC|ONE-TIME)
 * @expiration: snapshot expiration time (WEEK|MONTH|YEAR|NEVER)
 * @frequency: taking snapshot frequency (FSYNC|HOUR|DAY|WEEK)
 * @snapshots_threshold: max number of simultaneously available snapshots
 * @time_range: time range to select/modify/delete snapshots
 */
struct ssdfs_snapshot_info {
	char name[SSDFS_MAX_NAME_LEN];
	u8 uuid[SSDFS_UUID_SIZE];

	int mode;
	int type;
	int expiration;
	int frequency;
	u32 snapshots_threshold;
	struct ssdfs_time_range time_range;
};

/* Snapshot mode */
enum {
	SSDFS_UNKNOWN_SNAPSHOT_MODE,
	SSDFS_READ_ONLY_SNAPSHOT,
	SSDFS_READ_WRITE_SNAPSHOT,
	SSDFS_SNAPSHOT_MODE_MAX
};

#define SSDFS_READ_ONLY_MODE_STR	"READ_ONLY"
#define SSDFS_READ_WRITE_MODE_STR	"READ_WRITE"

/* Snapshot type */
enum {
	SSDFS_UNKNOWN_SNAPSHOT_TYPE,
	SSDFS_ONE_TIME_SNAPSHOT,
	SSDFS_PERIODIC_SNAPSHOT,
	SSDFS_SNAPSHOT_TYPE_MAX
};

#define SSDFS_ONE_TIME_TYPE_STR		"ONE_TIME"
#define SSDFS_PERIODIC_TYPE_STR		"PERIODIC"

/* Snapshot expiration */
enum {
	SSDFS_UNKNOWN_EXPIRATION_POINT,
	SSDFS_EXPIRATION_IN_WEEK,
	SSDFS_EXPIRATION_IN_MONTH,
	SSDFS_EXPIRATION_IN_YEAR,
	SSDFS_NEVER_EXPIRED,
	SSDFS_EXPIRATION_POINT_MAX
};

#define SSDFS_WEEK_EXPIRATION_POINT_STR		"WEEK"
#define SSDFS_MONTH_EXPIRATION_POINT_STR	"MONTH"
#define SSDFS_YEAR_EXPIRATION_POINT_STR		"YEAR"
#define SSDFS_NEVER_EXPIRED_STR			"NEVER"

/* Snapshot creation frequency */
enum {
	SSDFS_UNKNOWN_FREQUENCY,
	SSDFS_SYNCFS_FREQUENCY,
	SSDFS_HOUR_FREQUENCY,
	SSDFS_DAY_FREQUENCY,
	SSDFS_WEEK_FREQUENCY,
	SSDFS_MONTH_FREQUENCY,
	SSDFS_CREATION_FREQUENCY_MAX
};

#define SSDFS_SYNCFS_FREQUENCY_STR		"SYNCFS"
#define SSDFS_HOUR_FREQUENCY_STR		"HOUR"
#define SSDFS_DAY_FREQUENCY_STR			"DAY"
#define SSDFS_WEEK_FREQUENCY_STR		"WEEK"
#define SSDFS_MONTH_FREQUENCY_STR		"MONTH"

#define SSDFS_INFINITE_SNAPSHOTS_NUMBER		U32_MAX
#define SSDFS_UNDEFINED_SNAPSHOTS_NUMBER	(0)

#define SSDFS_IOCTL_MAGIC 0xdf

#define SSDFS_IOC_DO_TESTING		_IOW(SSDFS_IOCTL_MAGIC, 1, \
					     struct ssdfs_testing_environment)
#define SSDFS_IOC_CREATE_SNAPSHOT	_IOW(SSDFS_IOCTL_MAGIC, 2, \
					     struct ssdfs_snapshot_info)
#define SSDFS_IOC_LIST_SNAPSHOTS	_IOWR(SSDFS_IOCTL_MAGIC, 3, \
					     struct ssdfs_snapshot_info)
#define SSDFS_IOC_MODIFY_SNAPSHOT	_IOW(SSDFS_IOCTL_MAGIC, 4, \
					     struct ssdfs_snapshot_info)
#define SSDFS_IOC_REMOVE_SNAPSHOT	_IOW(SSDFS_IOCTL_MAGIC, 5, \
					     struct ssdfs_snapshot_info)
#define SSDFS_IOC_REMOVE_RANGE		_IOW(SSDFS_IOCTL_MAGIC, 6, \
					     struct ssdfs_snapshot_info)
#define SSDFS_IOC_SHOW_DETAILS		_IOWR(SSDFS_IOCTL_MAGIC, 7, \
					     struct ssdfs_snapshot_info)

/* lib/ssdfs_common.c */
const char *uuid_string(const unsigned char *uuid);
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
