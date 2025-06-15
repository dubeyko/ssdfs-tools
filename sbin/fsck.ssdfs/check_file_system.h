/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/fsck.ssdfs/check_file_system.h - declarations of structures for check step.
 *
 * Copyright (c) 2025 Viacheslav Dubeyko <slava@dubeyko.com>
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

enum {
	SSDFS_FSCK_METADATA_STRUCTURE_UNKNOWN_STATE,
	SSDFS_FSCK_METADATA_STRUCTURE_OK,
	SSDFS_FSCK_METADATA_STRUCTURE_CORRUPTED,
	SSDFS_FSCK_METADATA_STRUCTURE_ABSENT,
	SSDFS_FSCK_METADATA_STRUCTURE_CHECK_FAILED,
	SSDFS_FSCK_METADATA_STRUCTURE_STATE_MAX
};

struct ssdfs_fsck_base_snapshot_segment_corruption {
	int state;
	struct ssdfs_fsck_metadata_items_array items;
};

struct ssdfs_fsck_superblock_segment_corruption {
	int state;
	struct ssdfs_fsck_metadata_items_array items;
};

struct ssdfs_fsck_mapping_table_corruption {
	int state;
	struct ssdfs_fsck_metadata_items_array items;
};

struct ssdfs_fsck_segment_bitmap_corruption {
	int state;
	struct ssdfs_fsck_metadata_items_array items;
};

struct ssdfs_fsck_inodes_btree_corruption {
	int state;
	struct ssdfs_fsck_metadata_items_array items;
};

struct ssdfs_fsck_snapshots_btree_corruption {
	struct ssdfs_fsck_metadata_items_array items;
};

struct ssdfs_fsck_invalidated_extents_btree_corruption {
	int state;
	struct ssdfs_fsck_metadata_items_array items;
};

struct ssdfs_fsck_shared_dictionary_btree_corruption {
	int state;
	struct ssdfs_fsck_metadata_items_array items;
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
	struct ssdfs_fsck_invalid_extents_btree_corruption invalid_extents;
	struct ssdfs_fsck_shared_dictionary_btree_corruption shared_dictionary;
};

#endif /* _SSDFS_UTILS_CHECK_FILE_SYSTEM_H */
