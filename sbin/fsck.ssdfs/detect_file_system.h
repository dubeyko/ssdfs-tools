/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/fsck.ssdfs/detect_file_system.h - declarations of structures for detection step.
 *
 * Copyright (c) 2025 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#ifndef _SSDFS_UTILS_DETECT_FILE_SYSTEM_H
#define _SSDFS_UTILS_DETECT_FILE_SYSTEM_H

#define SSDFS_FSCK_BASE_SNAPSHOT_SEG_FOUND		(1 << 0)
#define SSDFS_FSCK_SB_SEGS_FOUND			(1 << 1)
#define SSDFS_FSCK_MAPPING_TBL_FOUND			(1 << 2)
#define SSDFS_FSCK_SEGMENT_BITMAP_FOUND			(1 << 3)
#define SSDFS_FSCK_MAPTBL_CACHE_FOUND			(1 << 4)
#define SSDFS_FSCK_NOTHING_FOUND_MASK			0x0
#define SSDFS_FSCK_ALL_CRITICAL_METADATA_FOUND_MASK	0x1F

struct ssdfs_fsck_found_log {
	u64 peb_id;
	u32 start_page;
	union ssdfs_metadata_header header;
	union ssdfs_metadata_footer footer;
};

struct ssdfs_fsck_base_snapshot_segment_detection {
	struct ssdfs_fsck_found_log log;
};

struct ssdfs_fsck_superblock_segment_detection {
	struct ssdfs_fsck_found_log logs[SSDFS_SB_SEG_COPY_MAX];
};

enum {
	SSDFS_FSCK_MAIN_LOG,
	SSDFS_FSCK_COPY_LOG,
	SSDFS_FSCK_LOGS_NUMBER_MAX
};

struct ssdfs_fsck_logs_pair {
	struct ssdfs_fsck_found_log logs[SSDFS_FSCK_LOGS_NUMBER_MAX];
};

struct ssdfs_fsck_logs_pair_array {
	struct ssdfs_fsck_logs_pair *pairs;
	u32 count;
	u32 capacity;
};

struct ssdfs_fsck_segment_bitmap_detection {
	struct ssdfs_fsck_logs_pair_array array;
	struct ssdfs_segbmap_sb_header segbmap_sb_hdr;
	u32 segs_count;
};

struct ssdfs_fsck_mapping_table_detection {
	struct ssdfs_fsck_logs_pair_array array;
	struct ssdfs_maptbl_sb_header maptbl_sb_hdr;
	u32 segs_count;
	u32 found_segs;
	u32 extents_count;
};

struct ssdfs_fsck_mapping_table_cache {
	struct ssdfs_maptbl_cache_header hdr;
	void *data;
	u32 bytes_count;
};

struct ssdfs_fsck_volume_creation_point {
	u64 found_metadata;
	struct ssdfs_fsck_base_snapshot_segment_detection base_snapshot_seg;
	struct ssdfs_fsck_superblock_segment_detection superblock_seg;
	struct ssdfs_fsck_segment_bitmap_detection segbmap;
	struct ssdfs_fsck_mapping_table_detection maptbl;
	struct ssdfs_fsck_mapping_table_cache maptbl_cache;
};

enum {
	SSDFS_FSCK_CREATION_ARRAY_NOT_INITIALIZED,
	SSDFS_FSCK_CREATION_ARRAY_USE_BUFFER,
	SSDFS_FSCK_CREATION_ARRAY_ALLOCATED,
	SSDFS_FSCK_CREATION_ARRAY_STATE_MAX
};

struct ssdfs_fsck_volume_creation_array {
	int state;
	int count;
	struct ssdfs_fsck_volume_creation_point *creation_points;
	struct ssdfs_fsck_volume_creation_point buf;
};

/* Inline functions */

static inline
void ssdfs_fsck_init_found_log(struct ssdfs_fsck_found_log *log)
{
	log->peb_id = U64_MAX;
	log->start_page = U32_MAX;
}

static inline
void ssdfs_fsck_init_base_snapshot_seg(struct ssdfs_fsck_volume_creation_point *ptr)
{
	ssdfs_fsck_init_found_log(&ptr->base_snapshot_seg.log);
}

static inline
void ssdfs_fsck_destroy_base_snapshot_seg(struct ssdfs_fsck_volume_creation_point *ptr)
{
	ssdfs_fsck_init_found_log(&ptr->base_snapshot_seg.log);
}

static inline
void ssdfs_fsck_init_superblock_seg(struct ssdfs_fsck_volume_creation_point *ptr)
{
	ssdfs_fsck_init_found_log(&ptr->superblock_seg.logs[SSDFS_MAIN_SB_SEG]);
	ssdfs_fsck_init_found_log(&ptr->superblock_seg.logs[SSDFS_COPY_SB_SEG]);
}

static inline
void ssdfs_fsck_destroy_superblock_seg(struct ssdfs_fsck_volume_creation_point *ptr)
{
	ssdfs_fsck_init_found_log(&ptr->superblock_seg.logs[SSDFS_MAIN_SB_SEG]);
	ssdfs_fsck_init_found_log(&ptr->superblock_seg.logs[SSDFS_COPY_SB_SEG]);
}

static inline
void ssdfs_fsck_init_logs_pair_array(struct ssdfs_fsck_logs_pair_array *array)
{
	array->pairs = NULL;
	array->count = 0;
	array->capacity = 0;
}

static inline
void ssdfs_fsck_destroy_logs_pair_array(struct ssdfs_fsck_logs_pair_array *array)
{
	if (array->pairs) {
		free(array->pairs);
		array->pairs = NULL;
	}

	array->count = 0;
	array->capacity = 0;
}

static inline
void ssdfs_fsck_init_segbmap(struct ssdfs_fsck_volume_creation_point *ptr)
{
	ptr->segbmap.segs_count = 0;
	ssdfs_fsck_init_logs_pair_array(&ptr->segbmap.array);
}

static inline
void ssdfs_fsck_destroy_segbmap(struct ssdfs_fsck_volume_creation_point *ptr)
{
	ptr->segbmap.segs_count = 0;
	ssdfs_fsck_destroy_logs_pair_array(&ptr->segbmap.array);
}

static inline
void ssdfs_fsck_init_maptbl(struct ssdfs_fsck_volume_creation_point *ptr)
{
	ptr->maptbl.segs_count = 0;
	ptr->maptbl.found_segs = 0;
	ptr->maptbl.extents_count = 0;
	ssdfs_fsck_init_logs_pair_array(&ptr->maptbl.array);
}

static inline
void ssdfs_fsck_destroy_maptbl(struct ssdfs_fsck_volume_creation_point *ptr)
{
	ptr->maptbl.segs_count = 0;
	ptr->maptbl.found_segs = 0;
	ptr->maptbl.extents_count = 0;
	ssdfs_fsck_destroy_logs_pair_array(&ptr->maptbl.array);
}

static inline
void ssdfs_fsck_init_maptbl_cache(struct ssdfs_fsck_volume_creation_point *ptr)
{
	ptr->maptbl_cache.data = NULL;
	ptr->maptbl_cache.bytes_count = 0;
}

static inline
void ssdfs_fsck_destroy_maptbl_cache(struct ssdfs_fsck_volume_creation_point *ptr)
{
	if (ptr->maptbl_cache.data) {
		free(ptr->maptbl_cache.data);
		ptr->maptbl_cache.data = NULL;
	}

	ptr->maptbl_cache.bytes_count = 0;
}

#endif /* _SSDFS_UTILS_DETECT_FILE_SYSTEM_H */
