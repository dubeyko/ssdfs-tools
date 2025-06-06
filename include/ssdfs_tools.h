/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * include/ssdfs_tools.h - SSDFS tools' declarations.
 *
 * Copyright (c) 2020-2025 Viacheslav Dubeyko <slava@dubeyko.com>
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
#include <time.h>

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
 * struct ssdfs_environment - tool's environment
 * @show_info: show info messages
 * @show_debug: show debug messages
 * @fs_size: size in bytes of selected partition
 * @erase_size: PEB size in bytes
 * @open_zones: number of open/active zones
 * @page_size: logical block size in bytes
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
	u32 page_size;

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
 * struct ssdfs_raw_buffer - raw buffer descriptor
 * @ptr: pointer on buffer
 * @size: buffer size
 */
struct ssdfs_raw_buffer {
	void *ptr;
	u32 size;
};

/*
 * struct ssdfs_raw_content_iterator - raw content iterator
 * @state: content state
 * @portion_offset: offset of block descriptors table's portion
 * @portion_size: size of portion in bytes
 * @fragment_index: index of fragment in portion
 * @fragment_size: size of fragment in bytes
 * @item_offset: item offset in bytes from area beginning
 * @item_size: item size in bytes
 */
struct ssdfs_raw_content_iterator {
	int state;

	u32 portion_offset;
	u32 portion_size;

	int fragment_index;
	u32 fragment_size;

	u32 item_offset;
	u32 item_size;
};

/*
 * Content states
 */
enum {
	SSDFS_RAW_AREA_CONTENT_UNKNOWN_STATE,
	SSDFS_RAW_AREA_CONTENT_ITERATOR_INITIALIZED,
	SSDFS_RAW_AREA_CONTENT_PROCESSED,
	SSDFS_RAW_AREA_CONTENT_STATE_MAX
};

/*
 * struct ssdfs_raw_area - raw area decriptor
 * @offset: area offset from PEB's beginning
 * @size: area size
 * @content.iter: iterator on current item
 * @content.metadata.raw_buffer: raw buffer
 * @content.metadata.blk_desc_hdr: block descriptors table header
 * @content.metadata.blk2off_tbl.hdr: block2offset table header
 * @content.metadata.blk2off_tbl.off_tbl_hdr: phys offsets table header
 * @content.uncompressed: decompressed content buffer
 * @content.delta: delta buffer
 */
struct ssdfs_raw_area {
	u64 offset;
	u32 size;

	struct {
		struct ssdfs_raw_content_iterator iter;

		union {
			u8 raw_buffer;
			struct ssdfs_area_block_table blk_desc_hdr;
			struct {
				struct ssdfs_blk2off_table_header hdr;
				struct ssdfs_phys_offset_table_header off_tbl_hdr;
			} blk2off_tbl;
		} metadata;

		struct ssdfs_raw_buffer uncompressed;
		struct ssdfs_raw_buffer delta;
	} content;
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
 * @content: extracted dump of data
 */
struct ssdfs_raw_dump_environment {
	u64 peb_offset;

	struct ssdfs_raw_area_environment seg_hdr;
	struct ssdfs_raw_area_environment desc[SSDFS_SEG_HDR_DESC_MAX];

	struct ssdfs_raw_buffer content;
};

#define SSDFS_CONTENT_BUFFER(area) \
	((struct ssdfs_raw_buffer *)(&area->content.uncompressed))
#define SSDFS_CONTENT_DELTA_BUFFER(area) \
	((struct ssdfs_raw_buffer *)(&area->content.delta))
#define SSDFS_CONTENT_ITER(area) \
	((struct ssdfs_raw_content_iterator *)(&area->content.iter))
#define SSDFS_CONTENT_BLK_DESC_HDR(area) \
	((struct ssdfs_area_block_table *)(&area->content.metadata.blk_desc_hdr))
#define SSDFS_RAW_SEG_HDR(dump_env) \
	((struct ssdfs_raw_buffer *)(&dump_env->seg_hdr.buffer))
#define SSDFS_RAW_AREA_ENV(dump_env, area_index) \
	((struct ssdfs_raw_area_environment *)(&dump_env->desc[area_index]))
#define SSDFS_COMPR_CONTENT(dump_env, area_index) \
	((struct ssdfs_raw_buffer *)(&SSDFS_RAW_AREA_ENV(dump_env, area_index)->buffer))
#define SSDFS_UNCOMPR_BUFFER(dump_env, area_index) \
	((struct ssdfs_raw_buffer *)(&SSDFS_RAW_AREA_ENV(dump_env, \
					area_index)->area.content.uncompressed))
#define SSDFS_DUMP_DATA(dump_env) \
	((struct ssdfs_raw_buffer *)(&dump_env->content))

union ssdfs_metadata_header {
	struct ssdfs_segment_header seg_hdr;
	struct ssdfs_partial_log_header pl_hdr;
	struct ssdfs_signature magic;
};

union ssdfs_metadata_footer {
	struct ssdfs_log_footer footer;
	struct ssdfs_partial_log_header pl_hdr;
	struct ssdfs_signature magic;
};

/*
 * struct ssdfs_folder_environment - output folder environment
 * @name: path to the folder
 * @fd: folder descriptor
 * @content.namelist: list of directory entries in the folder
 * @content.count: number of items in the list
 */
struct ssdfs_folder_environment {
	const char *name;
	int fd;

	struct {
		struct dirent **namelist;
		int count;
	} content;
};

/*
 * struct ssdfs_file_environment - data file environment
 * @fd: file descriptor
 * @inode_id: inode ID of processing file
 * @content.buffer: buffer for file's content
 * @content.size: buffer size in bytes
 */
struct ssdfs_file_environment {
	int fd;
	u64 inode_id;

	struct {
		u8 *buffer;
		size_t size;
	} content;
};

/*
 * struct ssdfs_metadata_peb_item - metadata PEB descriptor
 * @seg_id: segment ID
 * @leb_id: LEB ID
 * @peb_id: PEB ID
 * @type: PEB type
 * @index: PEB index in segment
 * @peb_creation_timestamp: PEB creation timestamp
 * @volume_creation_timestamp: volume creation timestamp
 */
struct ssdfs_metadata_peb_item {
	u64 seg_id;
	u64 leb_id;
	u64 peb_id;
	int type;
	u64 peb_creation_timestamp;
	u64 volume_creation_timestamp;
};

/*
 * struct ssdfs_metadata_map - metadata map
 * @array: metadata PEB descriptors array
 * @capacity: capacity of the array
 * @count: current number of items in the array
 */
struct ssdfs_metadata_map {
	struct ssdfs_metadata_peb_item *array;
	int capacity;
	int count;
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
 * @output_folder: output folder environment
 * @checkpoint_folder: checkpoint folder environment
 * @data_file: data file environment
 * @timestamp: timestamp defining the state of files
 * @metadata_map: metadata map
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
	struct ssdfs_folder_environment output_folder;
	struct ssdfs_folder_environment checkpoint_folder;
	struct ssdfs_file_environment data_file;
	struct ssdfs_time_range timestamp;
	struct ssdfs_metadata_map metadata_map;

	char name_buf[SSDFS_MAX_NAME_LEN + 1];
};

/*
 * struct ssdfs_threads_environment - threads environment
 * @jobs: thread state array
 * @capacity: capacity of the array
 * @requested_jobs: number of really requested jobs
 */
struct ssdfs_threads_environment {
	struct ssdfs_thread_state *jobs;
	unsigned int capacity;
	unsigned int requested_jobs;
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
 * struct ssdfs_memory_primitives_testing - memory primitives testing environment
 * @iterations_number: total iterations number
 * @capacity: total capacity of items
 * @count: total count of items
 * @item_size: item size in bytes
 * @test_types: type of tests
 */
struct ssdfs_memory_primitives_testing {
	u32 iterations_number;
	u64 capacity;
	u64 count;
	u32 item_size;
	u32 test_types;
};

/* Types of memory primitives tests */
#define SSDFS_ENABLE_FOLIO_VECTOR_TESTING	(1 << 0)
#define SSDFS_ENABLE_FOLIO_ARRAY_TESTING	(1 << 1)
#define SSDFS_ENABLE_DYNAMIC_ARRAY_TESTING	(1 << 2)

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
 * @mapping_table: mapping table testing environment
 * @memory_primitives: memory primitives testing environment
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
	struct ssdfs_memory_primitives_testing memory_primitives;
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
#define SSDFS_ENABLE_MEMORY_PRIMITIVES_TESTING	(1 << 5)
#define SSDFS_ENABLE_SEGMENT_BITMAP_TESTING	(1 << 6)
#define SSDFS_ENABLE_SHARED_DICTIONARY_TESTING	(1 << 7)
#define SSDFS_ENABLE_XATTR_TREE_TESTING		(1 << 8)
#define SSDFS_ENABLE_SHEXTREE_TESTING		(1 << 9)
#define SSDFS_ENABLE_SNAPSHOTS_TREE_TESTING	(1 << 10)

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
 * @buf: buffer to share the snapshot details
 * @buf_size: size of buffer in bytes
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

	char *buf;
	u64 buf_size;
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
	SSDFS_NOT_IMPLEMENTED_OPTION,
	SSDFS_OPTION_HAS_BEEN_APPLIED,
};

/*
 * struct ssdfs_tunefs_option - option state
 * @state: option state (ignore, enable, disable)
 * @value: option value
 * @recommended_value: value returned by driver as better option
 */
struct ssdfs_tunefs_option {
	int state;
	int value;
	int recommended_value;
};

/*
 * struct ssdfs_tunefs_volume_label_option - volume label option
 * @state: option state (ignore, enable, disable)
 * @volume_label: volume label
 */
struct ssdfs_tunefs_volume_label_option {
	int state;
	char volume_label[SSDFS_VOLUME_LABEL_MAX];
};

/*
 * struct ssdfs_tunefs_blkbmap_options - block bitmap options
 * @has_backup_copy: backup copy is present?
 * @compression: compression type
 */
struct ssdfs_tunefs_blkbmap_options {
	struct ssdfs_tunefs_option has_backup_copy;
	struct ssdfs_tunefs_option compression;
};

/*
 * struct ssdfs_tunefs_blk2off_table_options - offsets table options
 * @has_backup_copy: backup copy is present?
 * @compression: compression type
 */
struct ssdfs_tunefs_blk2off_table_options {
	struct ssdfs_tunefs_option has_backup_copy;
	struct ssdfs_tunefs_option compression;
};

/*
 * struct ssdfs_tunefs_segbmap_options - segment bitmap options
 * @has_backup_copy: backup copy is present?
 * @log_pages: count of pages in the full log
 * @migration_threshold: max amount of migrating PEBs for segment
 * @compression: compression type
 */
struct ssdfs_tunefs_segbmap_options {
	struct ssdfs_tunefs_option has_backup_copy;
	struct ssdfs_tunefs_option log_pages;
	struct ssdfs_tunefs_option migration_threshold;
	struct ssdfs_tunefs_option compression;
};

/*
 * struct ssdfs_tunefs_maptbl_options - PEB mapping table options
 * @has_backup_copy: backup copy is present?
 * @log_pages: count of pages in the full log
 * @migration_threshold: max amount of migrating PEBs for segment
 * @reserved_pebs_per_fragment: percentage of reserved PEBs for fragment
 * @compression: compression type
 */
struct ssdfs_tunefs_maptbl_options {
	struct ssdfs_tunefs_option has_backup_copy;
	struct ssdfs_tunefs_option log_pages;
	struct ssdfs_tunefs_option migration_threshold;
	struct ssdfs_tunefs_option reserved_pebs_per_fragment;
	struct ssdfs_tunefs_option compression;
};

/*
 * struct ssdfs_tunefs_btree_options - btree options
 * @min_index_area_size: minimal index area's size in bytes
 * @lnode_log_pages: leaf node's log pages
 * @hnode_log_pages: hybrid node's log pages
 * @inode_log_pages: index node's log pages
 */
struct ssdfs_tunefs_btree_options {
	struct ssdfs_tunefs_option min_index_area_size;
	struct ssdfs_tunefs_option lnode_log_pages;
	struct ssdfs_tunefs_option hnode_log_pages;
	struct ssdfs_tunefs_option inode_log_pages;
};

/*
 * struct ssdfs_tunefs_user_data_options - user data options
 * @log_pages: count of pages in the full log
 * @migration_threshold: max amount of migrating PEBs for segment
 * @compression: compression type
 */
struct ssdfs_tunefs_user_data_options {
	struct ssdfs_tunefs_option log_pages;
	struct ssdfs_tunefs_option migration_threshold;
	struct ssdfs_tunefs_option compression;
};

/*
 * struct ssdfs_metadata_options - metadata options
 * @blk_bmap.flags: block bitmap's flags
 * @blk_bmap.compression: compression type
 *
 * @blk2off_tbl.flags: offset translation table's flags
 * @blk2off_tbl.compression: compression type
 *
 * @user_data.flags: user data's flags
 * @user_data.compression: compression type
 * @user_data.migration_threshold: default value of destination PEBs in migration
 */
struct ssdfs_metadata_options {
	struct {
		u16 flags;
		u8 compression;
	} blk_bmap;

	struct {
		u16 flags;
		u8 compression;
	} blk2off_tbl;

	struct {
		u16 flags;
		u8 compression;
		u16 migration_threshold;
	} user_data;
};

/*
 * struct ssdfs_current_volume_config - current volume config
 * @fs_uuid: 128-bit volume's uuid
 * @fs_label: volume name
 * @nsegs: number of segments on the volume
 * @pagesize: page size in bytes
 * @erasesize: physical erase block size in bytes
 * @segsize: segment size in bytes
 * @pebs_per_seg: physical erase blocks per segment
 * @pages_per_peb: pages per physical erase block
 * @pages_per_seg: pages per segment
 * @leb_pages_capacity: maximal number of logical blocks per LEB
 * @peb_pages_capacity: maximal number of NAND pages can be written per PEB
 * @fs_ctime: volume create timestamp (mkfs phase)
 * @raw_inode_size: raw inode size in bytes
 * @create_threads_per_seg: number of creation threads per segment
 * @metadata_options: metadata options
 * @sb_seg_log_pages: full log size in sb segment (pages count)
 * @segbmap_log_pages: full log size in segbmap segment (pages count)
 * @segbmap_flags: segment bitmap flags
 * @maptbl_log_pages: full log size in maptbl segment (pages count)
 * @maptbl_flags: mapping table flags
 * @lnodes_seg_log_pages: full log size in leaf nodes segment (pages count)
 * @hnodes_seg_log_pages: full log size in hybrid nodes segment (pages count)
 * @inodes_seg_log_pages: full log size in index nodes segment (pages count)
 * @user_data_log_pages: full log size in user data segment (pages count)
 * @migration_threshold: default value of destination PEBs in migration
 * @is_zns_device: file system volume is on ZNS device
 * @zone_size: zone size in bytes
 * @zone_capacity: zone capacity in bytes available for write operations
 * @max_open_zones: open zones limitation (upper bound)
 */
struct ssdfs_current_volume_config {
	unsigned char fs_uuid[SSDFS_UUID_SIZE];
	char fs_label[SSDFS_VOLUME_LABEL_MAX];

	u64 nsegs;
	u32 pagesize;
	u32 erasesize;
	u32 segsize;
	u32 pebs_per_seg;
	u32 pages_per_peb;
	u32 pages_per_seg;
	u32 leb_pages_capacity;
	u32 peb_pages_capacity;
	u64 fs_ctime;
	u16 raw_inode_size;
	u16 create_threads_per_seg;

	struct ssdfs_metadata_options metadata_options;

	u16 sb_seg_log_pages;

	u16 segbmap_log_pages;
	u16 segbmap_flags;

	u16 maptbl_log_pages;
	u16 maptbl_flags;

	u16 lnodes_seg_log_pages;
	u16 hnodes_seg_log_pages;
	u16 inodes_seg_log_pages;
	u16 user_data_log_pages;

	u16 migration_threshold;

	int is_zns_device;
	u64 zone_size;
	u64 zone_capacity;
	u32 max_open_zones;
};

/*
 * struct ssdfs_tunefs_config_request - tunefs config request
 * @label: volume label option
 * @blkbmap: block bitmap options
 * @blk2off_tbl: offset translation table options
 * @segbmap: segment bitmap options
 * @maptbl: PEB mapping table options
 * @btree: btree options
 * @user_data_seg: user data segment's options
 */
struct ssdfs_tunefs_config_request {
	struct ssdfs_tunefs_volume_label_option label;
	struct ssdfs_tunefs_blkbmap_options blkbmap;
	struct ssdfs_tunefs_blk2off_table_options blk2off_tbl;
	struct ssdfs_tunefs_segbmap_options segbmap;
	struct ssdfs_tunefs_maptbl_options maptbl;
	struct ssdfs_tunefs_btree_options btree;
	struct ssdfs_tunefs_user_data_options user_data_seg;
};

/*
 * struct ssdfs_tunefs_options - tunefs options
 * @old_config: current volume configuration
 * @new_config: tunefs configuration request
 */
struct ssdfs_tunefs_options {
	struct ssdfs_current_volume_config old_config;
	struct ssdfs_tunefs_config_request new_config;
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
#define SSDFS_IOC_LIST_SNAPSHOT_RULES	_IOWR(SSDFS_IOCTL_MAGIC, 8, \
					     struct ssdfs_snapshot_info)
#define SSDFS_IOC_TUNEFS_GET_CONFIG	_IOR(SSDFS_IOCTL_MAGIC, 9, \
					     struct ssdfs_tunefs_options)
#define SSDFS_IOC_TUNEFS_SET_CONFIG	_IOWR(SSDFS_IOCTL_MAGIC, 10, \
					     struct ssdfs_tunefs_options)

#define SSDFS_SEG_HDR(ptr) \
	((struct ssdfs_segment_header *)(ptr))
#define SSDFS_PL_HDR(ptr) \
	((struct ssdfs_partial_log_header *)(ptr))
#define SSDFS_LOG_FOOTER(ptr) \
	((struct ssdfs_log_footer *)(ptr))

/* Inline methods */

static inline
void SSDFS_CREATE_CONTENT_ITERATOR(struct ssdfs_raw_content_iterator *iter)
{
	iter->portion_offset = U32_MAX;
	iter->portion_size = 0;
	iter->fragment_index = -1;
	iter->fragment_size = 0;
	iter->item_offset = U32_MAX;
	iter->item_size = 0;
	iter->state = SSDFS_RAW_AREA_CONTENT_UNKNOWN_STATE;
}

static inline
void SSDFS_INIT_CONTENT_ITERATOR(struct ssdfs_raw_content_iterator *iter,
				 u32 portion_offset,
				 u32 portion_size,
				 int fragment_index,
				 u32 fragment_size,
				 u32 item_offset,
				 u32 item_size)
{
	iter->portion_offset = portion_offset;
	iter->portion_size = portion_size;
	iter->fragment_index = fragment_index;
	iter->fragment_size = fragment_size;
	iter->item_offset = item_offset;
	iter->item_size = item_size;
	iter->state = SSDFS_RAW_AREA_CONTENT_ITERATOR_INITIALIZED;
}

static inline
int SSDFS_CONTENT_ITERATOR_INCREMENT(struct ssdfs_raw_content_iterator *iter)
{
	switch (iter->state) {
	case SSDFS_RAW_AREA_CONTENT_ITERATOR_INITIALIZED:
		/* expected state */
		break;

	case SSDFS_RAW_AREA_CONTENT_PROCESSED:
		return -ENODATA;

	default:
		SSDFS_ERR("invalid iterator state %#x\n",
			  iter->state);
		return -EINVAL;
	}

	if ((iter->item_offset + iter->item_size) > iter->fragment_size) {
		SSDFS_ERR("invalid item size: "
			  "item_offset %u, item_size %u, "
			  "fragment_size %u\n",
			  iter->item_offset,
			  iter->item_size,
			  iter->fragment_size);
		return -ERANGE;
	}

	iter->item_offset += iter->item_size;

	if (iter->item_offset >= iter->fragment_size) {
		iter->state = SSDFS_RAW_AREA_CONTENT_PROCESSED;
		return -ENODATA;
	}

	return 0;
}

#define SSDFS_AREA_TYPE2INDEX(type)({ \
	int index; \
	switch (type) { \
	case SSDFS_LOG_BLK_DESC_AREA: \
		index = SSDFS_BLK_DESC_AREA_INDEX; \
		break; \
	case SSDFS_LOG_MAIN_AREA: \
		index = SSDFS_COLD_PAYLOAD_AREA_INDEX; \
		break; \
	case SSDFS_LOG_DIFFS_AREA: \
		index = SSDFS_WARM_PAYLOAD_AREA_INDEX; \
		break; \
	case SSDFS_LOG_JOURNAL_AREA: \
		index = SSDFS_HOT_PAYLOAD_AREA_INDEX; \
		break; \
	default: \
		BUG(); \
	}; \
	index; \
})

static inline
u32 SSDFS_AREA2BUFFER_SIZE(int area_index)
{
	u32 size = SSDFS_4KB;

	switch (area_index) {
//	case SSDFS_BLK_BMAP_INDEX:
//	case SSDFS_SNAPSHOT_RULES_AREA_INDEX:
//	case SSDFS_OFF_TABLE_INDEX:
//	case SSDFS_COLD_PAYLOAD_AREA_INDEX:
//	case SSDFS_WARM_PAYLOAD_AREA_INDEX:
//	case SSDFS_HOT_PAYLOAD_AREA_INDEX:
//	case SSDFS_BLK_DESC_AREA_INDEX:
//	case SSDFS_MAPTBL_CACHE_INDEX:
//	case SSDFS_LOG_FOOTER_INDEX:
//		break;

	default:
		/* do nothing */
		break;
	}

	return size;
}

static inline
void ssdfs_init_folder_environment(struct ssdfs_folder_environment *env)
{
	env->name = NULL;
	env->fd = -1;
	env->content.namelist = NULL;
	env->content.count = 0;
}

static inline
void ssdfs_init_file_environment(struct ssdfs_file_environment *env)
{
	env->fd = -1;
	env->inode_id = U64_MAX;
	env->content.buffer = NULL;
	env->content.size = 0;
}

static inline
int check_string(const char *str1, const char *str2)
{
	return strncasecmp(str1, str2, strlen(str2));
}

static inline
u64 detect_granularity(const char *str)
{
	if (check_string(str, SSDFS_256B_STRING) == 0)
		return SSDFS_256B;
	else if (check_string(str, SSDFS_512B_STRING) == 0)
		return SSDFS_512B;
	else if (check_string(str, SSDFS_1KB_STRING) == 0)
		return SSDFS_1KB;
	else if (check_string(str, SSDFS_2KB_STRING) == 0)
		return SSDFS_2KB;
	else if (check_string(str, SSDFS_4KB_STRING) == 0)
		return SSDFS_4KB;
	else if (check_string(str, SSDFS_8KB_STRING) == 0)
		return SSDFS_8KB;
	else if (check_string(str, SSDFS_16KB_STRING) == 0)
		return SSDFS_16KB;
	else if (check_string(str, SSDFS_32KB_STRING) == 0)
		return SSDFS_32KB;
	else if (check_string(str, SSDFS_64KB_STRING) == 0)
		return SSDFS_64KB;
	else if (check_string(str, SSDFS_128KB_STRING) == 0)
		return SSDFS_128KB;
	else if (check_string(str, SSDFS_256KB_STRING) == 0)
		return SSDFS_256KB;
	else if (check_string(str, SSDFS_512KB_STRING) == 0)
		return SSDFS_512KB;
	else if (check_string(str, SSDFS_1MB_STRING) == 0)
		return SSDFS_1MB;
	else if (check_string(str, SSDFS_2MB_STRING) == 0)
		return SSDFS_2MB;
	else if (check_string(str, SSDFS_4MB_STRING) == 0)
		return SSDFS_4MB;
	else if (check_string(str, SSDFS_8MB_STRING) == 0)
		return SSDFS_8MB;
	else if (check_string(str, SSDFS_16MB_STRING) == 0)
		return SSDFS_16MB;
	else if (check_string(str, SSDFS_32MB_STRING) == 0)
		return SSDFS_32MB;
	else if (check_string(str, SSDFS_64MB_STRING) == 0)
		return SSDFS_64MB;
	else if (check_string(str, SSDFS_128MB_STRING) == 0)
		return SSDFS_128MB;
	else if (check_string(str, SSDFS_256MB_STRING) == 0)
		return SSDFS_256MB;
	else if (check_string(str, SSDFS_512MB_STRING) == 0)
		return SSDFS_512MB;
	else if (check_string(str, SSDFS_1GB_STRING) == 0)
		return SSDFS_1GB;
	else if (check_string(str, SSDFS_2GB_STRING) == 0)
		return SSDFS_2GB;
	else if (check_string(str, SSDFS_8GB_STRING) == 0)
		return SSDFS_8GB;
	else if (check_string(str, SSDFS_16GB_STRING) == 0)
		return SSDFS_16GB;
	else if (check_string(str, SSDFS_32GB_STRING) == 0)
		return SSDFS_32GB;
	else if (check_string(str, SSDFS_64GB_STRING) == 0)
		return SSDFS_64GB;

	return U64_MAX;
}

static inline
int __check_pagesize(int pagesize)
{
	switch (pagesize) {
	case SSDFS_4KB:
	case SSDFS_8KB:
	case SSDFS_16KB:
	case SSDFS_32KB:
		/* do nothing: proper value */
		break;

	default:
		SSDFS_ERR("Unsupported page size %d. "
			  "Please, use 4KB, 8KB, 16KB, 32KB.\n",
			  pagesize);
		return -EOPNOTSUPP;
	}

	return 0;
}

static inline
int __check_segsize(u64 segsize)
{
	switch (segsize) {
	case SSDFS_128KB:
	case SSDFS_256KB:
	case SSDFS_512KB:
	case SSDFS_1MB:
	case SSDFS_2MB:
	case SSDFS_4MB:
	case SSDFS_8MB:
	case SSDFS_16MB:
	case SSDFS_32MB:
	case SSDFS_64MB:
	case SSDFS_128MB:
	case SSDFS_256MB:
	case SSDFS_512MB:
	case SSDFS_1GB:
	case SSDFS_2GB:
	case SSDFS_4GB:
	case SSDFS_8GB:
	case SSDFS_16GB:
	case SSDFS_32GB:
	case SSDFS_64GB:
		/* do nothing: proper value */
		break;

	default:
		SSDFS_ERR("Unsupported segment size %llu.\n",
			  segsize);
		return -EOPNOTSUPP;
	}

	return 0;
}

static inline
int __check_erasesize(u64 erasesize)
{
	switch (erasesize) {
	case SSDFS_128KB:
	case SSDFS_256KB:
	case SSDFS_512KB:
	case SSDFS_1MB:
	case SSDFS_2MB:
	case SSDFS_4MB:
	case SSDFS_8MB:
	case SSDFS_16MB:
	case SSDFS_32MB:
	case SSDFS_64MB:
	case SSDFS_128MB:
	case SSDFS_256MB:
	case SSDFS_512MB:
	case SSDFS_1GB:
	case SSDFS_2GB:
		/* do nothing: proper value */
		break;

	default:
		SSDFS_ERR("Unsupported erase size %llu.\n",
			  erasesize);
		return -EOPNOTSUPP;
	}

	return 0;
}

/* API declarations */

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
void ssdfs_nanoseconds_to_localtime(u64 nanoseconds,
				    struct tm *local_time);
int is_zoned_device(int fd);
int ssdfs_create_raw_buffer(struct ssdfs_raw_buffer *buf,
			    size_t buf_size);
int ssdfs_create_raw_area(struct ssdfs_raw_area *area,
			  u64 offset, u32 size);
int ssdfs_create_raw_area_environment(struct ssdfs_raw_area_environment *env,
				      u64 area_offset, u32 area_size,
				      size_t raw_buffer_size);
int ssdfs_create_raw_dump_environment(struct ssdfs_environment *env,
				struct ssdfs_raw_dump_environment *raw_dump);
void ssdfs_destroy_raw_buffer(struct ssdfs_raw_buffer *buf);
void ssdfs_destroy_raw_area(struct ssdfs_raw_area *area);
void ssdfs_destroy_raw_area_environment(struct ssdfs_raw_area_environment *env);
void ssdfs_destroy_raw_dump_environment(struct ssdfs_raw_dump_environment *env);
int ssdfs_read_area_content(struct ssdfs_environment *env,
			      u64 peb_id, u32 peb_size,
			      u32 area_offset, u32 size,
			      void *buf);
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
int ssdfs_read_mapping_table_cache(struct ssdfs_environment *env,
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
