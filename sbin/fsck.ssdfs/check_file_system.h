/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/fsck.ssdfs/check_file_system.h - declarations of structures for check step.
 *
 * Copyright (c) 2025-2026 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#ifndef _SSDFS_UTILS_CHECK_FILE_SYSTEM_H
#define _SSDFS_UTILS_CHECK_FILE_SYSTEM_H

enum {
	SSDFS_FSCK_BASE_SNAPSHOT_SEG_ID,
	SSDFS_FSCK_SUPERBLOCK_SEG_ID,
	SSDFS_FSCK_MAPPING_TABLE_ID,
	SSDFS_FSCK_SEGMENT_BITMAP_ID,
	SSDFS_FSCK_INODES_BTREE_ID,
	SSDFS_FSCK_DENTRIES_BTREE_ID,
	SSDFS_FSCK_EXTENTS_BTREE_ID,
	SSDFS_FSCK_SNAPSHOTS_BTREE_ID,
	SSDFS_FSCK_INVALIDATED_EXTENTS_BTREE_ID,
	SSDFS_FSCK_SHARED_DICTIONARY_BTREE_ID,
};

#define SSDFS_FSCK_UNKNOWN_ITEM_ID	(-1)

enum {
	SSDFS_FSCK_METADATA_ITEM_UNKNOWN_STATE,
	SSDFS_FSCK_METADATA_ITEM_OK,
	SSDFS_FSCK_METADATA_ITEM_CORRUPTED,
	SSDFS_FSCK_METADATA_ITEM_ABSENT,
	SSDFS_FSCK_METADATA_ITEM_CHECK_FAILED,
	SSDFS_FSCK_METADATA_ITEM_STATE_MAX
};

struct ssdfs_fsck_metadata_item_state {
	int metadata_id;
	int item_id;
	int state;
};

struct ssdfs_fsck_metadata_items_array {
	struct ssdfs_fsck_metadata_item_state *states;
	int capacity;
	int count;
	int checked;
	int corrupted;
};

/*
 * On-disk metadata IDs
 *
 * Every metadata structure keeps the items of its on-disk metadata
 * in metadata[SSDFS_FSCK_METADATA_ITEMS_MAX] array. The array is
 * two-dimensional: (1) on-disk metadata ID, (2) item ID.
 */
enum {
	SSDFS_FSCK_LOG_HDR_METADATA_ID,
	SSDFS_FSCK_LOG_FOOTER_METADATA_ID,
	SSDFS_FSCK_METADATA_ITEMS_MAX
};

enum {
	SSDFS_FSCK_METADATA_STRUCTURE_UNKNOWN_STATE,
	SSDFS_FSCK_METADATA_STRUCTURE_OK,
	SSDFS_FSCK_METADATA_STRUCTURE_CORRUPTED,
	SSDFS_FSCK_METADATA_STRUCTURE_ABSENT,
	SSDFS_FSCK_METADATA_STRUCTURE_CHECK_FAILED,
	SSDFS_FSCK_METADATA_STRUCTURE_STATE_MAX
};

enum {
	SSDFS_FSCK_HDR_SIGNATURE_ID,
	SSDFS_FSCK_HDR_VERSION_ID,
	SSDFS_FSCK_HDR_CHECKSUM_ID,
	SSDFS_FSCK_HDR_PAGESIZE_ID,
	SSDFS_FSCK_HDR_ERASESIZE_ID,
	SSDFS_FSCK_HDR_SEGSIZE_ID,
	SSDFS_FSCK_HDR_MEGS_PER_PEB_ID,
	SSDFS_FSCK_HDR_PEBS_PER_SEG_ID,
	SSDFS_FSCK_HDR_CREATE_TIME_ID,
	SSDFS_FSCK_HDR_CREATE_CNO_ID,
	SSDFS_FSCK_HDR_FLAGS_ID,
	SSDFS_FSCK_HDR_LEBS_PER_PEB_INDEX_ID,
	SSDFS_FSCK_HDR_SB_PEBS_ID,
	SSDFS_FSCK_HDR_SEGBMAP_ID,
	SSDFS_FSCK_HDR_MAPTBL_ID,
	SSDFS_FSCK_HDR_LOG_PAGES_ID,
	SSDFS_FSCK_HDR_CREATE_THREADS_ID,
	SSDFS_FSCK_HDR_DENTRIES_BTREE_ID,
	SSDFS_FSCK_HDR_EXTENTS_BTREE_ID,
	SSDFS_FSCK_HDR_XATTR_BTREE_ID,
	SSDFS_FSCK_HDR_INVEXTREE_ID,
	SSDFS_FSCK_HDR_UUID_ID,
	SSDFS_FSCK_HDR_LOG_TIMESTAMP_ID,
	SSDFS_FSCK_HDR_LOG_CNO_ID,
	SSDFS_FSCK_HDR_SEG_TYPE_ID,
	SSDFS_FSCK_HDR_SEG_FLAGS_ID,
	SSDFS_FSCK_HDR_DESC_ARRAY_ID,
	SSDFS_FSCK_HDR_PEB_MIGRATION_ID,
	SSDFS_FSCK_HDR_PEB_CREATE_TIME_ID,
	SSDFS_FSCK_HDR_RELATION_PEB_ID,
	SSDFS_FSCK_HDR_PL_FLAGS_ID,
	SSDFS_FSCK_HDR_LOG_BYTES_ID,
	SSDFS_FSCK_HDR_NSEGS_ID,
	SSDFS_FSCK_HDR_FREE_PAGES_ID,
	SSDFS_FSCK_HDR_ROOT_FOLDER_ID,
	SSDFS_FSCK_HDR_INODES_BTREE_ID,
	SSDFS_FSCK_HDR_SHEXTREE_ID,
	SSDFS_FSCK_HDR_SHDICT_BTREE_ID,
	SSDFS_FSCK_HDR_SEQUENCE_ID,
	SSDFS_FSCK_HDR_SNAP_BTREE_ID,
	SSDFS_FSCK_HDR_OPEN_ZONES_ID,
	SSDFS_FSCK_HDR_ITEMS_MAX
};

enum {
	SSDFS_FSCK_FOOTER_SIGNATURE_ID,
	SSDFS_FSCK_FOOTER_VERSION_ID,
	SSDFS_FSCK_FOOTER_CHECKSUM_ID,
	SSDFS_FSCK_FOOTER_NSEGS_ID,
	SSDFS_FSCK_FOOTER_FREE_PAGES_ID,
	SSDFS_FSCK_FOOTER_LOG_TIMESTAMP_ID,
	SSDFS_FSCK_FOOTER_LOG_CNO_ID,
	SSDFS_FSCK_FOOTER_FLAGS_ID,
	SSDFS_FSCK_FOOTER_STATE_ID,
	SSDFS_FSCK_FOOTER_ERRORS_ID,
	SSDFS_FSCK_FOOTER_FEATURES_ID,
	SSDFS_FSCK_FOOTER_UUID_ID,
	SSDFS_FSCK_FOOTER_CUR_SEGS_ID,
	SSDFS_FSCK_FOOTER_MIGRATION_THRESHOLD_ID,
	SSDFS_FSCK_FOOTER_BLKBMAP_OPTIONS_ID,
	SSDFS_FSCK_FOOTER_BLK2OFF_OPTIONS_ID,
	SSDFS_FSCK_FOOTER_USER_DATA_OPTIONS_ID,
	SSDFS_FSCK_FOOTER_OPEN_ZONES_ID,
	SSDFS_FSCK_FOOTER_ROOT_FOLDER_ID,
	SSDFS_FSCK_FOOTER_INODES_BTREE_ID,
	SSDFS_FSCK_FOOTER_SHEXTREE_ID,
	SSDFS_FSCK_FOOTER_SHDICT_BTREE_ID,
	SSDFS_FSCK_FOOTER_SNAP_BTREE_ID,
	SSDFS_FSCK_FOOTER_LOG_BYTES_ID,
	SSDFS_FSCK_FOOTER_LOG_FLAGSS_ID,
	SSDFS_FSCK_FOOTER_DESC_ARRAY_ID,
	SSDFS_FSCK_FOOTER_PEB_CREATE_TIME_ID,
	SSDFS_FSCK_FOOTER_ITEMS_MAX
};

struct ssdfs_fsck_base_snapshot_segment_corruption {
	int state;
	struct ssdfs_fsck_metadata_items_array metadata[SSDFS_FSCK_METADATA_ITEMS_MAX];
};

struct ssdfs_fsck_superblock_segment_corruption {
	int state;
	struct ssdfs_fsck_metadata_items_array metadata[SSDFS_FSCK_METADATA_ITEMS_MAX];
};

struct ssdfs_fsck_mapping_table_corruption {
	int state;
	struct ssdfs_fsck_metadata_items_array metadata[SSDFS_FSCK_METADATA_ITEMS_MAX];
};

struct ssdfs_fsck_segment_bitmap_corruption {
	int state;
	struct ssdfs_fsck_metadata_items_array metadata[SSDFS_FSCK_METADATA_ITEMS_MAX];
};

struct ssdfs_fsck_inodes_btree_corruption {
	int state;
	struct ssdfs_fsck_metadata_items_array metadata[SSDFS_FSCK_METADATA_ITEMS_MAX];
};

struct ssdfs_fsck_snapshots_btree_corruption {
	int state;
	struct ssdfs_fsck_metadata_items_array metadata[SSDFS_FSCK_METADATA_ITEMS_MAX];
};

struct ssdfs_fsck_invalidated_extents_btree_corruption {
	int state;
	struct ssdfs_fsck_metadata_items_array metadata[SSDFS_FSCK_METADATA_ITEMS_MAX];
};

struct ssdfs_fsck_shared_dictionary_btree_corruption {
	int state;
	struct ssdfs_fsck_metadata_items_array metadata[SSDFS_FSCK_METADATA_ITEMS_MAX];
};

/*
 * Corruption mask flags
 */
#define SSDFS_FSCK_BASE_SNAPSHOT_SEGMENT_CORRUPTED		(1 << 0)
#define SSDFS_FSCK_SUPERBLOCK_SEGMENT_CORRUPTED			(1 << 1)
#define SSDFS_FSCK_MAPPING_TABLE_CORRUPTED			(1 << 2)
#define SSDFS_FSCK_SEGMENT_BITMAP_CORRUPTED			(1 << 3)
#define SSDFS_FSCK_INODES_BTREE_CORRUPTED			(1 << 4)
#define SSDFS_FSCK_SNAPSHOTS_BTREE_CORRUPTED			(1 << 5)
#define SSDFS_FSCK_DENTRIES_BTREE_CORRUPTED			(1 << 6)
#define SSDFS_FSCK_EXTENTS_BTREE_CORRUPTED			(1 << 7)
#define SSDFS_FSCK_SHARED_EXTENTS_BTREE_CORRUPTED		(1 << 8)
#define SSDFS_FSCK_INVALID_EXTENTS_BTREE_CORRUPTED		(1 << 9)
#define SSDFS_FSCK_SHARED_DICT_BTREE_CORRUPTED			(1 << 10)
#define SSDFS_FSCK_XATTR_BTREE_CORRUPTED			(1 << 11)
#define SSDFS_FSCK_SHARED_XATTR_BTREE_CORRUPTED			(1 << 12)
#define SSDFS_FSCK_NOTHING_CORRUPTED_MASK			0x0

struct ssdfs_fsck_corruption_details {
	u64 mask;
	struct ssdfs_fsck_volume_creation_point *creation_point;
	struct ssdfs_fsck_base_snapshot_segment_corruption base_snapshot_seg;
	struct ssdfs_fsck_superblock_segment_corruption superblock_seg;
	struct ssdfs_fsck_mapping_table_corruption mapping_table;
	struct ssdfs_fsck_segment_bitmap_corruption segment_bitmap;
	struct ssdfs_fsck_inodes_btree_corruption inodes_btree;
	struct ssdfs_fsck_snapshots_btree_corruption snapshots_btree;
	struct ssdfs_fsck_invalidated_extents_btree_corruption invalid_extents;
	struct ssdfs_fsck_shared_dictionary_btree_corruption shared_dictionary;
};

/*
 * struct ssdfs_fsck_meta_extents - metadata extents
 * @name: name of metadata structure
 * @array: extents array [@rows][@chains]
 * @rows: number of extents (per chain)
 * @chains: number of chains (main and copy)
 * @has_copy: does the metadata structure have backup copy?
 *
 * The extents of metadata structure (segment bitmap, mapping table)
 * are stored in the volume header and can continue in the overflow
 * extents of superblock segment.
 */
struct ssdfs_fsck_meta_extents {
	const char *name;
	struct ssdfs_meta_area_extent *array;
	u32 rows;
	u32 chains;
	int has_copy;
};

/*
 * ssdfs_fsck_meta_extent() - get metadata extent
 * @extents: metadata extents
 * @row: extent's index in the chain
 * @chain: chain type (main or copy)
 */
static inline
struct ssdfs_meta_area_extent *
ssdfs_fsck_meta_extent(struct ssdfs_fsck_meta_extents *extents,
			u32 row, u32 chain)
{
	return &extents->array[(row * extents->chains) + chain];
}

typedef int (*check_metadata_item_func)(struct ssdfs_thread_state *state,
					struct ssdfs_fsck_metadata_item_state *item,
					int metadata_id,
					struct ssdfs_fsck_found_log *log);

/* check_segment_header.c */
int ssdfs_fsck_seg_header_version_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log);
int ssdfs_fsck_pagesize_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u8 log_pagesize);
int ssdfs_fsck_erasesize_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u8 log_pagesize, u8 log_erasesize);
int ssdfs_fsck_segsize_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u8 log_pagesize, u8 log_erasesize,
				u8 log_segsize);
int ssdfs_fsck_pebs_per_seg_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u8 log_erasesize,
				u8 log_segsize,
				u8 log_pebs_per_seg,
				u16 pebs_per_seg);
int ssdfs_fsck_create_time_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u64 create_time, u64 timestamp);
int ssdfs_fsck_lebs_per_peb_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u8 log_erasesize,
				u16 pebs_per_seg,
				u32 lebs_per_peb_index);
int ssdfs_fsck_check_log_pages(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				struct ssdfs_fsck_found_log *log,
				u64 pages_per_peb, u16 log_pages);
int ssdfs_fsck_create_threads_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u16 pebs_per_seg,
				u8 log_segsize,
				u8 log_erasesize,
				u16 create_threads_per_seg);
int ssdfs_fsck_btree_desc_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				struct ssdfs_fsck_found_log *log,
				u8 type,
				struct ssdfs_btree_descriptor *desc);
int ssdfs_fsck_uuid_check(struct ssdfs_thread_state *state,
			  struct ssdfs_fsck_metadata_item_state *item,
			  int metadata_id,
			  struct ssdfs_fsck_found_log *log,
			  const char *uuid_str);
int ssdfs_fsck_log_create_time_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u64 volume_create_time,
				u64 timestamp);
int ssdfs_fsck_log_create_cno_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u64 cno);
int ssdfs_fsck_seg_type_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u16 seg_type);
int ssdfs_fsck_seg_flags_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u32 seg_flags);
int ssdfs_fsck_desc_array_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				struct ssdfs_metadata_descriptor *desc_array,
				size_t array_capacity);

/* check_partial_log_header.c */
int ssdfs_fsck_log_bytes_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u16 log_pages, u8 log_pagesize,
				u8 log_erasesize, u32 log_bytes);
int ssdfs_fsck_nsegs_check(struct ssdfs_thread_state *state,
			   struct ssdfs_fsck_metadata_item_state *item,
			   int metadata_id,
			   struct ssdfs_fsck_found_log *log,
			   u8 log_segsize, u64 nsegs);
int ssdfs_fsck_free_pages_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u8 log_pagesize, u8 log_segsize,
				u64 nsegs, u64 free_pages);
int ssdfs_fsck_root_folder_check(struct ssdfs_thread_state *state,
				 struct ssdfs_fsck_metadata_item_state *item,
				 int metadata_id,
				 struct ssdfs_fsck_found_log *log,
				 struct ssdfs_inode *root_folder);
int ssdfs_fsck_open_zones_check(struct ssdfs_thread_state *state,
				struct ssdfs_fsck_metadata_item_state *item,
				int metadata_id,
				struct ssdfs_fsck_found_log *log,
				u32 open_zones, u64 nsegs);

/* check_file_system.c */
int ssdfs_fsck_init_metadata_items(struct ssdfs_fsck_metadata_items_array *items,
				   int capacity);
void ssdfs_fsck_destroy_metadata_items(struct ssdfs_fsck_metadata_items_array *items);
void ssdfs_fsck_destroy_metadata_items_array(struct ssdfs_fsck_metadata_items_array *metadata);
int ssdfs_fsck_metadata_checked_items(struct ssdfs_fsck_metadata_items_array *metadata);
int ssdfs_fsck_metadata_corrupted_items(struct ssdfs_fsck_metadata_items_array *metadata);

int ssdfs_fsck_check_segment_header(struct ssdfs_thread_state *state,
		struct ssdfs_fsck_metadata_items_array *metadata,
		struct ssdfs_fsck_found_log *log);
int ssdfs_fsck_check_partial_log_header(struct ssdfs_thread_state *state,
		struct ssdfs_fsck_metadata_items_array *metadata,
		struct ssdfs_fsck_found_log *log);
int ssdfs_fsck_check_log_footer(struct ssdfs_thread_state *state,
		struct ssdfs_fsck_metadata_items_array *metadata,
		struct ssdfs_fsck_found_log *log);

/*
 * ssdfs_fsck_log2_to_size() - convert on-disk log2 value into size
 * @log2: log2 value of size
 *
 * The on-disk log2 values can be corrupted. Shifting of int 1
 * by 31 gives negative value (that is sign extended into u64)
 * and shifting by 32 or more is undefined behavior.
 *
 * RETURN: size or 0, if @log2 is too big.
 */
static inline
u64 ssdfs_fsck_log2_to_size(u32 log2)
{
	if (log2 >= 64)
		return 0;

	return (u64)1 << log2;
}

/*
 * ssdfs_fsck_log_page_size() - get page size of the log
 * @base: base environment (detected volume geometry)
 * @hdr: segment header of the log
 *
 * The page size of the segment header is used to walk through
 * the logs of the PEB. But it can be corrupted. In this case,
 * the page size of detected volume geometry is used.
 */
static inline
u32 ssdfs_fsck_log_page_size(struct ssdfs_environment *base,
			     struct ssdfs_segment_header *hdr)
{
	u8 log_pagesize = hdr->volume_hdr.log_pagesize;
	u64 page_size = ssdfs_fsck_log2_to_size(log_pagesize);

	if (page_size < SSDFS_4KB || page_size > SSDFS_128KB ||
	    page_size >= base->erase_size)
		return base->page_size;

	return (u32)page_size;
}

#endif /* _SSDFS_UTILS_CHECK_FILE_SYSTEM_H */
