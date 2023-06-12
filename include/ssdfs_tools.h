//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * include/ssdfs_tools.h - SSDFS tools' declarations.
 *
 * Copyright (c) 2020-2023 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
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

#include <zlib.h>
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
 * struct ssdfs_nand_geometry - NAND geometry details
 * @erasesize: erase size in bytes
 * @writesize: NAND flash page size in bytes
 */
struct ssdfs_nand_geometry {
	u32 erasesize;
	u32 writesize;
};

/*
 * struct ssdfs_device_ops - operations set
 * @fd: file descriptor
 * @offset: offset in bytes
 * @size: size in bytes
 * @buf: pointer on data buffer
 * @erasesize: PEB size in bytes
 * @writesize: NAND flash page size in bytes
 * @info: NAND geometry details
 */
struct ssdfs_device_ops {
	/* read method */
	int (*read)(int fd, u64 offset, size_t size, void *buf, int is_debug);
	/* write method */
	int (*write)(int fd, struct ssdfs_nand_geometry *info,
		     u64 offset, size_t size, void *buf,
		     u32 *open_zones, int is_debug);
	/* erase method */
	int (*erase)(int fd, u64 offset, size_t size,
		     void *buf, size_t buf_size, int is_debug);
	/* check NAND features */
	int (*check_nand_geometry)(int fd, struct ssdfs_nand_geometry *info,
				   int is_debug);
	/* check PEB */
	int (*check_peb)(int fd, u64 offset, u32 erasesize,
			 int need_close_zone, int is_debug);
};

/*
 * struct ssdfs_environment - tool's environment
 * @show_info: show info messages
 * @show_debug: show debug messages
 * @fs_size: size in bytes of selected partition
 * @erase_size: PEB size in bytes
 * @open_zones: number of open/active zones
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
	u32 open_zones;

	int device_type;
	const char *dev_name;
	int fd;
	const struct ssdfs_device_ops *dev_ops;
};

/*
 * struct ssdfs_peb_environment - PEB environment
 * @id: PEB's identification number
 * @pebs_count: count of PEBs in the range
 * @peb_size: PEB size in bytes
 * @log_offset: log offset in bytes
 * @log_size: log's size in bytes
 * @log_index: log index in chain
 * @logs_count: count of logs in the range
 */
struct ssdfs_peb_environment {
	u64 id;
	u64 pebs_count;
	u32 peb_size;

	u32 log_offset;
	u32 log_size;
	u32 log_index;
	u32 logs_count;
};

/*
 * struct ssdfs_raw_area - raw area decriptor
 * @offset: area offset from PEB's beginning
 * @size: area size
 * @metadata.blk_desc_tbl: block descriptors table header
 * @metadata.blk2off_tbl.hdr: block2offset table header
 * @metadata.blk2off_tbl.off_tbl_hdr: phys offsets table header
 */
struct ssdfs_raw_area {
	u64 offset;
	u32 size;

	union {
		struct ssdfs_area_block_table blk_desc_tbl;
		struct {
			struct ssdfs_blk2off_table_header hdr;
			struct ssdfs_phys_offset_table_header off_tbl_hdr;
		} blk2off_tbl;
	} metadata;
};

/*
 * struct ssdfs_raw_buffer - raw buffer descriptor
 * @ptr: pointer on buffer
 * @size: buffer size
 */
struct ssdfs_raw_buffer {
	void *ptr;
	u32 size;
};

/*
 * struct ssdfs_raw_area_environment - raw area environment
 * @area: area descriptor
 * @buffer: buffer descriptor
 */
struct ssdfs_raw_area_environment {
	struct ssdfs_raw_area area;
	struct ssdfs_raw_buffer buffer;
};

/*
 * struct ssdfs_raw_dump_environment - raw dump environment
 * @peb_offset: PEB offset in bytes
 * @seg_hdr: segment header area
 * @desc: array of log's area descriptors
 */
struct ssdfs_raw_dump_environment {
	u64 peb_offset;

	struct ssdfs_raw_area_environment seg_hdr;
	struct ssdfs_raw_area_environment desc[SSDFS_SEG_HDR_DESC_MAX];
};

union ssdfs_log_header {
	struct ssdfs_segment_header seg_hdr;
	struct ssdfs_partial_log_header pl_hdr;
	struct ssdfs_signature magic;
};

/*
 * struct ssdfs_thread_state - thread state
 * @id: thread ID
 * @thread: thread descriptor
 * @err: code of error
 *
 * @base: basic environment
 * @peb: PEB environment
 * @raw_dump: raw dump environment
 * @output_folder: path to the output folder
 * @output_fd: output folder descriptor
 *
 * @name_buf: name buffer
 */
struct ssdfs_thread_state {
	unsigned int id;
	pthread_t thread;
	int err;

	struct ssdfs_environment base;
	struct ssdfs_peb_environment peb;
	struct ssdfs_raw_dump_environment raw_dump;
	const char *output_folder;
	int output_fd;

	char name_buf[SSDFS_MAX_NAME_LEN + 1];
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
 * struct ssdfs_snapshots_tree_testing - snapshots tree testing environment
 * @snapshots_number_threshold: maximum number of snapshots
 */
struct ssdfs_snapshots_tree_testing {
	u64 snapshots_number_threshold;
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
 * @snapshots_tree: snaphots tree testing environment
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
	struct ssdfs_snapshots_tree_testing snapshots_tree;
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
#define SSDFS_ENABLE_SNAPSHOTS_TREE_TESTING	(1 << 9)

/*
 * struct ssdfs_time_range - time range definition
 * @minute: minute of the time range
 * @hour: hour of the time range
 * @day: day of the time range
 * @month: month of the time range
 * @year: year of the time range
 */
struct ssdfs_time_range {
	u32 minute;
	u32 hour;
	u32 day;
	u32 month;
	u32 year;
};

#define SSDFS_ANY_MINUTE			U32_MAX
#define SSDFS_ANY_HOUR				U32_MAX
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

/*
 * Option possible states
 */
enum {
	/* REQUEST */
	SSDFS_IGNORE_OPTION,
	SSDFS_ENABLE_OPTION,
	SSDFS_DISABLE_OPTION,

	/* RESPONCE */
	SSDFS_DONT_SUPPORT_OPTION,
	SSDFS_USE_RECOMMENDED_VALUE,
	SSDFS_UNRECOGNIZED_VALUE,
};

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

#define SSDFS_SEG_HDR(ptr) \
	((struct ssdfs_segment_header *)(ptr))
#define SSDFS_PL_HDR(ptr) \
	((struct ssdfs_partial_log_header *)(ptr))
#define SSDFS_LOG_FOOTER(ptr) \
	((struct ssdfs_log_footer *)(ptr))

/* lib/ssdfs_common.c */
const char *uuid_string(const unsigned char *uuid);
__le32 ssdfs_crc32_le(void *data, size_t len);
int ssdfs_calculate_csum(struct ssdfs_metadata_check *check,
			 void *buf, size_t buf_size);
int is_csum_valid(struct ssdfs_metadata_check *check,
		   void *buf, size_t buf_size);
int open_device(struct ssdfs_environment *env, u32 flags);
int ssdfs_pread(int fd, u64 offset, size_t size, void *buf);
int ssdfs_pwrite(int fd, u64 offset, size_t size, void *buf);
u64 ssdfs_current_time_in_nanoseconds(void);
char *ssdfs_nanoseconds_to_time(u64 nanoseconds);
int is_zoned_device(int fd);
int ssdfs_create_raw_buffers(struct ssdfs_environment *base,
			     struct ssdfs_raw_dump_environment *raw_dump);
void ssdfs_destroy_raw_buffers(struct ssdfs_raw_dump_environment *raw_dump);
int ssdfs_read_blk_desc_array(struct ssdfs_environment *env,
			      u64 peb_id, u32 peb_size,
			      u32 area_offset, u32 size,
			      void *buf);
int ssdfs_read_blk2off_table(struct ssdfs_environment *env,
			     u64 peb_id, u32 peb_size,
			     u32 area_offset, u32 size,
			     void *buf);
int ssdfs_read_block_bitmap(struct ssdfs_environment *env,
			    u64 peb_id, u32 peb_size,
			    u32 area_offset, u32 size,
			    void *buf);
int ssdfs_read_log_footer(struct ssdfs_environment *env,
			  u64 peb_id, u32 peb_size,
			  u32 area_offset, u32 size,
			  void *buf);
int ssdfs_read_partial_log_footer(struct ssdfs_environment *env,
				  u64 peb_id, u32 peb_size,
				  u32 area_offset, u32 size,
				  void *buf);
int ssdfs_read_segment_header(struct ssdfs_environment *env,
			      u64 peb_id, u32 peb_size,
			      u32 log_offset, u32 size,
			      void *buf);
int ssdfs_read_partial_log_header(struct ssdfs_environment *env,
				  u64 peb_id, u32 peb_size,
				  u32 log_offset, u32 size,
				  void *buf);
int ssdfs_find_any_valid_peb(struct ssdfs_environment *env,
			     struct ssdfs_segment_header *hdr);

/* lib/compression.c */
int ssdfs_zlib_compress(unsigned char *data_in,
			unsigned char *cdata_out,
			u32 *srclen, u32 *destlen,
			int is_debug);
int ssdfs_zlib_decompress(unsigned char *cdata_in,
			  unsigned char *data_out,
			  u32 srclen, u32 destlen,
			  int is_debug);

/* lib/mtd_readwrite.c */
int mtd_read(int fd, u64 offset, size_t size, void *buf, int is_debug);
int mtd_write(int fd, struct ssdfs_nand_geometry *info,
		u64 offset, size_t size, void *buf,
		u32 *open_zones, int is_debug);
int mtd_erase(int fd, u64 offset, size_t size,
		void *buf, size_t buf_size, int is_debug);
int mtd_check_nand_geometry(int fd, struct ssdfs_nand_geometry *info,
			    int is_debug);
int mtd_check_peb(int fd, u64 offset, u32 erasesize,
		  int need_close_zone, int is_debug);

/* lib/bdev_readwrite.c */
int bdev_read(int fd, u64 offset, size_t size, void *buf, int is_debug);
int bdev_write(int fd, struct ssdfs_nand_geometry *info,
		u64 offset, size_t size, void *buf,
		u32 *open_zones, int is_debug);
int bdev_erase(int fd, u64 offset, size_t size,
		void *buf, size_t buf_size, int is_debug);
int bdev_check_nand_geometry(int fd, struct ssdfs_nand_geometry *info,
			     int is_debug);
int bdev_check_peb(int fd, u64 offset, u32 erasesize,
		   int need_close_zone, int is_debug);

/* lib/zns_readwrite.c */
int zns_read(int fd, u64 offset, size_t size, void *buf, int is_debug);
int zns_write(int fd, struct ssdfs_nand_geometry *info,
		u64 offset, size_t size, void *buf,
		u32 *open_zones, int is_debug);
int zns_erase(int fd, u64 offset, size_t size,
		void *buf, size_t buf_size, int is_debug);
int zns_check_nand_geometry(int fd, struct ssdfs_nand_geometry *info,
			    int is_debug);
int zns_check_peb(int fd, u64 offset, u32 erasesize,
		  int need_close_zone, int is_debug);

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

static const struct ssdfs_device_ops zns_ops = {
	.read = zns_read,
	.write = zns_write,
	.erase = zns_erase,
	.check_nand_geometry = zns_check_nand_geometry,
	.check_peb = zns_check_peb,
};

#endif /* _SSDFS_TOOLS_H */
