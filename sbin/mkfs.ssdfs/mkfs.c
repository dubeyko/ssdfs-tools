//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/mkfs.ssdfs/mkfs.c - implementation of mkfs.ssdfs (creation) utility.
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

#include "mkfs.h"
#include <mtd/mtd-abi.h>

/* Volume layout description */
static struct ssdfs_volume_layout volume_layout = {
	.force_overwrite = SSDFS_FALSE,
	.need_erase_device = SSDFS_TRUE,
	.env.show_debug = SSDFS_FALSE,
	.env.show_info = SSDFS_TRUE,
	.seg_size = SSDFS_8MB,
	.env.erase_size = SSDFS_8MB,
	.page_size = SSDFS_4KB,
	.nand_dies_count = SSDFS_NAND_DIES_DEFAULT,
	.migration_threshold = U16_MAX,
	.compression = SSDFS_ZLIB_BLOB,
	.inode_size = sizeof(struct ssdfs_inode),
	.sb.log_pages = U16_MAX,
	.blkbmap.has_backup_copy = SSDFS_FALSE,
	.blkbmap.compression = SSDFS_UNKNOWN_COMPRESSION,
	.blk2off_tbl.has_backup_copy = SSDFS_FALSE,
	.blk2off_tbl.compression = SSDFS_UNKNOWN_COMPRESSION,
	.blk2off_tbl.pages_per_seg = U32_MAX,
	.segbmap.has_backup_copy = SSDFS_FALSE,
	.segbmap.segs_per_chain = SSDFS_SEGBMAP_SEGS_PER_CHAIN_DEFAULT,
	.segbmap.fragments_per_peb = SSDFS_SEGBMAP_FRAG_PER_PEB_DEFAULT,
	.segbmap.log_pages = U16_MAX,
	.segbmap.migration_threshold = U16_MAX,
	.segbmap.compression = SSDFS_UNKNOWN_COMPRESSION,
	.segbmap.fragments_count = 0,
	.segbmap.fragment_size = PAGE_CACHE_SIZE,
	.segbmap.fragments_array = NULL,
	.maptbl.has_backup_copy = SSDFS_FALSE,
	.maptbl.stripes_per_portion = SSDFS_MAPTBL_STRIPES_PER_FRAG_DEFAULT,
	.maptbl.portions_per_fragment = SSDFS_MAPTBL_FRAG_PER_PEB_DEFAULT,
	.maptbl.log_pages = U16_MAX,
	.maptbl.migration_threshold = U16_MAX,
	.maptbl.reserved_pebs_per_fragment = U16_MAX,
	.maptbl.compression = SSDFS_UNKNOWN_COMPRESSION,
	.btree.node_size = SSDFS_8KB,
	.btree.min_index_area_size = 0,
	.btree.lnode_log_pages = U16_MAX,
	.btree.hnode_log_pages = U16_MAX,
	.btree.inode_log_pages = U16_MAX,
	.user_data_seg.log_pages = U16_MAX,
	.user_data_seg.migration_threshold = U16_MAX,
	.user_data_seg.compression = SSDFS_UNKNOWN_COMPRESSION,
	.env.device_type = SSDFS_DEVICE_TYPE_MAX,
	.write_buffer.ptr = NULL,
	.write_buffer.offset = 0,
	.write_buffer.capacity = 0,
	.is_volume_erased = SSDFS_FALSE,
};

/* Metadata items creation operations set */
static struct ssdfs_mkfs_operations *mkfs_ops[SSDFS_METADATA_ITEMS_MAX];

/************************************************************************
 *                    User data segment configuration                   *
 ************************************************************************/

static int user_data_mkfs_validate(struct ssdfs_volume_layout *layout)
{
	u64 seg_size = layout->seg_size;
	u32 erase_size = layout->env.erase_size;
	u32 pebs_per_seg = (u32)(seg_size / erase_size);

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	if (layout->user_data_seg.migration_threshold >= U16_MAX) {
		layout->user_data_seg.migration_threshold =
					layout->migration_threshold;
	} else if (layout->user_data_seg.migration_threshold > pebs_per_seg) {
		SSDFS_WARN("user data migration threshold %u "
			   "was corrected to %u\n",
			   layout->user_data_seg.migration_threshold,
			   pebs_per_seg);
		layout->user_data_seg.migration_threshold = pebs_per_seg;
	}

	return 0;
}

static inline
int prepare_user_data_options(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_volume_state *vs = &layout->sb.vs;
	u16 flags = 0;
	u8 compression = SSDFS_USER_DATA_NOCOMPR_TYPE;

	switch (layout->user_data_seg.compression) {
	case SSDFS_UNCOMPRESSED_BLOB:
		/* do nothing */
		break;

	case SSDFS_ZLIB_BLOB:
		flags |= SSDFS_USER_DATA_MAKE_COMPRESSION;
		compression = SSDFS_USER_DATA_ZLIB_COMPR_TYPE;
		break;

	case SSDFS_LZO_BLOB:
		flags |= SSDFS_USER_DATA_MAKE_COMPRESSION;
		compression = SSDFS_USER_DATA_LZO_COMPR_TYPE;
		break;

	default:
		SSDFS_ERR("invalid compression type %#x\n",
			  layout->user_data_seg.compression);
		return -ERANGE;
	}

	vs->user_data.flags = cpu_to_le16(flags);
	vs->user_data.compression = cpu_to_le8(compression);
	vs->user_data.migration_threshold =
		cpu_to_le16(layout->user_data_seg.migration_threshold);

	return 0;
}

static int user_data_mkfs_define_layout(struct ssdfs_volume_layout *layout)
{
	u32 erasesize;
	u32 pagesize;
	u32 pages_per_peb;
	u32 log_pages = layout->user_data_seg.log_pages;
	int err;

	BUG_ON(log_pages == 0);

	SSDFS_DBG(layout->env.show_debug,
		  "log_pages %u\n",
		  layout->user_data_seg.log_pages);

	erasesize = layout->env.erase_size;
	pagesize = layout->page_size;
	pages_per_peb = erasesize / pagesize;

	if (log_pages >= U16_MAX) {
		log_pages = pages_per_peb / SSDFS_DATA_LOGS_PER_PEB_DEFAULT;
		log_pages = min_t(u32, log_pages, (u32)SSDFS_LOG_MAX_PAGES);
		layout->user_data_seg.log_pages = (u16)log_pages;
	}

	if (log_pages > pages_per_peb) {
		SSDFS_WARN("invalid user data segment option: "
			   "log_pages %u will be changed on "
			   "pages_per_peb %u\n",
			   log_pages, pages_per_peb);

		log_pages = pages_per_peb;
		log_pages = min_t(u32, log_pages, (u32)SSDFS_LOG_MAX_PAGES);
		layout->user_data_seg.log_pages = (u16)log_pages;
	}

	if (pages_per_peb % log_pages) {
		u16 corrected_value;

		corrected_value = 1 << ilog2(log_pages);

		BUG_ON(pages_per_peb % corrected_value);

		SSDFS_WARN("invalid user data segment option: "
			   "log_pages %u will be changed on "
			   "corrected_value %u\n",
			   log_pages, corrected_value);

		log_pages = corrected_value;
		log_pages = min_t(u32, log_pages, (u32)SSDFS_LOG_MAX_PAGES);
		layout->user_data_seg.log_pages = (u16)log_pages;
	}

	BUG_ON(log_pages >= U16_MAX);

	layout->sb.vh.user_data_log_pages = cpu_to_le16((u16)log_pages);

	err = prepare_user_data_options(layout);
	if (err) {
		SSDFS_ERR("fail to prepare user data options: "
			  "err %d\n", err);
		return err;
	}

	return 0;
}

/************************************************************************
 *                    Metadata creation operations                      *
 ************************************************************************/

static struct ssdfs_mkfs_operations initial_snapshot_mkfs_ops = {
	.allocation_policy = snap_mkfs_allocation_policy,
	.prepare = snap_mkfs_prepare,
	/*.validate = snap_mkfs_validate,*/
	.define_layout = snap_mkfs_define_layout,
	.commit = snap_mkfs_commit,
};

static struct ssdfs_mkfs_operations sb_mkfs_ops = {
	.allocation_policy = sb_mkfs_allocation_policy,
	.prepare = sb_mkfs_prepare,
	.validate = sb_mkfs_validate,
	.define_layout = sb_mkfs_define_layout,
	.commit = sb_mkfs_commit,
};

static struct ssdfs_mkfs_operations segbmap_mkfs_ops = {
	.allocation_policy = segbmap_mkfs_allocation_policy,
	.prepare = segbmap_mkfs_prepare,
	.validate = segbmap_mkfs_validate,
	.define_layout = segbmap_mkfs_define_layout,
	.commit = segbmap_mkfs_commit,
};

static struct ssdfs_mkfs_operations maptbl_mkfs_ops = {
	.allocation_policy = maptbl_mkfs_allocation_policy,
	.prepare = maptbl_mkfs_prepare,
	.validate = maptbl_mkfs_validate,
	.define_layout = maptbl_mkfs_define_layout,
	.commit = maptbl_mkfs_commit,
};

static struct ssdfs_mkfs_operations user_data_mkfs_ops = {
	.validate = user_data_mkfs_validate,
	.define_layout = user_data_mkfs_define_layout,
};

static void prepare_metadata_creation_ops(void)
{
	mkfs_ops[SSDFS_INITIAL_SNAPSHOT] = &initial_snapshot_mkfs_ops;
	mkfs_ops[SSDFS_SUPERBLOCK] = &sb_mkfs_ops;
	mkfs_ops[SSDFS_SEGBMAP] = &segbmap_mkfs_ops;
	mkfs_ops[SSDFS_PEB_MAPPING_TABLE] = &maptbl_mkfs_ops;
	mkfs_ops[SSDFS_USER_DATA] = &user_data_mkfs_ops;
}

/************************************************************************
 *                       Base mkfs algorithm                            *
 ************************************************************************/

static int validate_key_creation_options(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_nand_geometry info;
	u64 fs_size = layout->env.fs_size;
	u64 seg_size = layout->seg_size;
	u32 erase_size = layout->env.erase_size;
	u32 page_size = layout->page_size;
	u64 segs_count;
	u32 pebs_per_seg;
	u32 pages_per_seg;
	int res;

	SSDFS_DBG(layout->env.show_debug,
		  "BEFORE_CHECK: fs_size %llu, seg_size %llu, "
		  "erase_size %u, page_size %u\n",
		  (unsigned long long)fs_size, seg_size,
		  erase_size, page_size);

	if (page_size >= erase_size) {
		SSDFS_ERR("page size %u can't be equal/greater than erase size %u.\n",
			  page_size, erase_size);
		return -EINVAL;
	}

	if ((erase_size % page_size) != 0) {
		SSDFS_ERR("erase size %u should be aligned on page size %u.\n",
			  erase_size, page_size);
		return -EINVAL;
	}

	if (seg_size < erase_size) {
		SSDFS_ERR("segment size %llu can't be lesser than erase size %u.\n",
			  seg_size, erase_size);
		return -EINVAL;
	}

	if ((seg_size % erase_size) != 0) {
		SSDFS_ERR("segment size %llu should be aligned on erase size %u.\n",
			  seg_size, erase_size);
		return -EINVAL;
	}

	if (fs_size <= seg_size) {
		SSDFS_ERR("fs size %llu can't be equal/lesser than segment size %llu.\n",
			  (unsigned long long)fs_size, seg_size);
		return -EINVAL;
	}

	switch (layout->env.device_type) {
	case SSDFS_ZNS_DEVICE:
		info.erasesize = layout->env.erase_size;
		info.writesize = layout->page_size;

		res = layout->env.dev_ops->check_nand_geometry(layout->env.fd,
							&info,
							layout->env.show_debug);
		if (res == -ENOENT) {
			layout->env.erase_size = info.erasesize;
			layout->page_size = info.writesize;

			SSDFS_INFO("NAND geometry corrected: "
				   "erase_size %u, write_size %u\n",
				   info.erasesize, info.writesize);

			erase_size = layout->env.erase_size;
			page_size = layout->page_size;
		} else if (res != 0)
			return res;

		if (seg_size != erase_size) {
			layout->seg_size = layout->env.erase_size;

			SSDFS_INFO("segment size corrected: "
				   "seg_size %llu, erase_size %u\n",
				   layout->seg_size,
				   layout->env.erase_size);

			seg_size = layout->seg_size;
		}
		break;

	default:
		/* do nothing */
		break;
	};

	segs_count = fs_size / seg_size;
	layout->env.fs_size = segs_count * seg_size;

	if (layout->env.fs_size != fs_size && layout->env.show_info) {
		SSDFS_WARN("device size %llu was corrected to fs size %llu "
			   "because of segment size %llu\n",
			   fs_size, layout->env.fs_size, seg_size);
	}

	pebs_per_seg = seg_size / erase_size;

	if (layout->migration_threshold >= U16_MAX)
		layout->migration_threshold = pebs_per_seg;
	else if (layout->migration_threshold > pebs_per_seg) {
		SSDFS_WARN("migration threshold %u was corrected to %u\n",
			   layout->migration_threshold, pebs_per_seg);
		layout->migration_threshold = pebs_per_seg;
	}

	pages_per_seg = seg_size / page_size;
	if (pages_per_seg >= U32_MAX) {
		SSDFS_ERR("pages_per_seg %u is too huge\n",
			  pages_per_seg);
		return -EINVAL;
	}

	layout->blk2off_tbl.pages_per_seg = pages_per_seg;

	if (layout->blkbmap.compression == SSDFS_UNKNOWN_COMPRESSION)
		layout->blkbmap.compression = layout->compression;

	if (layout->blk2off_tbl.compression == SSDFS_UNKNOWN_COMPRESSION)
		layout->blk2off_tbl.compression = layout->compression;

	if (layout->segbmap.compression == SSDFS_UNKNOWN_COMPRESSION)
		layout->segbmap.compression = layout->compression;

	if (layout->maptbl.compression == SSDFS_UNKNOWN_COMPRESSION)
		layout->maptbl.compression = layout->compression;

	if (layout->user_data_seg.compression == SSDFS_UNKNOWN_COMPRESSION)
		layout->user_data_seg.compression = layout->compression;

	SSDFS_DBG(layout->env.show_debug, "AFTER_CHECK: fs_size %llu\n",
		  (unsigned long long)layout->env.fs_size);

	return 0;
}

static int is_device_mounted(struct ssdfs_volume_layout *layout)
{
	FILE *fp;
	char line[NAME_MAX + 1];
	const char *device = layout->env.dev_name;
	size_t name_len = strlen(layout->env.dev_name);

	SSDFS_DBG(layout->env.show_debug, "fd %d, device %s\n",
		  layout->env.fd, layout->env.dev_name);

	fp = fopen(_PATH_MOUNTED, "r");
	if (fp == NULL) {
		SSDFS_ERR("unable to open %s\n", _PATH_MOUNTED);
		return SSDFS_TRUE;
	}

	while (fgets(line, NAME_MAX + 1, fp) != NULL) {
		if (strncmp(strtok(line, " "), device, name_len) == 0) {
			fclose(fp);
			SSDFS_ERR("%s is currently mounted. "
				  "You can't make a filesystem here.\n",
				  layout->env.dev_name);
			return SSDFS_TRUE;
		}
	}

	fclose(fp);
	return SSDFS_FALSE;
}

static int is_safe_overwrite_device(struct ssdfs_volume_layout *layout)
{
	int c, c_next;
	blkid_probe pr = NULL;
	blkid_loff_t size;
	int ret = 0;

	SSDFS_DBG(layout->env.show_debug, "fd %d, device %s\n",
		  layout->env.fd, layout->env.dev_name);

	if (layout->force_overwrite == SSDFS_FALSE) {
		pr = blkid_new_probe_from_filename(layout->env.dev_name);
		if (!pr) {
			ret = -1;
			goto end_check;
		}

		size = blkid_probe_get_size(pr);
		if (size <= 0)
			goto end_check;

		ret = blkid_probe_enable_partitions(pr, 1);
		if (ret < 0)
			goto end_check;

		ret = blkid_do_fullprobe(pr);
		if (ret < 0) /* error */
			goto end_check;
		else if (ret == 0) { /* some signature was found */
			const char *type;

			if (!blkid_probe_lookup_value(pr, "TYPE",
							&type, NULL)) {
				SSDFS_MKFS_INFO(SSDFS_TRUE,
						"Device %s appears to contain"
						" an existing %s superblock\n",
						layout->env.dev_name, type);
			} else if (!blkid_probe_lookup_value(pr, "PTTYPE",
								&type, NULL)) {
				SSDFS_MKFS_INFO(SSDFS_TRUE,
						"Device %s appears to contain"
						" an partition table (%s)\n",
						layout->env.dev_name, type);
			} else {
				SSDFS_MKFS_INFO(SSDFS_TRUE,
						"Device %s appears to contain"
						" something weird\n",
						layout->env.dev_name);
				goto end_check;
			}

			SSDFS_MKFS_INFO(SSDFS_TRUE,
					"All data will be lost after format!");
			SSDFS_MKFS_INFO(SSDFS_TRUE,
					"\nDO YOU REALLY WANT TO FORMAT %s?\n",
					layout->env.dev_name);

			do {
				SSDFS_MKFS_INFO(SSDFS_TRUE,
						"\nContinue? [y/N] ");
				c = getchar();

				if (c == EOF || c == '\n')
					goto abort_format;

				c_next = getchar();
				if (c_next != EOF && c_next != '\n')
					goto clear_input_buffer;

				if (c == 'n' || c == 'N')
					goto abort_format;

clear_input_buffer:
				while (c_next != '\n' && c_next != EOF)
					c_next = getchar();
			} while (c != 'y' && c != 'Y');
		}
	}

end_check:
	if (pr)
		blkid_free_probe(pr);
	if (ret < 0)
		SSDFS_MKFS_INFO(SSDFS_TRUE,
				"Probe of %s failed, can't detect any fs\n",
				layout->env.dev_name);
	return SSDFS_TRUE;

abort_format:
	if (pr)
		blkid_free_probe(pr);
	SSDFS_ERR("Abort format of device %s\n", layout->env.dev_name);
	return SSDFS_FALSE;
}

static void init_meta_array_item(struct ssdfs_volume_layout *layout,
				 int index)
{
	SSDFS_DBG(layout->env.show_debug, "index %d\n", index);

	BUG_ON(index >= SSDFS_METADATA_ITEMS_MAX);

	layout->meta_array[index].start_seg_index = -1;
	layout->meta_array[index].segs_count = -1;
	layout->meta_array[index].seg_state = SSDFS_ALLOC_POLICY_MAX;

	switch (index) {
	case SSDFS_INITIAL_SNAPSHOT:
		layout->meta_array[index].ptr = NULL;
		break;

	case SSDFS_SUPERBLOCK:
		layout->meta_array[index].ptr = (void *)&layout->sb;
		break;

	case SSDFS_SEGBMAP:
		layout->meta_array[index].ptr = (void *)&layout->segbmap;
		break;

	case SSDFS_PEB_MAPPING_TABLE:
		layout->meta_array[index].ptr = (void *)&layout->maptbl;
		break;

	case SSDFS_USER_DATA:
		layout->meta_array[index].ptr = NULL;
		break;

	default:
		BUG();
	};
}

static int alloc_segs_array(struct ssdfs_volume_layout *layout)
{
	int segs[SSDFS_ALLOC_POLICY_MAX] = {0};
	u32 fs_segs_count, fs_metadata_quota_max;
	u32 pebs_per_seg = (u32)(layout->seg_size / layout->env.erase_size);
	int i, j;
	int err = 0;

	SSDFS_DBG(layout->env.show_debug,
		  "segs %p, segs_capacity %d, segs_count %d\n",
		  layout->segs, layout->segs_capacity, layout->segs_count);

	BUG_ON(layout->segs);

	layout->segs_capacity = 0;
	layout->last_allocated_seg_index = -1;
	layout->segs_count = 0;

	for (i = 0; i < SSDFS_METADATA_ITEMS_MAX; i++) {
		int count = SSDFS_DEFAULT_ALLOC_SEGS_COUNT;
		int policy;

		if (i == SSDFS_USER_DATA)
			continue;

		init_meta_array_item(layout, i);

		if (mkfs_ops[i]->allocation_policy) {
			policy = mkfs_ops[i]->allocation_policy(layout, &count);
			if (policy < 0 || policy >= SSDFS_ALLOC_POLICY_MAX) {
				SSDFS_ERR("invalid allocation policy %d\n",
					  policy);
				return -EINVAL;
			} else if (count < 1) {
				SSDFS_ERR("invalid segments count %d\n",
					  count);
				return -EINVAL;
			}

			switch (policy) {
			case SSDFS_DEDICATED_SEGMENT:
				segs[policy] += count;
				break;

			case SSDFS_SHARED_SEGMENT:
				segs[policy] = max(segs[policy], count);
				break;

			default:
				BUG();
			};
		} else
			segs[SSDFS_DEDICATED_SEGMENT] += count;
	}

	for (i = 0; i < SSDFS_ALLOC_POLICY_MAX; i++)
		layout->segs_capacity += segs[i];

	fs_segs_count = (u32)(layout->env.fs_size / layout->seg_size);
	fs_metadata_quota_max = SSDFS_DEFAULT_METADATA_QUOTA_MAX(fs_segs_count);

	if (layout->segs_capacity > fs_metadata_quota_max) {
		SSDFS_ERR("partition too small: fs_segs_count %u, "
			  "fs_metadata_quota_max %u, metadata_segs_count %u\n",
			  fs_segs_count, fs_metadata_quota_max,
			  layout->segs_capacity);
		return -E2BIG;
	}

	layout->segs = calloc(layout->segs_capacity,
				sizeof(struct ssdfs_segment_desc));
	if (!layout->segs) {
		SSDFS_ERR("cannot allocate memory: count %d, item size %zu\n",
			  layout->segs_capacity,
			  sizeof(struct ssdfs_segment_desc));
		return -ENOMEM;
	}

	for (i = 0; i < layout->segs_capacity; i++) {
		layout->segs[i].seg_type = SSDFS_METADATA_ITEMS_MAX;
		layout->segs[i].seg_state = SSDFS_ALLOC_POLICY_MAX;
		layout->segs[i].seg_id = U64_MAX;
		layout->segs[i].pebs_capacity = pebs_per_seg;
		layout->segs[i].pebs_count = 0;

		layout->segs[i].pebs = calloc(pebs_per_seg,
					      sizeof(struct ssdfs_peb_content));
		if (!layout->segs[i].pebs) {
			err = -ENOMEM;
			SSDFS_ERR("fail to allocate pebs array: "
				  "pebs_per_seg %u, seg_index %d\n",
				  pebs_per_seg, i);
			goto free_memory;
		}

		for (j = 0; j < pebs_per_seg; j++) {
			layout->segs[i].pebs[j].leb_id = U64_MAX;
			layout->segs[i].pebs[j].peb_id = U64_MAX;
		}
	}

	layout->write_buffer.capacity = SSDFS_4KB;
	layout->write_buffer.offset = 0;
	err = posix_memalign((void **)&layout->write_buffer.ptr,
			     SSDFS_4KB,
			     layout->write_buffer.capacity);
	if (err) {
		layout->write_buffer.capacity = 0;
		SSDFS_ERR("fail to allocate memory\n");
		return err;
	} else if (!layout->write_buffer.ptr) {
		err = -ENOMEM;
		layout->write_buffer.capacity = 0;
		SSDFS_ERR("fail to allocate memory: "
			  "size %u\n",
			  layout->write_buffer.capacity);
		goto free_memory;
	}

	SSDFS_DBG(layout->env.show_debug, "ALLOCATED: segs %p, segs_capacity %d\n",
		  layout->segs, layout->segs_capacity);

	return 0;

free_memory:
	for (; i >= 0; i--)
		free(layout->segs[i].pebs);
	free(layout->segs);
	layout->segs = NULL;
	return err;
}

static void free_segs_array(struct ssdfs_volume_layout *layout)
{
	int i, j, k;

	SSDFS_DBG(layout->env.show_debug,
		  "segs %p, segs_capacity %d, segs_count %d\n",
		  layout->segs, layout->segs_capacity, layout->segs_count);

	if (layout->write_buffer.ptr) {
		free(layout->write_buffer.ptr);
		layout->write_buffer.ptr = NULL;
		layout->write_buffer.capacity = 0;
		layout->write_buffer.offset = 0;
	}

	segbmap_destroy_fragments_array(layout);
	maptbl_destroy_fragments_array(layout);
	maptbl_cache_destroy_fragments_array(layout);

	if (layout->segs_capacity != layout->segs_count)
		SSDFS_WARN("segments capacity is not equal to segments count\n");

	if (!layout->segs)
		return;

	for (i = 0; i < layout->segs_count; i++) {
		for (j = 0; j < layout->segs[i].pebs_capacity; j++) {
			for (k = 0; k < SSDFS_SEG_LOG_ITEMS_COUNT; k++) {
				struct ssdfs_extent_desc *desc;

				desc = &layout->segs[i].pebs[j].extents[k];
				free(desc->buf);
			}
		}

		free(layout->segs[i].pebs);
		layout->segs[i].pebs = NULL;
	}

	free(layout->segs);
	layout->segs = NULL;
}

static int mkfs_create(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_segment_desc *seg;
	struct ssdfs_peb_content *peb;
	struct ssdfs_extent_desc *extent;
	int i, j, k;
	int err;

	for (i = 0; i < SSDFS_METADATA_ITEMS_MAX; i++) {
		if (!mkfs_ops[i]->prepare)
			continue;
		err = mkfs_ops[i]->prepare(layout);
		if (err)
			return err;
	}

	for (i = 0; i < layout->segs_capacity; i++) {
		seg = &layout->segs[i];
		SSDFS_DBG(layout->env.show_debug,
			  "seg_type %#x, seg_state %#x, seg_id %llu, "
			  "pebs_count %u, pebs_capacity %u\n",
			  seg->seg_type, seg->seg_state,
			  seg->seg_id, seg->pebs_count,
			  seg->pebs_capacity);

		for (j = 0; j < seg->pebs_capacity; j++) {
			peb = &seg->pebs[j];
			SSDFS_DBG(layout->env.show_debug,
				  "leb_id %llu, peb_id %llu\n",
				  peb->leb_id, peb->peb_id);

			for (k = 0; k < SSDFS_SEG_LOG_ITEMS_COUNT; k++) {
				extent = &peb->extents[k];
				SSDFS_DBG(layout->env.show_debug,
					  "index %d, offset %u, "
					  "bytes_count %u, buf %p\n",
					  k, extent->offset,
					  extent->bytes_count,
					  extent->buf);
			}
		}
	}

	for (i = 0; i < SSDFS_METADATA_ITEMS_MAX; i++) {
		if (!mkfs_ops[i]->validate)
			continue;
		err = mkfs_ops[i]->validate(layout);
		if (err)
			return err;
	}

	for (i = 0; i < layout->segs_capacity; i++) {
		seg = &layout->segs[i];
		SSDFS_DBG(layout->env.show_debug,
			  "seg_type %#x, seg_state %#x, seg_id %llu, "
			  "pebs_count %u, pebs_capacity %u\n",
			  seg->seg_type, seg->seg_state,
			  seg->seg_id, seg->pebs_count,
			  seg->pebs_capacity);

		for (j = 0; j < seg->pebs_capacity; j++) {
			peb = &seg->pebs[j];
			SSDFS_DBG(layout->env.show_debug,
				  "leb_id %llu, peb_id %llu\n",
				  peb->leb_id, peb->peb_id);

			for (k = 0; k < SSDFS_SEG_LOG_ITEMS_COUNT; k++) {
				extent = &peb->extents[k];
				SSDFS_DBG(layout->env.show_debug,
					  "index %d, offset %u, "
					  "bytes_count %u, buf %p\n",
					  k, extent->offset,
					  extent->bytes_count,
					  extent->buf);
			}
		}
	}

	for (i = 0; i < SSDFS_METADATA_ITEMS_MAX; i++) {
		if (!mkfs_ops[i]->define_layout)
			continue;
		err = mkfs_ops[i]->define_layout(layout);
		if (err)
			return err;
	}

	for (i = 0; i < layout->segs_capacity; i++) {
		seg = &layout->segs[i];
		SSDFS_DBG(layout->env.show_debug,
			  "seg_type %#x, seg_state %#x, seg_id %llu, "
			  "pebs_count %u, pebs_capacity %u\n",
			  seg->seg_type, seg->seg_state,
			  seg->seg_id, seg->pebs_count,
			  seg->pebs_capacity);

		for (j = 0; j < seg->pebs_capacity; j++) {
			peb = &seg->pebs[j];
			SSDFS_DBG(layout->env.show_debug,
				  "leb_id %llu, peb_id %llu\n",
				  peb->leb_id, peb->peb_id);

			for (k = 0; k < SSDFS_SEG_LOG_ITEMS_COUNT; k++) {
				extent = &peb->extents[k];
				SSDFS_DBG(layout->env.show_debug,
					  "index %d, offset %u, "
					  "bytes_count %u, buf %p\n",
					  k, extent->offset,
					  extent->bytes_count,
					  extent->buf);
			}
		}
	}

	for (i = 0; i < SSDFS_METADATA_ITEMS_MAX; i++) {
		if (!mkfs_ops[i]->commit)
			continue;
		err = mkfs_ops[i]->commit(layout);
		if (err)
			return err;
	}

	for (i = 0; i < layout->segs_capacity; i++) {
		seg = &layout->segs[i];
		SSDFS_DBG(layout->env.show_debug,
			  "seg_type %#x, seg_state %#x, seg_id %llu, "
			  "pebs_count %u, pebs_capacity %u\n",
			  seg->seg_type, seg->seg_state,
			  seg->seg_id, seg->pebs_count,
			  seg->pebs_capacity);

		for (j = 0; j < seg->pebs_capacity; j++) {
			peb = &seg->pebs[j];
			SSDFS_DBG(layout->env.show_debug,
				  "leb_id %llu, peb_id %llu\n",
				  peb->leb_id, peb->peb_id);

			for (k = 0; k < SSDFS_SEG_LOG_ITEMS_COUNT; k++) {
				extent = &peb->extents[k];
				SSDFS_DBG(layout->env.show_debug,
					  "index %d, offset %u, "
					  "bytes_count %u, buf %p\n",
					  k, extent->offset,
					  extent->bytes_count,
					  extent->buf);
			}
		}
	}

	return 0;
}

static int check_extent_before_write(struct ssdfs_volume_layout *layout,
				     u64 peb_id,
				     struct ssdfs_extent_desc *desc)
{
	u64 fs_size = layout->env.fs_size;
	u32 erasesize = layout->env.erase_size;
	u64 peb_start_offset;
	u32 extent_offset;
	u32 extent_size;

	SSDFS_DBG(layout->env.show_debug,
		  "buf %p, peb_id %llu, extent_offset %u, extent_bytes %u\n",
		  desc->buf, peb_id, desc->offset, desc->bytes_count);

	if (!desc->buf)
		return 0;

	BUG_ON((U64_MAX / erasesize) <= peb_id);

	peb_start_offset = peb_id * erasesize;

	if (peb_start_offset >= fs_size) {
		SSDFS_ERR("peb_start_offset %llu >= fs_size %llu\n",
			  peb_start_offset, fs_size);
		return -E2BIG;
	}

	extent_offset = desc->offset;

	BUG_ON(peb_start_offset >= (U64_MAX - extent_offset));

	if ((peb_start_offset + extent_offset) >= fs_size) {
		SSDFS_ERR("peb_start_offset %llu, extent_offset %u, "
			  "fs_size %llu\n",
			  peb_start_offset, extent_offset, fs_size);
		return -E2BIG;
	}

	extent_size = desc->bytes_count;

	BUG_ON(extent_size == 0);
	BUG_ON((peb_start_offset + extent_offset) >= (U64_MAX - extent_size));

	if ((peb_start_offset + extent_offset + extent_size) > fs_size) {
		SSDFS_ERR("peb_start_offset %llu, extent_offset %u, "
			  "extent_size %u, fs_size %llu\n",
			  peb_start_offset, extent_offset,
			  extent_size, fs_size);
		return -E2BIG;
	}

	if (((u64)extent_offset + extent_size) > erasesize) {
		SSDFS_ERR("extent (offset %u, size %u) is outside "
			  "of erasesize %u\n",
			  extent_offset, extent_size, erasesize);
		return -ERANGE;
	}

	return 0;
}

static int check_peb_before_write(struct ssdfs_volume_layout *layout,
				  struct ssdfs_peb_content *peb,
				  char *bmap, u32 *blks)
{
	struct ssdfs_extent_desc *desc;
	u32 start_offset = U32_MAX;
	u32 payload_size = U32_MAX;
	u32 aligned_offset;
	u32 aligned_size;
	u32 erasesize = layout->env.erase_size;
	u32 pagesize = layout->page_size;
	u64 start_blk;
	int i;
	int err;

	SSDFS_DBG(layout->env.show_debug,
		  "layout %p, leb_id %llu, peb_id %llu, bmap %p\n",
		  layout, peb->leb_id, peb->peb_id, bmap);

	for (i = 0; i < SSDFS_SEG_LOG_ITEMS_COUNT; i++) {
		desc = &peb->extents[i];

		if (!desc->buf)
			continue;

		err = check_extent_before_write(layout, peb->peb_id, desc);
		if (err) {
			SSDFS_ERR("invalid extent: "
				  "index %d, peb_id %llu, err %d\n",
				  i, peb->peb_id, err);
			return err;
		}

		if (start_offset == U32_MAX && payload_size == U32_MAX) {
			start_offset = desc->offset;
			payload_size = desc->bytes_count;
		} else {
			u32 cur_offset = start_offset + payload_size;

			if (cur_offset > desc->offset) {
				SSDFS_ERR("invalid extent: "
					  "cur_offset %u, offset %u, size %u\n",
					  cur_offset, desc->offset,
					  desc->bytes_count);
				return -ERANGE;
			}

			if ((desc->offset - cur_offset) >= pagesize) {
				SSDFS_ERR("invalid extent: "
					  "cur_offset %u, offset %u, size %u\n",
					  cur_offset, desc->offset,
					  desc->bytes_count);
				return -ERANGE;
			}

			if (cur_offset != desc->offset)
				payload_size += desc->offset - cur_offset;

			payload_size += desc->bytes_count;
		}
	}

	BUG_ON(start_offset == U32_MAX);
	BUG_ON(payload_size == U32_MAX);
	BUG_ON(payload_size == 0);

	aligned_offset = (start_offset / pagesize) * pagesize;
	BUG_ON(aligned_offset > start_offset);
	aligned_size = payload_size + (start_offset - aligned_offset);

	start_blk = (peb->peb_id * erasesize) + aligned_offset;
	start_blk /= pagesize;

	*blks = (aligned_size + pagesize - 1) / pagesize;

	for (i = 0; i < *blks; i++) {
		u64 cur_blk;
		size_t bits;
		unsigned long *addr;
		int nr;

		cur_blk = start_blk + i;
		bits = 8 * sizeof(unsigned long);
		addr = (unsigned long *)bmap + (cur_blk / bits);
		nr = cur_blk % bits;

		if (test_bit(nr, addr)) {
			SSDFS_ERR("block %llu has used yet\n", cur_blk);
			return -EINVAL;
		} else
			__set_bit(nr, addr);
	}

	return 0;
}

static int check_layout_before_write(struct ssdfs_volume_layout *layout)
{
	char *bmap;
	u64 fs_size = layout->env.fs_size;
	u64 segsize = layout->seg_size;
	u32 pagesize = layout->page_size;
	u64 fs_blks = fs_size / pagesize;
	u64 seg_blks_capacity = segsize / pagesize;
	int i, j;
	int err = 0;

	bmap = calloc((fs_blks + 8 - 1)/8, 1);
	if (!bmap) {
		SSDFS_ERR("unable to allocate %llu bytes\n",
			  (unsigned long long)((fs_blks + 8 - 1)/8));
		return -ENOMEM;
	}

	for (i = 0; i < layout->segs_count; i++) {
		u64 seg_blks = 0;

		for (j = 0; j < layout->segs[i].pebs_count; j++) {
			struct ssdfs_peb_content *peb;
			u32 blks = 0;

			peb = &layout->segs[i].pebs[j];
			err = check_peb_before_write(layout, peb, bmap, &blks);
			if (err) {
				SSDFS_ERR("invalid PEB: "
					  "seg_index %d, peb_index %d, "
					  "err %d\n",
					  i, j, err);
				return err;
			}

			seg_blks += blks;
		}

		if (seg_blks > seg_blks_capacity) {
			SSDFS_ERR("blocks count %llu is greater than %llu\n",
				  seg_blks, seg_blks_capacity);
			err = -E2BIG;
			goto free_bmap;
		}
	}

free_bmap:
	free(bmap);
	return err;
}

static int erase_peb(struct ssdfs_volume_layout *layout,
		     int seg_index, int peb_index,
		     char *buf, size_t buf_size)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	int fd = layout->env.fd;
	u32 peb_size = layout->env.erase_size;
	u64 offset = 0;
	int err;

	SSDFS_DBG(layout->env.show_debug,
		  "seg_index %d, peb_index %d, buf %p, buf_size %zu\n",
		  seg_index, peb_index, buf, buf_size);

	if (seg_index >= layout->segs_capacity) {
		SSDFS_ERR("invalid seg_index %d, "
			  "segs_capacity %u\n",
			  seg_index,
			  layout->segs_capacity);
		return -EINVAL;
	}

	seg_desc = &layout->segs[seg_index];

	if (peb_index >= seg_desc->pebs_count) {
		SSDFS_ERR("peb_index %d >= seg_desc->pebs_count %u\n",
			  peb_index, seg_desc->pebs_count);
		return -EINVAL;
	}

	peb_desc = &seg_desc->pebs[peb_index];
	offset = peb_desc->peb_id * peb_size;

	err = layout->env.dev_ops->erase(fd, offset, peb_size,
					 buf, buf_size,
					 layout->env.show_debug);
	if (err) {
		SSDFS_ERR("unable to erase peb #%llu\n",
			  peb_desc->peb_id);
		return err;
	}

	return 0;
}

static int erase_device(struct ssdfs_volume_layout *layout)
{
	int fd = layout->env.fd;
	u64 seg_size = layout->seg_size;
	u64 offset = 0;
	char *buf;
	size_t buf_size = SSDFS_128KB;
	u32 i, j;
	int err = 0;

	SSDFS_DBG(layout->env.show_debug,
		  "device %s, segs_count %u, seg_size %llu, "
		  "need_erase_device %d, is_volume_erased %d\n",
		  layout->env.dev_name, layout->segs_count,
		  layout->seg_size, layout->need_erase_device,
		  layout->is_volume_erased);

	if (layout->is_volume_erased)
		return 0;

	err = posix_memalign((void **)&buf, SSDFS_128KB, buf_size);
	if (err) {
		SSDFS_ERR("fail to allocate memory\n");
		return err;
	} else if (!buf) {
		SSDFS_ERR("fail to allocate memory: "
			  "size %zu\n",
			  buf_size);
		return -ENOMEM;
	}

	memset(buf, 0xff, buf_size);

	if (layout->need_erase_device) {
		u32 fs_segs_count = (u32)(layout->env.fs_size / seg_size);

		for (i = 0; i < fs_segs_count; i++) {
			SSDFS_DBG(layout->env.show_debug,
				  "erasing segment %u...\n",
				  i);

			err = layout->env.dev_ops->erase(fd, offset, seg_size,
							buf, buf_size,
							layout->env.show_debug);
			if (err) {
				SSDFS_ERR("unable to erase segment #%u\n", i);
				goto free_erase_buf;
			}

			offset += seg_size;
		}
	} else {
		for (i = 0; i < layout->segs_count; i++) {
			for (j = 0; j < layout->segs[i].pebs_count; j++) {
				err = erase_peb(layout, i, j, buf, buf_size);
				if (err) {
					SSDFS_ERR("fail to erase peb: "
						  "seg_index %u, peb_index %u, "
						  "err %d\n",
						  i, j, err);
					goto free_erase_buf;
				}
			}
		}
	}

free_erase_buf:
	free(buf);
	return err;
}

static int flush_write_buffer(struct ssdfs_volume_layout *layout,
			      u64 offset, u32 size)
{
	struct ssdfs_nand_geometry info = {
		.erasesize = layout->env.erase_size,
		.writesize = layout->page_size,
	};
	int fd = layout->env.fd;
	int err;

	SSDFS_DBG(layout->env.show_debug,
		  "offset %llu, size %u\n",
		  offset, size);

	if (layout->write_buffer.ptr == NULL) {
		SSDFS_ERR("write buffer is not allocated\n");
		return -ERANGE;
	}

	if (layout->write_buffer.capacity == 0) {
		SSDFS_ERR("invalid write buffer capacity %u\n",
			  layout->write_buffer.capacity);
		return -ERANGE;
	}

	if (size == 0 || size > layout->write_buffer.capacity) {
		SSDFS_ERR("invalid requested size: "
			  "size %u, layout->write_buffer.capacity %u\n",
			  size, layout->write_buffer.capacity);
		return -ERANGE;
	}

	if (offset % SSDFS_4KB) {
		SSDFS_ERR("unaligned offset %llu\n",
			  offset);
		return -ERANGE;
	}

	err = layout->env.dev_ops->write(fd, &info, offset, size,
					 layout->write_buffer.ptr,
					 layout->env.show_debug);
	if (err) {
		SSDFS_ERR("unable to write: "
			  "offset %llu, bytes_count %u\n",
			  offset, size);
		return err;
	}

	memset(layout->write_buffer.ptr, 0xFF, layout->write_buffer.capacity);
	layout->write_buffer.offset = 0;

	return 0;
}

static int prepare_write_buffer(struct ssdfs_volume_layout *layout,
				u32 offset, char *buf, u32 size,
				u32 *copied_size)
{
	u32 bytes_count;

	SSDFS_DBG(layout->env.show_debug,
		  "offset %u, size %u\n",
		  offset, size);

	*copied_size = 0;

	if (layout->write_buffer.ptr == NULL) {
		SSDFS_ERR("write buffer is not allocated\n");
		return -ERANGE;
	}

	if (layout->write_buffer.capacity == 0) {
		SSDFS_ERR("invalid write buffer capacity %u\n",
			  layout->write_buffer.capacity);
		return -ERANGE;
	}

	if (buf == NULL) {
		SSDFS_ERR("input buffer is not allocated\n");
		return -ERANGE;
	}

	if (offset < layout->write_buffer.offset ||
	    offset >= layout->write_buffer.capacity) {
		SSDFS_DBG(layout->env.show_debug,
			  "no more space: write_buffer.offset %u, "
			  "offset %u, size %u\n",
			  layout->write_buffer.offset, offset, size);
		return -ENOSPC;
	}

	bytes_count = min_t(u32, size, layout->write_buffer.capacity - offset);
	memcpy(layout->write_buffer.ptr + offset, buf, bytes_count);
	*copied_size = bytes_count;
	layout->write_buffer.offset = offset + bytes_count;

	if (*copied_size != size) {
		SSDFS_DBG(layout->env.show_debug,
			  "no more space: offset %u, size %u\n",
			  offset, size);
		return -ENOSPC;
	}

	if ((offset + bytes_count) == layout->write_buffer.capacity) {
		SSDFS_DBG(layout->env.show_debug,
			  "no more space: offset %u, size %u\n",
			  offset, size);
		return -ENOSPC;
	}

	return 0;
}

static int write_peb(struct ssdfs_volume_layout *layout,
		     int seg_index, int peb_index)
{
	struct ssdfs_segment_desc *seg_desc;
	struct ssdfs_peb_content *peb_desc;
	u32 erase_size = layout->env.erase_size;
	u64 peb_id;
	u32 peb_offset = 0;
	u64 volume_offset;
	u32 flushed_bytes = 0;
	u32 i;
	int err = 0;

	SSDFS_DBG(layout->env.show_debug,
		  "device %s, segs_count %u, segs_capacity %u, "
		  "seg_index %d, peb_index %d\n",
		  layout->env.dev_name, layout->segs_count,
		  layout->segs_capacity,
		  seg_index, peb_index);

	if (seg_index >= layout->segs_capacity) {
		SSDFS_ERR("invalid seg_index %d, "
			  "segs_capacity %u\n",
			  seg_index,
			  layout->segs_capacity);
		return -EINVAL;
	}

	seg_desc = &layout->segs[seg_index];

	if (peb_index >= seg_desc->pebs_count) {
		SSDFS_ERR("peb_index %d >= seg_desc->pebs_count %u\n",
			  peb_index, seg_desc->pebs_count);
		return -EINVAL;
	}

	memset(layout->write_buffer.ptr, 0xFF, layout->write_buffer.capacity);

	peb_desc = &seg_desc->pebs[peb_index];
	peb_id = peb_desc->peb_id;
	volume_offset = peb_id * erase_size;

	for (i = 0; i < SSDFS_SEG_LOG_ITEMS_COUNT; i++) {
		struct ssdfs_extent_desc *desc;
		u32 write_buf_offset;
		u32 size;
		char *buf;

		desc = &peb_desc->extents[i];
		if (!desc->buf)
			continue;

		buf = desc->buf;

		if (desc->offset < peb_offset) {
			SSDFS_ERR("desc->offset %u < peb_offset %u\n",
				  desc->offset, peb_offset);
			return -ERANGE;
		}

		peb_offset = desc->offset;
		size = desc->bytes_count;

		SSDFS_DBG(layout->env.show_debug,
			  "item_index %d, peb_offset %u, "
			  "size %u, flushed_bytes %u\n",
			  i, peb_offset, size, flushed_bytes);

		while (size > 0) {
			u32 copied_bytes = 0;

			write_buf_offset =
				peb_offset % layout->write_buffer.capacity;

			err = prepare_write_buffer(layout, write_buf_offset,
						   buf, size, &copied_bytes);
			if (err == -ENOSPC) {
				err = flush_write_buffer(layout, volume_offset,
						layout->write_buffer.capacity);
				if (err) {
					SSDFS_ERR("fail to flush write buffer: "
						  "volume_offset %llu, err %d\n",
						  volume_offset, err);
					return err;
				}

				volume_offset += layout->write_buffer.capacity;
				flushed_bytes += layout->write_buffer.capacity;
			} else if (err) {
				SSDFS_ERR("fail to prepare write buffer: "
					  "peb_offset %u, write_buf_offset %u, "
					  "size %u, err %d\n",
					  peb_offset, write_buf_offset,
					  size, err);
				return err;
			}

			if (copied_bytes > size) {
				SSDFS_ERR("copied_bytes %u > size %u\n",
					  copied_bytes, size);
				return -ERANGE;
			}

			buf += copied_bytes;
			size -= copied_bytes;
			peb_offset += copied_bytes;

			SSDFS_DBG(layout->env.show_debug,
				  "copied_bytes %u, size %u, "
				  "peb_offset %u\n",
				  copied_bytes, size,
				  peb_offset);
		};
	}

	SSDFS_DBG(layout->env.show_debug,
		  "peb_offset %u, flushed_bytes %u\n",
		  peb_offset, flushed_bytes);

	if (peb_offset > flushed_bytes) {
		u32 aligned_size;

		aligned_size = peb_offset - flushed_bytes;
		aligned_size += SSDFS_4KB - 1;
		aligned_size = (aligned_size / SSDFS_4KB) * SSDFS_4KB;

		err = flush_write_buffer(layout, volume_offset,
					 aligned_size);
		if (err) {
			SSDFS_ERR("fail to flush write buffer: "
				  "volume_offset %llu, err %d\n",
				  volume_offset, err);
			return err;
		}
	}

	return 0;
}

static int write_segments(struct ssdfs_volume_layout *layout)
{
	u32 i, j;
	int err;

	SSDFS_DBG(layout->env.show_debug,
		  "device %s, segs_count %u, segs_capacity %u\n",
		  layout->env.dev_name, layout->segs_count,
		  layout->segs_capacity);

	for (i = 0; i < layout->segs_count; i++) {
		for (j = 0; j < layout->segs[i].pebs_count; j++) {
			err = write_peb(layout, i, j);
			if (err) {
				SSDFS_ERR("fail to write PEB: "
					  "seg_index %d, peb_index %d, "
					  "err %d\n",
					  i, j, err);
				return err;
			}
		}
	}

	return 0;
}

static int write_device(struct ssdfs_volume_layout *layout)
{
	int err;

	SSDFS_DBG(layout->env.show_debug,
		  "fd %d, device %s, segs %p, "
		  "segs_capacity %u, segs_count %u\n",
		  layout->env.fd, layout->env.dev_name, layout->segs,
		  layout->segs_capacity, layout->segs_count);

	BUG_ON(!layout->segs);
	BUG_ON(layout->segs_capacity == 0 || layout->segs_count == 0);
	BUG_ON(!layout->env.dev_ops);
	BUG_ON(!layout->env.dev_ops->write || !layout->env.dev_ops->erase);

	if (layout->segs_capacity != layout->segs_count) {
		SSDFS_ERR("segs_capacity %llu is unequal to segs_count %llu\n",
			  (unsigned long long)layout->segs_capacity,
			  (unsigned long long)layout->segs_count);
		return -EINVAL;
	}

	err = check_layout_before_write(layout);
	if (err)
		return err;

	err = erase_device(layout);
	if (err)
		return err;

	err = write_segments(layout);
	if (err)
		return err;

	if (fsync(layout->env.fd) < 0) {
		SSDFS_ERR("fail to sync device %s: %s\n",
			  layout->env.dev_name, strerror(errno));
		return errno;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	struct ssdfs_volume_layout *layout_ptr;
	int err = 0;

	layout_ptr = &volume_layout;

	parse_options(argc, argv, layout_ptr);

	prepare_metadata_creation_ops();

	layout_ptr->env.dev_name = argv[optind];

	SSDFS_MKFS_INFO(layout_ptr->env.show_info,
			"[001]\tOPEN DEVICE...\n");

	err = open_device(&layout_ptr->env, O_DIRECT);
	if (err)
		exit(EXIT_FAILURE);

	SSDFS_MKFS_INFO(layout_ptr->env.show_info,
			"[001]\t[SUCCESS]\n");
	SSDFS_MKFS_INFO(layout_ptr->env.show_info,
			"[002]\tCHECK ENVIRONMENT...\n");

	err = validate_key_creation_options(layout_ptr);
	if (err)
		goto mkfs_failed;

	if (is_device_mounted(layout_ptr))
		goto mkfs_failed;

	if (!is_safe_overwrite_device(layout_ptr))
		goto mkfs_failed;

	SSDFS_MKFS_INFO(layout_ptr->env.show_info,
			"[002]\t[SUCCESS]\n");
	SSDFS_MKFS_INFO(layout_ptr->env.show_info,
			"[003]\tPREPARE SEGMENTS ARRAY...\n");

	err = alloc_segs_array(layout_ptr);
	if (err)
		goto mkfs_failed;

	SSDFS_MKFS_INFO(layout_ptr->env.show_info,
			"[003]\t[SUCCESS]\n");
	SSDFS_MKFS_INFO(layout_ptr->env.show_info,
			"[004]\tCREATE VOLUME STRUCTURES...\n");

	err = mkfs_create(layout_ptr);
	if (err)
		goto free_segs_memory;

	SSDFS_MKFS_INFO(layout_ptr->env.show_info,
			"[004]\t[SUCCESS]\n");
	SSDFS_MKFS_INFO(layout_ptr->env.show_info,
			"[005]\tWRITE METADATA...\n");

	err = write_device(layout_ptr);
	if (err)
		goto free_segs_memory;
	else {
		SSDFS_MKFS_INFO(layout_ptr->env.show_info,
				"[005]\t[SUCCESS]\n");
	}

free_segs_memory:
	free_segs_array(layout_ptr);

mkfs_failed:
	close(layout_ptr->env.fd);
	exit(err ? EXIT_FAILURE : EXIT_SUCCESS);
}
