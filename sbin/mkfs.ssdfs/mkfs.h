/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/mkfs.ssdfs/mkfs.h - declarations of mkfs.ssdfs (creation) utility.
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

#ifndef _SSDFS_UTILS_MKFS_H
#define _SSDFS_UTILS_MKFS_H

#ifdef mkfs_fmt
#undef mkfs_fmt
#endif

#include "version.h"

#define mkfs_fmt(fmt) "mkfs.ssdfs: " SSDFS_UTILS_VERSION ": " fmt

#include <pthread.h>

#include "ssdfs_tools.h"
#include "segbmap.h"

#define SSDFS_MKFS_INFO(show, fmt, ...) \
	do { \
		if (show) { \
			fprintf(stdout, mkfs_fmt(fmt), ##__VA_ARGS__); \
		} \
	} while (0)

#define SSDFS_CREATE_CNO		(0)
#define SSDFS_MKFS_UNKNOWN_THREADS	(0)
#define SSDFS_MKFS_DEFAULT_THREADS	(1)

enum {
	SSDFS_SEG_HEADER,
	SSDFS_BLOCK_BITMAP,
	SSDFS_OFFSET_TABLE,
	SSDFS_BLOCK_DESCRIPTORS,
	SSDFS_MAPTBL_CACHE,
	SSDFS_LOG_PAYLOAD,
	SSDFS_LOG_FOOTER,
	SSDFS_BLOCK_BITMAP_BACKUP,
	SSDFS_OFFSET_TABLE_BACKUP,
	SSDFS_SEG_LOG_ITEMS_COUNT
};

enum {
	SSDFS_DEDICATED_SEGMENT,
	SSDFS_SHARED_SEGMENT,
	SSDFS_ALLOC_POLICY_MAX
};

enum {
	SSDFS_INITIAL_SNAPSHOT,
	SSDFS_SUPERBLOCK,
	SSDFS_SEGBMAP,
	SSDFS_PEB_MAPPING_TABLE,
	SSDFS_USER_DATA,
	SSDFS_METADATA_ITEMS_MAX
};

#define SSDFS_DEFAULT_ALLOC_SEGS_COUNT (1)
#define SSDFS_DEFAULT_METADATA_QUOTA_MAX(segs_count) \
	(segs_count / 2)

/*
 * struct ssdfs_extent_desc - extent descriptor
 * @state: extent state [compressed or not compressed]
 * @buf: pointer on buffer with initialized data
 * @compr_buf: pointer on buffer with compressed data
 * @offset: offset from PEB's beginning in bytes
 * @bytes_count: bytes count in the extent
 * @compr_bytes: compressed size in bytes
 */
struct ssdfs_extent_desc {
	int state;
	char *buf;
	char *compr_buf;
	u32 offset;
	u32 bytes_count;
	u32 compr_bytes;
};

/*
 * struct ssdfs_peb_content - content of the PEB's log
 * @leb_id: LEB's identification number
 * @peb_id: PEB's identification number
 * @extents: array of extent descriptors
 */
struct ssdfs_peb_content {
	u64 leb_id;
	u64 peb_id;
	struct ssdfs_extent_desc extents[SSDFS_SEG_LOG_ITEMS_COUNT];
};

/*
 * struct ssdfs_segment_desc - description of prepared segment's content
 * @seg_type: segment type
 * @seg_state: state of segment (dedicated/shared)
 * @seg_id: segment number
 * @pebs: array of PEBs' content
 * @pebs_count: count of initialized PEBs in the array
 * @pebs_capacity: count of PEBs in segment
 */
struct ssdfs_segment_desc {
	int seg_type;
	int seg_state;
	u64 seg_id;
	struct ssdfs_peb_content *pebs;
	u16 pebs_count;
	u16 pebs_capacity;
};

/*
 * struct ssdfs_superblock_layout - superblock creation structure
 * @log_pages: count of pages in the full log of sb segment
 * @vh: volume header
 * @vs: volume state
 */
struct ssdfs_superblock_layout {
	/* creation options */
	u16 log_pages;

	/* layout */
	struct ssdfs_volume_header vh;
	struct ssdfs_volume_state vs;
};

/*
 * struct ssdfs_blkbmap_layout - block bitmap creation structure
 * @has_backup_copy: backup copy is present?
 * @compression: compression type
 */
struct ssdfs_blkbmap_layout {
	/* creation options */
	int has_backup_copy;
	int compression;
};

/*
 * struct ssdfs_blk2off_table_layout - offsets table creation structure
 * @has_backup_copy: backup copy is present?
 * @compression: compression type
 * @pages_per_seg: number of pages per segment
 */
struct ssdfs_blk2off_table_layout {
	/* creation options */
	int has_backup_copy;
	int compression;
	u32 pages_per_seg;
};

/*
 * struct ssdfs_segbmap_layout - segment bitmap creation structure
 * @has_backup_copy: backup copy is present?
 * @segs_per_chain: count of reserved (or allocated) segment in chain
 * @fragments_per_peb: count of segbmap's fragment per PEB
 * @pebs_per_seg: count of PEBs per segment
 * @log_pages: count of pages in the full log
 * @migration_threshold: max amount of migrating PEBs for segment
 * @compression: compression type
 * @fragments_count: count of fragments in segment bitmap
 * @fragment_size: size of fragment in bytes
 * @bmap_bytes: bitmap payload size in bytes
 * @fragments_array: array of pointers on PEBs' buffers
 */
struct ssdfs_segbmap_layout {
	/* creation options */
	int has_backup_copy;
	u16 segs_per_chain;
	u16 fragments_per_peb;
	u16 pebs_per_seg;
	u16 log_pages;
	u16 migration_threshold;
	int compression;

	/* layout */
	u32 fragments_count;
	size_t fragment_size;
	u32 bmap_bytes;
	void **fragments_array;
};

/*
 * struct ssdfs_maptbl_layout - PEB mapping table creation structure
 * @has_backup_copy: backup copy is present?
 * @stripes_per_portion: count of stripes in portion (from different dies)
 * @portions_per_peb: count of maptbl's portions per PEB
 * @log_pages: count of pages in the full log
 * @migration_threshold: max amount of migrating PEBs for segment
 * @reserved_pebs_per_fragment: percentage of reserved PEBs for fragment
 * @compression: compression type
 * @maptbl_pebs: count of PEBs for the whole mapping table
 * @lebtbl_portion_bytes: LEB table's fragments size for one maptbl portion
 * @lebtbl_portion_mempages: number of memory pages for LEB table's fragments
 * @pebtbl_portion_bytes: PEB table's fragments size for one maptbl portion
 * @pebtbl_portion_mempages: number of memory pages for PEB table's fragments
 * @lebs_per_portion: LEB descriptors in one portion
 * @pebs_per_portion: PEB descriptors in one portion
 * @portions_count: count of portions in mapping table
 * @portion_size: size of portion in bytes
 * @fragments_array: array of pointers on fragment's buffers
 */
struct ssdfs_maptbl_layout {
	/* creation options */
	int has_backup_copy;
	u16 stripes_per_portion;
	u16 portions_per_fragment;
	u16 log_pages;
	u16 migration_threshold;
	u16 reserved_pebs_per_fragment;
	int compression;

	/* layout */
	u32 maptbl_pebs;
	u32 lebtbl_portion_bytes;
	u32 lebtbl_portion_mempages;
	u32 pebtbl_portion_bytes;
	u32 pebtbl_portion_mempages;
	u16 lebs_per_portion;
	u16 pebs_per_portion;
	u32 portions_count;
	size_t portion_size;
	void **fragments_array;
};

/*
 * struct ssdfs_maptbl_cache_layout - PEB mapping table cache creation structure
 * @fragments_count: count of fragments in the maptbl cache
 * @fragment_size: size of fragment in bytes
 * @bytes_count: number of bytes in the whole maptbl cache
 * @fragments_array: array of pointers on buffers
 */
struct ssdfs_maptbl_cache_layout {
	/* layout */
	u32 fragments_count;
	size_t fragment_size;
	u32 bytes_count;
	void **fragments_array;
};

/*
 * struct ssdfs_btree_layout - btree creation options
 * @node_size: btree's node size in bytes
 * @min_index_area_size: minimal index area's size in bytes
 * @lnode_log_pages: leaf node's log pages
 * @hnode_log_pages: hybrid node's log pages
 * @inode_log_pages: index node's log pages
 */
struct ssdfs_btree_layout {
	/* creation options */
	u32 node_size;
	u16 min_index_area_size;
	u16 lnode_log_pages;
	u16 hnode_log_pages;
	u16 inode_log_pages;
};

/*
 * struct ssdfs_user_data_layout - user data creation options
 * @log_pages: count of pages in the full log
 * @migration_threshold: max amount of migrating PEBs for segment
 * @compression: compression type
 */
struct ssdfs_user_data_layout {
	/* creation options */
	u16 log_pages;
	u16 migration_threshold;
	int compression;
};

/*
 * struct ssdfs_metadata_desc - generalized metadata descriptor
 * @start_seg_index: starting index in segments array
 * @segs_count: count of contiguous segments in the array
 * @seg_state: state of segment (dedicated/shared)
 * @ptr: pointer on concrete metadata structure
 */
struct ssdfs_metadata_desc {
	int start_seg_index;
	int segs_count;
	int seg_state;
	void *ptr;
};

/*
 * struct ssdfs_write_buffer - write buffer
 * @ptr: pointer on write buffer
 * @offset: current offset in buffer
 * @capacity: capacity of buffer in bytes
 */
struct ssdfs_write_buffer {
	char *ptr;
	u32 offset;
	u32 capacity;
};

/*
 * struct ssdfs_volume_layout - description of created volume layout
 * @force_overwrite: force overwrite partition option
 * @need_erase_device: necessity in device erasure option
 * @seg_size: segment size in bytes
 * @page_size: page size in bytes
 * @nand_dies_count: NAND dies count on device
 * @lebs_per_peb_index: difference of LEB IDs between PEB indexes in segment
 * @volume_label: volume label string
 * @create_timestamp: timestamp of file system creation
 * @create_cno: checkpoint of file system creation
 * @migration_threshold: max amount of migrating PEBs for segment
 * @compression: compression type
 * @inode_size: inode size in bytes
 * @sb: superblock creation options
 * @blkbmap: block bitmap creation options
 * @segbmap: segment bitmap creation options
 * @maptbl: PEB mapping table creation options
 * @maptbl_cache: PEB mapping table cache creation options
 * @btree: btree creation options
 * @user_data_seg: user data segment's creation options
 * @meta_array: array of metadata descriptors
 * @segs: array of segment descriptors
 * @segs_capacity: maximum available count of descriptors in array
 * @last_allocated_seg_index: last allocated segment index
 * @segs_count: count of prepared segments
 * @calculated_open_zones: calculated number of open zones
 * @write_buf: buffer for alligned write of prepared metadata
 * @env: environment
 * @threads: threads environment
 * @is_volume_erased: inform that volume has been erased
 */
struct ssdfs_volume_layout {
	int force_overwrite;
	int need_erase_device;

	u64 seg_size;
	u32 page_size;
	u32 nand_dies_count;
	u32 lebs_per_peb_index;
	char volume_label[SSDFS_VOLUME_LABEL_MAX];
	u64 create_timestamp;
	u64 create_cno;
	u16 migration_threshold;
	int compression;
	u16 inode_size;

	struct ssdfs_superblock_layout sb;
	struct ssdfs_blkbmap_layout blkbmap;
	struct ssdfs_blk2off_table_layout blk2off_tbl;
	struct ssdfs_segbmap_layout segbmap;
	struct ssdfs_maptbl_layout maptbl;
	struct ssdfs_maptbl_cache_layout maptbl_cache;
	struct ssdfs_btree_layout btree;
	struct ssdfs_user_data_layout user_data_seg;

	struct ssdfs_metadata_desc meta_array[SSDFS_METADATA_ITEMS_MAX];

	struct ssdfs_segment_desc *segs;
	int segs_capacity;
	int last_allocated_seg_index;
	int segs_count;
	u32 calculated_open_zones;

	struct ssdfs_write_buffer write_buffer;

	struct ssdfs_environment env;
	struct ssdfs_threads_environment threads;
	int is_volume_erased;
};

/*
 * struct ssdfs_mkfs_operations - phases of creation volume's metadata
 *
 * Operations:
 * @allocation_policy: get allocation policy and segments count
 * @prepare: prepare metadata structure in memory
 * @validate: validate prepared metadata and to correct (if necessary)
 * @define_layout: define final placement of layout's items
 * @commit: place prepared metadata into segment(s)
 *
 * Arguments:
 * @ptr: pointer of volume layout structure
 * @segs: count of segments [out]
 */
struct ssdfs_mkfs_operations {
	int (*allocation_policy)(struct ssdfs_volume_layout *ptr, int *segs);
	int (*prepare)(struct ssdfs_volume_layout *ptr);
	int (*validate)(struct ssdfs_volume_layout *ptr);
	int (*define_layout)(struct ssdfs_volume_layout *ptr);
	int (*commit)(struct ssdfs_volume_layout *ptr);
};

#define OFF_DESC_PER_FRAGMENT() \
	((PAGE_CACHE_SIZE - sizeof(struct ssdfs_translation_extent) - \
		sizeof(struct ssdfs_phys_offset_table_header)) / \
			sizeof(struct ssdfs_phys_offset_descriptor))

#define OFF_DESC_HEADER_SIZE(fragments_count) \
	(sizeof(struct ssdfs_block_state_descriptor) + \
	 (fragments_count * sizeof(struct ssdfs_fragment_desc)))

#define BLK_DESC_PER_FRAGMENT() \
	((PAGE_CACHE_SIZE - sizeof(struct ssdfs_area_block_table)) / \
	 sizeof(struct ssdfs_block_descriptor))

#define BLK_DESC_TABLE_FRAGMENTS(blks_count) \
	((blks_count + BLK_DESC_PER_FRAGMENT() - 1) / BLK_DESC_PER_FRAGMENT())

#define BLK2OFF_TBL_HDR(ptr) \
	((struct ssdfs_blk2off_table_header *)(ptr))
#define TRANS_EXTENT(ptr) \
	((struct ssdfs_translation_extent *)(ptr))

static inline
int META2SEG_TYPE(int meta_index)
{
	switch (meta_index) {
	case SSDFS_INITIAL_SNAPSHOT:
		return SSDFS_INITIAL_SNAPSHOT_SEG_TYPE;

	case SSDFS_SUPERBLOCK:
		return SSDFS_SB_SEG_TYPE;

	case SSDFS_SEGBMAP:
		return SSDFS_SEGBMAP_SEG_TYPE;

	case SSDFS_PEB_MAPPING_TABLE:
		return SSDFS_MAPTBL_SEG_TYPE;
	}

	return SSDFS_UNKNOWN_SEG_TYPE;
}

static inline
void ssdfs_init_thread_descriptor(struct ssdfs_thread_state *state,
				  int thread_id)
{
	state->id = thread_id;
	state->err = 0;
}

static inline
void ssdfs_init_base_environment(struct ssdfs_thread_state *state,
				 struct ssdfs_environment *base)
{
	memcpy(&state->base, base, sizeof(struct ssdfs_environment));
}

static inline
void ssdfs_init_peb_environment(struct ssdfs_thread_state *state,
				int thread_id, u64 pebs_per_thread,
				u64 pebs_count, u32 erase_size)
{
	state->peb.id = (u64)thread_id * pebs_per_thread;
	state->peb.pebs_count = min_t(u64,
					pebs_count - state->peb.id,
					pebs_per_thread);
	state->peb.peb_size = erase_size;
}

static inline
int ssdfs_mkfs_init_thread_state(struct ssdfs_thread_state *state,
				 int thread_id,
				 struct ssdfs_environment *base,
				 u64 pebs_per_thread,
				 u64 pebs_count,
				 u32 erase_size)
{
	ssdfs_init_thread_descriptor(state, thread_id);
	ssdfs_init_base_environment(state, base);
	ssdfs_init_peb_environment(state, thread_id, pebs_per_thread,
				   pebs_count, erase_size);

	return 0;
}

static inline
void ssdfs_wait_threads_activity_ending(struct ssdfs_volume_layout *layout)
{
	int i;

	SSDFS_DBG(layout->env.show_debug,
		  "requested_jobs %u, tasks_capacity %u\n",
		  layout->threads.requested_jobs, layout->threads.capacity);

	for (i = 0; i < layout->threads.requested_jobs; i++) {
		pthread_join(layout->threads.jobs[i].thread, NULL);

		if (layout->threads.jobs[i].err != 0) {
			SSDFS_ERR("thread %d has failed: err %d\n",
				  i, layout->threads.jobs[i].err);
		}
	}
}

/* options.c */
void print_version(void);
void print_usage(void);
void parse_options(int argc, char *argv[],
		   struct ssdfs_volume_layout *layout);

/* common.c */
int reserve_segments(struct ssdfs_volume_layout *layout,
		     int meta_index);
int set_extent_start_offset(struct ssdfs_volume_layout *layout,
			    int seg_type,
			    struct ssdfs_peb_content *desc,
			    int extent_index);
u32 calculate_log_pages(struct ssdfs_volume_layout *layout,
			int seg_type,
			struct ssdfs_peb_content *desc);
u32 calculate_metadata_blks(struct ssdfs_volume_layout *layout,
			    int seg_type,
			    struct ssdfs_peb_content *desc);
int define_segment_header_layout(struct ssdfs_volume_layout *layout,
				 int seg_index, int peb_index);
int pre_commit_segment_header(struct ssdfs_volume_layout *layout,
			      int seg_index, int peb_index,
			      u16 seg_type);
void commit_segment_header(struct ssdfs_volume_layout *layout,
			   int seg_index, int peb_index,
			   u32 blks_count);
int pre_commit_block_bitmap(struct ssdfs_volume_layout *layout,
			    int seg_index, int peb_index,
			    size_t bytes_count,
			    u32 start_logical_blk, u16 blks_count);
void commit_block_bitmap(struct ssdfs_volume_layout *layout,
			 int seg_index, int peb_index,
			 u16 metadata_blks);
int pre_commit_block_bitmap_backup(struct ssdfs_volume_layout *layout,
				   int seg_index, int peb_index,
				   size_t bytes_count,
				   u32 start_logical_blk, u16 blks_count);
void commit_block_bitmap_backup(struct ssdfs_volume_layout *layout,
				int seg_index, int peb_index,
				u16 metadata_blks);
int pre_commit_offset_table(struct ssdfs_volume_layout *layout,
			    int seg_index, int peb_index,
			    u64 logical_byte_offset,
			    u32 start_logical_blk,
			    u16 valid_blks,
			    u32 used_logical_blks,
			    u32 last_allocated_blk);
void commit_offset_table(struct ssdfs_volume_layout *layout,
			 int seg_index, int peb_index);
int pre_commit_offset_table_backup(struct ssdfs_volume_layout *layout,
				   int seg_index, int peb_index,
				   u64 logical_byte_offset,
				   u32 start_logical_blk,
				   u16 valid_blks,
				   u32 used_logical_blks,
				   u32 last_allocated_blk);
void commit_offset_table_backup(struct ssdfs_volume_layout *layout,
				int seg_index, int peb_index);
int pre_commit_block_descriptors(struct ssdfs_volume_layout *layout,
				 int seg_index, int peb_index,
				 u32 start_logical_blk,
				 u16 valid_blks, u64 inode_id,
				 u32 payload_offset_in_bytes,
				 u32 item_size);
void commit_block_descriptors(struct ssdfs_volume_layout *layout,
			      int seg_index, int peb_index);
int define_log_footer_layout(struct ssdfs_volume_layout *layout,
			     int seg_index, int peb_index);
int pre_commit_log_footer(struct ssdfs_volume_layout *layout,
			  int seg_index, int peb_index);
void commit_log_footer(struct ssdfs_volume_layout *layout,
		       int seg_index, int peb_index,
		       u32 blks_count);

/* initial_snapshot.c */
int snap_mkfs_allocation_policy(struct ssdfs_volume_layout *layout,
				int *segs);
int snap_mkfs_prepare(struct ssdfs_volume_layout *layout);
int snap_mkfs_define_layout(struct ssdfs_volume_layout *layout);
int snap_mkfs_commit(struct ssdfs_volume_layout *layout);

/* superblock_segment.c */
int sb_mkfs_allocation_policy(struct ssdfs_volume_layout *layout,
				int *segs);
int sb_mkfs_prepare(struct ssdfs_volume_layout *layout);
int sb_mkfs_validate(struct ssdfs_volume_layout *layout);
int sb_mkfs_define_layout(struct ssdfs_volume_layout *layout);
int sb_mkfs_commit(struct ssdfs_volume_layout *layout);

/* segment_bitmap.c */
int segbmap_mkfs_allocation_policy(struct ssdfs_volume_layout *layout,
				   int *segs);
int segbmap_mkfs_prepare(struct ssdfs_volume_layout *layout);
int segbmap_mkfs_validate(struct ssdfs_volume_layout *layout);
int segbmap_mkfs_define_layout(struct ssdfs_volume_layout *layout);
int segbmap_mkfs_commit(struct ssdfs_volume_layout *layout);
void segbmap_destroy_fragments_array(struct ssdfs_volume_layout *layout);

/* mapping_table.c */
int maptbl_mkfs_allocation_policy(struct ssdfs_volume_layout *layout,
				   int *segs);
int maptbl_mkfs_prepare(struct ssdfs_volume_layout *layout);
int maptbl_mkfs_validate(struct ssdfs_volume_layout *layout);
int maptbl_mkfs_define_layout(struct ssdfs_volume_layout *layout);
int maptbl_mkfs_commit(struct ssdfs_volume_layout *layout);
void maptbl_destroy_fragments_array(struct ssdfs_volume_layout *layout);

/* mapping_table_cache.c */
int maptbl_cache_mkfs_prepare(struct ssdfs_volume_layout *layout);
int cache_leb2peb_pair(struct ssdfs_volume_layout *layout,
			u64 leb_id, u64 peb_id);
void maptbl_cache_destroy_fragments_array(struct ssdfs_volume_layout *layout);

#endif /* _SSDFS_UTILS_MKFS_H */
