//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/mkfs.ssdfs/superblock_segment.c - superblock segment creation.
 *
 * Copyright (c) 2014-2019 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 *
 * HGST Confidential
 * (C) Copyright 2014-2019, HGST, Inc., All rights reserved.
 *
 * Created by HGST, San Jose Research Center, Storage Architecture Group
 * Authors: Vyacheslav Dubeyko <slava@dubeyko.com>
 *
 * Acknowledgement: Cyril Guyot <Cyril.Guyot@wdc.com>
 *                  Zvonimir Bandic <Zvonimir.Bandic@wdc.com>
 */

#include <unistd.h>
#include <sys/types.h>
#include <uuid/uuid.h>

#include "mkfs.h"

/************************************************************************
 *                    Superblock creation functionality                 *
 ************************************************************************/

int sb_mkfs_allocation_policy(struct ssdfs_volume_layout *layout,
				int *segs)
{
	int seg_state = SSDFS_DEDICATED_SEGMENT;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	*segs = (SSDFS_RESERVED_SB_SEG + 1) * SSDFS_SB_SEG_COPY_MAX;
	layout->meta_array[SSDFS_SUPERBLOCK].segs_count = *segs;
	layout->meta_array[SSDFS_SUPERBLOCK].seg_state = seg_state;

	SSDFS_DBG(layout->env.show_debug,
		  "superblock segs %d\n", *segs);
	return seg_state;
}

static inline
int prepare_block_bitmap_options(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_volume_state *vs = &layout->sb.vs;
	u16 flags = 0;
	u8 compression = SSDFS_BLK_BMAP_NOCOMPR_TYPE;

	if (layout->blkbmap.has_backup_copy)
		flags |= SSDFS_BLK_BMAP_CREATE_COPY;

	switch (layout->blkbmap.compression) {
	case SSDFS_UNCOMPRESSED_BLOB:
		/* do nothing */
		break;

	case SSDFS_ZLIB_BLOB:
		flags |= SSDFS_BLK_BMAP_MAKE_COMPRESSION;
		compression = SSDFS_BLK_BMAP_ZLIB_COMPR_TYPE;
		break;

	case SSDFS_LZO_BLOB:
		flags |= SSDFS_BLK_BMAP_MAKE_COMPRESSION;
		compression = SSDFS_BLK_BMAP_LZO_COMPR_TYPE;
		break;

	default:
		SSDFS_ERR("invalid compression type %#x\n",
			  layout->blkbmap.compression);
		return -ERANGE;
	}

	vs->blkbmap.flags = cpu_to_le16(flags);
	vs->blkbmap.compression = cpu_to_le8(compression);

	return 0;
}

static inline
int prepare_blk2off_table_options(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_volume_state *vs = &layout->sb.vs;
	u16 flags = 0;
	u8 compression = SSDFS_BLK2OFF_TBL_NOCOMPR_TYPE;

	if (layout->blk2off_tbl.has_backup_copy)
		flags |= SSDFS_BLK2OFF_TBL_CREATE_COPY;

	switch (layout->blk2off_tbl.compression) {
	case SSDFS_UNCOMPRESSED_BLOB:
		/* do nothing */
		break;

	case SSDFS_ZLIB_BLOB:
		flags |= SSDFS_BLK2OFF_TBL_MAKE_COMPRESSION;
		compression = SSDFS_BLK2OFF_TBL_ZLIB_COMPR_TYPE;
		break;

	case SSDFS_LZO_BLOB:
		flags |= SSDFS_BLK2OFF_TBL_MAKE_COMPRESSION;
		compression = SSDFS_BLK2OFF_TBL_LZO_COMPR_TYPE;
		break;

	default:
		SSDFS_ERR("invalid compression type %#x\n",
			  layout->blk2off_tbl.compression);
		return -ERANGE;
	}

	vs->blk2off_tbl.flags = cpu_to_le16(flags);
	vs->blk2off_tbl.compression = cpu_to_le8(compression);

	return 0;
}

static void sb_set_lnodes_log_pages(struct ssdfs_volume_layout *layout)
{
	u32 erasesize;
	u32 pagesize;
	u32 pages_per_peb;

	SSDFS_DBG(layout->env.show_debug,
		  "log_pages %u\n",
		  layout->btree.lnode_log_pages);

	erasesize = layout->env.erase_size;
	pagesize = layout->page_size;
	pages_per_peb = erasesize / pagesize;

	if (layout->btree.lnode_log_pages == U16_MAX)
		layout->btree.lnode_log_pages = pages_per_peb;
	else {
		if (layout->btree.lnode_log_pages > pages_per_peb) {
			SSDFS_WARN("log_pages is corrected from %u to %u\n",
				   layout->btree.lnode_log_pages,
				   pages_per_peb);
			layout->btree.lnode_log_pages = pages_per_peb;
		}

		if (pages_per_peb % layout->btree.lnode_log_pages) {
			SSDFS_WARN("pages_per_peb %u, log_pages %u\n",
				   pages_per_peb,
				   layout->btree.lnode_log_pages);
		}
	}

	layout->sb.vh.lnodes_seg_log_pages =
		cpu_to_le16(layout->btree.lnode_log_pages);

	SSDFS_DBG(layout->env.show_debug,
		  "leaf node's log pages %u\n",
		  layout->btree.lnode_log_pages);
}

static void sb_set_hnodes_log_pages(struct ssdfs_volume_layout *layout)
{
	u32 erasesize;
	u32 pagesize;
	u32 pages_per_peb;

	SSDFS_DBG(layout->env.show_debug,
		  "log_pages %u\n",
		  layout->btree.hnode_log_pages);

	erasesize = layout->env.erase_size;
	pagesize = layout->page_size;
	pages_per_peb = erasesize / pagesize;

	if (layout->btree.hnode_log_pages == U16_MAX)
		layout->btree.hnode_log_pages = pages_per_peb;
	else {
		if (layout->btree.hnode_log_pages > pages_per_peb) {
			SSDFS_WARN("log_pages is corrected from %u to %u\n",
				   layout->btree.hnode_log_pages,
				   pages_per_peb);
			layout->btree.hnode_log_pages = pages_per_peb;
		}

		if (pages_per_peb % layout->btree.hnode_log_pages) {
			SSDFS_WARN("pages_per_peb %u, log_pages %u\n",
				   pages_per_peb,
				   layout->btree.hnode_log_pages);
		}
	}

	layout->sb.vh.hnodes_seg_log_pages =
		cpu_to_le16(layout->btree.hnode_log_pages);

	SSDFS_DBG(layout->env.show_debug,
		  "hybrid node's log pages %u\n",
		  layout->btree.hnode_log_pages);
}

static void sb_set_inodes_log_pages(struct ssdfs_volume_layout *layout)
{
	u32 erasesize;
	u32 pagesize;
	u32 pages_per_peb;

	SSDFS_DBG(layout->env.show_debug,
		  "log_pages %u\n",
		  layout->btree.inode_log_pages);

	erasesize = layout->env.erase_size;
	pagesize = layout->page_size;
	pages_per_peb = erasesize / pagesize;

	if (layout->btree.inode_log_pages == U16_MAX)
		layout->btree.inode_log_pages = pages_per_peb;
	else {
		if (layout->btree.inode_log_pages > pages_per_peb) {
			SSDFS_WARN("log_pages is corrected from %u to %u\n",
				   layout->btree.inode_log_pages,
				   pages_per_peb);
			layout->btree.inode_log_pages = pages_per_peb;
		}

		if (pages_per_peb % layout->btree.inode_log_pages) {
			SSDFS_WARN("pages_per_peb %u, log_pages %u\n",
				   pages_per_peb,
				   layout->btree.inode_log_pages);
		}
	}

	layout->sb.vh.inodes_seg_log_pages =
		cpu_to_le16(layout->btree.inode_log_pages);

	SSDFS_DBG(layout->env.show_debug,
		  "index node's log pages %u\n",
		  layout->btree.inode_log_pages);
}

static int sb_dentries_btree_desc_prepare(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_btree_descriptor *desc;
	u32 erasesize;
	u32 pagesize;
	u32 node_size;
	size_t hdr_size = sizeof(struct ssdfs_dentries_btree_node_header);
	size_t node_ptr_size = sizeof(struct ssdfs_btree_index_key);
	u16 min_index_area_size;

	memset(&layout->sb.vh.dentries_btree, 0,
		sizeof(struct ssdfs_dentries_btree_descriptor));

	desc = &layout->sb.vh.dentries_btree.desc;

	desc->magic = cpu_to_le32(SSDFS_DENTRIES_BTREE_MAGIC);
	desc->flags = cpu_to_le16(SSDFS_BTREE_DESC_INDEX_AREA_RESIZABLE);
	desc->type = cpu_to_le8((u8)SSDFS_DENTRIES_BTREE);

	erasesize = layout->env.erase_size;
	pagesize = layout->page_size;
	node_size = layout->btree.node_size;

	if (node_size <= 0 || node_size >= U16_MAX) {
		SSDFS_ERR("invalid option: node_size %u\n",
			  node_size);
		return -ERANGE;
	}

	if (node_size < pagesize || node_size % pagesize) {
		SSDFS_ERR("invalid option: node_size %u, pagesize %u \n",
			  node_size, pagesize);
		return -ERANGE;
	}

	if (node_size >= erasesize || erasesize % node_size) {
		SSDFS_ERR("invalid option: node_size %u, erasesize %u \n",
			  node_size, erasesize);
		return -ERANGE;
	}

	desc->log_node_size = cpu_to_le8((u8)ilog2(node_size));
	desc->pages_per_node = cpu_to_le8((u8)(node_size / pagesize));
	desc->node_ptr_size = cpu_to_le8((u8)node_ptr_size);
	desc->index_size = cpu_to_le16((u16)sizeof(struct ssdfs_btree_index));
	desc->item_size = cpu_to_le16((u16)sizeof(struct ssdfs_dir_entry));

	min_index_area_size = layout->btree.min_index_area_size;

	if (min_index_area_size == 0)
		min_index_area_size = (u16)hdr_size;

	if (min_index_area_size <= node_ptr_size ||
	    min_index_area_size % node_ptr_size) {
		SSDFS_ERR("invalid option: min_index_area_size %u, "
			  "node_ptr_size %zu\n",
			  min_index_area_size, node_ptr_size);
		return -ERANGE;
	}

	if (min_index_area_size >= (node_size / 2)) {
		SSDFS_ERR("invalid option: min_index_area_size %u, "
			  "node_size %u\n",
			  min_index_area_size,
			  node_size);
		return -ERANGE;
	}

	desc->index_area_min_size = cpu_to_le16(min_index_area_size);

	SSDFS_DBG(layout->env.show_debug,
		  "dentries tree's descriptor: "
		  "node_size %u, node_ptr_size %zu, "
		  "index_size %zu, item_size %zu, "
		  "min_index_area_size %u\n",
		  node_size, node_ptr_size,
		  sizeof(struct ssdfs_btree_index),
		  sizeof(struct ssdfs_dir_entry),
		  min_index_area_size);

	return 0;
}

static int sb_extents_btree_desc_prepare(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_btree_descriptor *desc;
	u32 erasesize;
	u32 pagesize;
	u32 node_size;
	size_t hdr_size = sizeof(struct ssdfs_extents_btree_node_header);
	size_t node_ptr_size = sizeof(struct ssdfs_btree_index_key);
	u16 min_index_area_size;

	memset(&layout->sb.vh.extents_btree, 0,
		sizeof(struct ssdfs_extents_btree_descriptor));

	desc = &layout->sb.vh.extents_btree.desc;

	desc->magic = cpu_to_le32(SSDFS_EXTENTS_BTREE_MAGIC);
	desc->flags = cpu_to_le16(SSDFS_BTREE_DESC_INDEX_AREA_RESIZABLE);
	desc->type = cpu_to_le8((u8)SSDFS_EXTENTS_BTREE);

	erasesize = layout->env.erase_size;
	pagesize = layout->page_size;
	node_size = layout->btree.node_size;

	if (node_size <= 0 || node_size >= U16_MAX) {
		SSDFS_ERR("invalid option: node_size %u\n",
			  node_size);
		return -ERANGE;
	}

	if (node_size < pagesize || node_size % pagesize) {
		SSDFS_ERR("invalid option: node_size %u, pagesize %u \n",
			  node_size, pagesize);
		return -ERANGE;
	}

	if (node_size >= erasesize || erasesize % node_size) {
		SSDFS_ERR("invalid option: node_size %u, erasesize %u \n",
			  node_size, erasesize);
		return -ERANGE;
	}

	desc->log_node_size = cpu_to_le8((u8)ilog2(node_size));
	desc->pages_per_node = cpu_to_le8((u8)(node_size / pagesize));
	desc->node_ptr_size = cpu_to_le8((u8)node_ptr_size);
	desc->index_size = cpu_to_le16((u16)sizeof(struct ssdfs_btree_index));
	desc->item_size = cpu_to_le16((u16)sizeof(struct ssdfs_raw_fork));

	min_index_area_size = layout->btree.min_index_area_size;

	if (min_index_area_size == 0)
		min_index_area_size = (u16)hdr_size;

	if (min_index_area_size <= node_ptr_size ||
	    min_index_area_size % node_ptr_size) {
		SSDFS_ERR("invalid option: min_index_area_size %u, "
			  "node_ptr_size %zu\n",
			  min_index_area_size, node_ptr_size);
		return -ERANGE;
	}

	if (min_index_area_size >= (node_size / 2)) {
		SSDFS_ERR("invalid option: min_index_area_size %u, "
			  "node_size %u\n",
			  min_index_area_size,
			  node_size);
		return -ERANGE;
	}

	desc->index_area_min_size = cpu_to_le16(min_index_area_size);

	SSDFS_DBG(layout->env.show_debug,
		  "extents tree's descriptor: "
		  "node_size %u, node_ptr_size %zu, "
		  "index_size %zu, item_size %zu, "
		  "min_index_area_size %u\n",
		  node_size, node_ptr_size,
		  sizeof(struct ssdfs_btree_index),
		  sizeof(struct ssdfs_raw_fork),
		  min_index_area_size);

	return 0;
}

static int sb_xattrs_btree_desc_prepare(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_btree_descriptor *desc;
	u32 erasesize;
	u32 pagesize;
	u32 node_size;
	size_t hdr_size = sizeof(struct ssdfs_xattrs_btree_node_header);
	size_t node_ptr_size = sizeof(struct ssdfs_btree_index_key);
	u16 min_index_area_size;

	memset(&layout->sb.vh.xattr_btree, 0,
		sizeof(struct ssdfs_xattr_btree_descriptor));

	desc = &layout->sb.vh.xattr_btree.desc;

	desc->magic = cpu_to_le32(SSDFS_XATTR_BTREE_MAGIC);
	desc->flags = cpu_to_le16(SSDFS_BTREE_DESC_INDEX_AREA_RESIZABLE);
	desc->type = cpu_to_le8((u8)SSDFS_XATTR_BTREE);

	erasesize = layout->env.erase_size;
	pagesize = layout->page_size;
	node_size = layout->btree.node_size;

	if (node_size <= 0 || node_size >= U16_MAX) {
		SSDFS_ERR("invalid option: node_size %u\n",
			  node_size);
		return -ERANGE;
	}

	if (node_size < pagesize || node_size % pagesize) {
		SSDFS_ERR("invalid option: node_size %u, pagesize %u \n",
			  node_size, pagesize);
		return -ERANGE;
	}

	if (node_size >= erasesize || erasesize % node_size) {
		SSDFS_ERR("invalid option: node_size %u, erasesize %u \n",
			  node_size, erasesize);
		return -ERANGE;
	}

	desc->log_node_size = cpu_to_le8((u8)ilog2(node_size));
	desc->pages_per_node = cpu_to_le8((u8)(node_size / pagesize));
	desc->node_ptr_size = cpu_to_le8((u8)node_ptr_size);
	desc->index_size = cpu_to_le16((u16)sizeof(struct ssdfs_btree_index));
	desc->item_size = cpu_to_le16((u16)sizeof(struct ssdfs_xattr_entry));

	min_index_area_size = layout->btree.min_index_area_size;

	if (min_index_area_size == 0)
		min_index_area_size = (u16)hdr_size;

	if (min_index_area_size <= node_ptr_size ||
	    min_index_area_size % node_ptr_size) {
		SSDFS_ERR("invalid option: min_index_area_size %u, "
			  "node_ptr_size %zu\n",
			  min_index_area_size, node_ptr_size);
		return -ERANGE;
	}

	if (min_index_area_size >= (node_size / 2)) {
		SSDFS_ERR("invalid option: min_index_area_size %u, "
			  "node_size %u\n",
			  min_index_area_size,
			  node_size);
		return -ERANGE;
	}

	desc->index_area_min_size = cpu_to_le16(min_index_area_size);

	SSDFS_DBG(layout->env.show_debug,
		  "xattrs tree's descriptor: "
		  "node_size %u, node_ptr_size %zu, "
		  "index_size %zu, item_size %zu, "
		  "min_index_area_size %u\n",
		  node_size, node_ptr_size,
		  sizeof(struct ssdfs_btree_index),
		  sizeof(struct ssdfs_xattr_entry),
		  min_index_area_size);

	return 0;
}

static int sb_inodes_btree_desc_prepare(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_btree_descriptor *desc;
	u32 erasesize;
	u32 pagesize;
	u32 node_size;
	size_t inode_size = sizeof(struct ssdfs_inode);
	size_t node_ptr_size = sizeof(struct ssdfs_btree_index_key);
	u16 min_index_area_size;

	desc = &layout->sb.vs.inodes_btree.desc;

	desc->magic = cpu_to_le32(SSDFS_INODES_BTREE_MAGIC);
	desc->flags = cpu_to_le16(0);
	desc->type = cpu_to_le8((u8)SSDFS_INODES_BTREE);

	erasesize = layout->env.erase_size;
	pagesize = layout->page_size;
	node_size = layout->btree.node_size;

	if (node_size <= 0 || node_size >= U16_MAX) {
		SSDFS_ERR("invalid option: node_size %u\n",
			  node_size);
		return -ERANGE;
	}

	if (node_size < pagesize || node_size % pagesize) {
		SSDFS_ERR("invalid option: node_size %u, pagesize %u \n",
			  node_size, pagesize);
		return -ERANGE;
	}

	if (node_size >= erasesize || erasesize % node_size) {
		SSDFS_ERR("invalid option: node_size %u, erasesize %u \n",
			  node_size, erasesize);
		return -ERANGE;
	}

	desc->log_node_size = cpu_to_le8((u8)ilog2(node_size));
	desc->pages_per_node = cpu_to_le8((u8)(node_size / pagesize));
	desc->node_ptr_size = cpu_to_le8((u8)node_ptr_size);
	desc->index_size = cpu_to_le16((u16)sizeof(struct ssdfs_btree_index));
	desc->item_size = cpu_to_le16((u16)inode_size);

	min_index_area_size = layout->btree.min_index_area_size;

	if (min_index_area_size == 0)
		min_index_area_size = (u16)inode_size;

	if (min_index_area_size <= node_ptr_size ||
	    min_index_area_size % node_ptr_size) {
		SSDFS_ERR("invalid option: min_index_area_size %u, "
			  "node_ptr_size %zu\n",
			  min_index_area_size, node_ptr_size);
		return -ERANGE;
	}

	if (min_index_area_size >= (node_size / 2)) {
		SSDFS_ERR("invalid option: min_index_area_size %u, "
			  "node_size %u\n",
			  min_index_area_size,
			  node_size);
		return -ERANGE;
	}

	desc->index_area_min_size = cpu_to_le16(min_index_area_size);

	SSDFS_DBG(layout->env.show_debug,
		  "inodes tree's descriptor: "
		  "node_size %u, node_ptr_size %zu, "
		  "index_size %zu, item_size %zu, "
		  "min_index_area_size %u\n",
		  node_size, node_ptr_size,
		  sizeof(struct ssdfs_btree_index),
		  sizeof(struct ssdfs_inode),
		  min_index_area_size);

	return 0;
}

static void sb_inodes_btree_prepare_root_node(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_btree_inline_root_node *root_node;

	root_node = &layout->sb.vs.inodes_btree.root_node;

	root_node->header.height = cpu_to_le8(SSDFS_BTREE_LEAF_NODE_HEIGHT);
	root_node->header.items_count = cpu_to_le8(0);
	root_node->header.flags = cpu_to_le8(0);
	root_node->header.type = cpu_to_le8(SSDFS_BTREE_ROOT_NODE);
	root_node->header.upper_node_id = cpu_to_le32(SSDFS_BTREE_ROOT_NODE_ID);
	root_node->header.create_cno = cpu_to_le64(SSDFS_CREATE_CNO);
}

static void sb_prepare_root_folder(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_inode *root_folder;
	struct ssdfs_dir_entry *de;
	time_t creation_time;
	__le32 checksum;

	root_folder = &layout->sb.vs.root_folder;

	root_folder->magic = cpu_to_le16(SSDFS_INODE_MAGIC);
	root_folder->mode = cpu_to_le16(LINUX_S_IFDIR | 0755);
	root_folder->flags = cpu_to_le32(0);

	root_folder->uid = cpu_to_le32(getuid());
	root_folder->gid = cpu_to_le32(getgid());

	creation_time = time(NULL);
	root_folder->atime = cpu_to_le64(creation_time);
	root_folder->ctime = cpu_to_le64(creation_time);
	root_folder->mtime = cpu_to_le64(creation_time);
	root_folder->birthtime = cpu_to_le64(creation_time);

	root_folder->atime_nsec = cpu_to_le32(0);
	root_folder->ctime_nsec = cpu_to_le32(0);
	root_folder->mtime_nsec = cpu_to_le32(0);
	root_folder->birthtime_nsec = cpu_to_le32(0);

	root_folder->generation = cpu_to_le64(0);
	root_folder->size = cpu_to_le64(layout->page_size);
	root_folder->blocks = cpu_to_le64(1);
	root_folder->parent_ino = cpu_to_le64(SSDFS_ROOT_INO);

	root_folder->refcount = cpu_to_le32(2);

	root_folder->ino = cpu_to_le64(SSDFS_ROOT_INO);
	root_folder->hash_code = cpu_to_le64(0);
	root_folder->name_len = cpu_to_le16(0);
	root_folder->private_flags =
		cpu_to_le16(SSDFS_INODE_HAS_INLINE_DENTRIES);
	root_folder->count_of.dentries = cpu_to_le32(2);

	de = &root_folder->internal[0].area1.dentries.array[0];

	de->ino = cpu_to_le64(SSDFS_ROOT_INO);
	de->hash_code = cpu_to_le64(0);
	de->name_len = cpu_to_le8(1);
	de->dentry_type = cpu_to_le8(SSDFS_INLINE_DENTRY);
	de->file_type = cpu_to_le8(SSDFS_FT_DIR);
	de->flags = cpu_to_le8(0);

	memcpy((void *)de->inline_string,
		".\0\0\0\0\0\0\0\0\0\0",
		SSDFS_DENTRY_INLINE_NAME_MAX_LEN);

	de = &root_folder->internal[0].area1.dentries.array[1];

	de->ino = cpu_to_le64(SSDFS_ROOT_INO);
	de->hash_code = cpu_to_le64(0);
	de->name_len = cpu_to_le8(2);
	de->dentry_type = cpu_to_le8(SSDFS_INLINE_DENTRY);
	de->file_type = cpu_to_le8(SSDFS_FT_DIR);
	de->flags = cpu_to_le8(0);

	memcpy((void *)de->inline_string,
		"..\0\0\0\0\0\0\0\0\0",
		SSDFS_DENTRY_INLINE_NAME_MAX_LEN);

	root_folder->checksum = 0;
	checksum = ssdfs_crc32_le(root_folder, sizeof(struct ssdfs_inode));
	root_folder->checksum = checksum;
}

static int sb_prepare_inodes_btree(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_inodes_btree *tree;
	u64 feature_compat;
	int err;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	tree = &layout->sb.vs.inodes_btree;

	memset(tree, 0xFF, sizeof(struct ssdfs_inodes_btree));

	err = sb_inodes_btree_desc_prepare(layout);
	if (err) {
		SSDFS_ERR("fail to prepare inodes tree's descriptor: "
			  "err %d\n",
			  err);
		return err;
	}

	tree->allocated_inodes = cpu_to_le64(1);
	tree->free_inodes = cpu_to_le64(0);
	tree->inodes_capacity = cpu_to_le64(1);
	tree->leaf_nodes = cpu_to_le32(0);
	/* inodes tree has the root node */
	tree->nodes_count = cpu_to_le32(1);
	tree->upper_allocated_ino = cpu_to_le64(SSDFS_ROOT_INO);

	sb_inodes_btree_prepare_root_node(layout);
	sb_prepare_root_folder(layout);

	feature_compat = le64_to_cpu(layout->sb.vs.feature_compat);
	feature_compat |= SSDFS_HAS_INODES_TREE_COMPAT_FLAG;
	layout->sb.vs.feature_compat = cpu_to_le64(feature_compat);

	return 0;
}

static
int sb_shared_extents_btree_desc_prepare(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_btree_descriptor *desc;
	u32 erasesize;
	u32 pagesize;
	u32 node_size;
	size_t hdr_size = sizeof(struct ssdfs_extents_btree_node_header);
	size_t node_ptr_size = sizeof(struct ssdfs_btree_index_key);
	u16 min_index_area_size;

	desc = &layout->sb.vs.shared_extents_btree.desc;

	desc->magic = cpu_to_le32(SSDFS_SHARED_EXTENTS_BTREE_MAGIC);
	desc->flags = cpu_to_le16(SSDFS_BTREE_DESC_INDEX_AREA_RESIZABLE);
	desc->type = cpu_to_le8((u8)SSDFS_SHARED_EXTENTS_BTREE);

	erasesize = layout->env.erase_size;
	pagesize = layout->page_size;
	node_size = layout->btree.node_size;

	if (node_size <= 0 || node_size >= U16_MAX) {
		SSDFS_ERR("invalid option: node_size %u\n",
			  node_size);
		return -ERANGE;
	}

	if (node_size < pagesize || node_size % pagesize) {
		SSDFS_ERR("invalid option: node_size %u, pagesize %u \n",
			  node_size, pagesize);
		return -ERANGE;
	}

	if (node_size >= erasesize || erasesize % node_size) {
		SSDFS_ERR("invalid option: node_size %u, erasesize %u \n",
			  node_size, erasesize);
		return -ERANGE;
	}

	desc->log_node_size = cpu_to_le8((u8)ilog2(node_size));
	desc->pages_per_node = cpu_to_le8((u8)(node_size / pagesize));
	desc->node_ptr_size = cpu_to_le8((u8)node_ptr_size);
	desc->index_size = cpu_to_le16((u16)sizeof(struct ssdfs_btree_index));
	desc->item_size = cpu_to_le16((u16)sizeof(struct ssdfs_raw_fork));

	min_index_area_size = layout->btree.min_index_area_size;

	if (min_index_area_size == 0)
		min_index_area_size = (u16)hdr_size;

	if (min_index_area_size <= node_ptr_size ||
	    min_index_area_size % node_ptr_size) {
		SSDFS_ERR("invalid option: min_index_area_size %u, "
			  "node_ptr_size %zu\n",
			  min_index_area_size, node_ptr_size);
		return -ERANGE;
	}

	if (min_index_area_size >= (node_size / 2)) {
		SSDFS_ERR("invalid option: min_index_area_size %u, "
			  "node_size %u\n",
			  min_index_area_size,
			  node_size);
		return -ERANGE;
	}

	desc->index_area_min_size = cpu_to_le16(min_index_area_size);

	SSDFS_DBG(layout->env.show_debug,
		  "inodes tree's descriptor: "
		  "node_size %u, node_ptr_size %zu, "
		  "index_size %zu, item_size %zu, "
		  "min_index_area_size %u\n",
		  node_size, node_ptr_size,
		  sizeof(struct ssdfs_btree_index),
		  sizeof(struct ssdfs_raw_fork),
		  min_index_area_size);

	return 0;
}

static void
sb_shared_extents_btree_prepare_root_node(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_btree_inline_root_node *root_node;

	root_node = &layout->sb.vs.shared_extents_btree.root_node;

	root_node->header.height = cpu_to_le8(SSDFS_BTREE_LEAF_NODE_HEIGHT);
	root_node->header.items_count = cpu_to_le8(0);
	root_node->header.flags = cpu_to_le8(0);
	root_node->header.type = cpu_to_le8(SSDFS_BTREE_ROOT_NODE);
	root_node->header.upper_node_id = cpu_to_le32(SSDFS_BTREE_ROOT_NODE_ID);
	root_node->header.create_cno = cpu_to_le64(SSDFS_CREATE_CNO);
}

static int sb_prepare_shared_extents_btree(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_shared_extents_btree *tree;
	u64 feature_compat;
	int err;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	tree = &layout->sb.vs.shared_extents_btree;

	memset(tree, 0xFF, sizeof(struct ssdfs_shared_extents_btree));

	err = sb_shared_extents_btree_desc_prepare(layout);
	if (err) {
		SSDFS_ERR("fail to prepare shared extents tree's descriptor: "
			  "err %d\n",
			  err);
		return err;
	}

	sb_shared_extents_btree_prepare_root_node(layout);

	feature_compat = le64_to_cpu(layout->sb.vs.feature_compat);
	feature_compat |= SSDFS_HAS_SHARED_EXTENTS_COMPAT_FLAG;
	layout->sb.vs.feature_compat = cpu_to_le64(feature_compat);

	return 0;
}

static int sb_shared_dict_btree_desc_prepare(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_btree_descriptor *desc;
	u32 erasesize;
	u32 pagesize;
	u32 node_size;
	size_t hdr_size = sizeof(struct ssdfs_shared_dictionary_node_header);
	size_t node_ptr_size = sizeof(struct ssdfs_btree_index_key);
	u16 min_index_area_size;

	desc = &layout->sb.vs.shared_dict_btree.desc;

	desc->magic = cpu_to_le32(SSDFS_SHARED_DICT_BTREE_MAGIC);
	desc->flags = cpu_to_le16(SSDFS_BTREE_DESC_INDEX_AREA_RESIZABLE);
	desc->type = cpu_to_le8((u8)SSDFS_SHARED_DICTIONARY_BTREE);

	erasesize = layout->env.erase_size;
	pagesize = layout->page_size;
	node_size = layout->btree.node_size;

	if (node_size <= 0 || node_size >= U16_MAX) {
		SSDFS_ERR("invalid option: node_size %u\n",
			  node_size);
		return -ERANGE;
	}

	if (node_size < pagesize || node_size % pagesize) {
		SSDFS_ERR("invalid option: node_size %u, pagesize %u \n",
			  node_size, pagesize);
		return -ERANGE;
	}

	if (node_size >= erasesize || erasesize % node_size) {
		SSDFS_ERR("invalid option: node_size %u, erasesize %u \n",
			  node_size, erasesize);
		return -ERANGE;
	}

	desc->log_node_size = cpu_to_le8((u8)ilog2(node_size));
	desc->pages_per_node = cpu_to_le8((u8)(node_size / pagesize));
	desc->node_ptr_size = cpu_to_le8((u8)node_ptr_size);
	desc->index_size = cpu_to_le16((u16)sizeof(struct ssdfs_btree_index));
	desc->item_size = cpu_to_le16((u16)SSDFS_MAX_NAME_LEN);

	min_index_area_size = layout->btree.min_index_area_size;

	if (min_index_area_size == 0)
		min_index_area_size = (u16)hdr_size;

	if (min_index_area_size <= node_ptr_size ||
	    min_index_area_size % node_ptr_size) {
		SSDFS_ERR("invalid option: min_index_area_size %u, "
			  "node_ptr_size %zu\n",
			  min_index_area_size, node_ptr_size);
		return -ERANGE;
	}

	if (min_index_area_size >= (node_size / 2)) {
		SSDFS_ERR("invalid option: min_index_area_size %u, "
			  "node_size %u\n",
			  min_index_area_size,
			  node_size);
		return -ERANGE;
	}

	desc->index_area_min_size = cpu_to_le16(min_index_area_size);

	SSDFS_DBG(layout->env.show_debug,
		  "inodes tree's descriptor: "
		  "node_size %u, node_ptr_size %zu, "
		  "index_size %zu, item_size %zu, "
		  "min_index_area_size %u\n",
		  node_size, node_ptr_size,
		  sizeof(struct ssdfs_btree_index),
		  sizeof(struct ssdfs_raw_fork),
		  min_index_area_size);

	return 0;
}

static void
sb_shared_dict_btree_prepare_root_node(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_btree_inline_root_node *root_node;

	root_node = &layout->sb.vs.shared_dict_btree.root_node;

	root_node->header.height = cpu_to_le8(SSDFS_BTREE_LEAF_NODE_HEIGHT);
	root_node->header.items_count = cpu_to_le8(0);
	root_node->header.flags = cpu_to_le8(0);
	root_node->header.type = cpu_to_le8(SSDFS_BTREE_ROOT_NODE);
	root_node->header.upper_node_id = cpu_to_le32(SSDFS_BTREE_ROOT_NODE_ID);
	root_node->header.create_cno = cpu_to_le64(SSDFS_CREATE_CNO);
}

static int sb_prepare_shared_dict_btree(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_shared_dictionary_btree *tree;
	u64 feature_compat;
	int err;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	tree = &layout->sb.vs.shared_dict_btree;

	memset(tree, 0xFF, sizeof(struct ssdfs_shared_dictionary_btree));

	err = sb_shared_dict_btree_desc_prepare(layout);
	if (err) {
		SSDFS_ERR("fail to prepare shared dict tree's descriptor: "
			  "err %d\n",
			  err);
		return err;
	}

	sb_shared_dict_btree_prepare_root_node(layout);

	feature_compat = le64_to_cpu(layout->sb.vs.feature_compat);
	feature_compat |= SSDFS_HAS_SHARED_DICT_COMPAT_FLAG;
	layout->sb.vs.feature_compat = cpu_to_le64(feature_compat);

	return 0;
}

int sb_mkfs_prepare(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_volume_header *vh = &layout->sb.vh;
	struct ssdfs_volume_state *vs = &layout->sb.vs;
	u32 pagesize = layout->page_size;
	u32 segsize = layout->seg_size;
	u32 erasesize = layout->env.erase_size;
	u64 segs_count;
	u64 ctime;
	u64 cno;
	int err;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	err = reserve_segments(layout, SSDFS_SUPERBLOCK);
	if (err) {
		SSDFS_ERR("fail to reserve segments: err %d\n", err);
		return err;
	}

	ctime = ssdfs_current_time_in_nanoseconds();
	cno = SSDFS_CREATE_CNO;
	layout->create_timestamp = ctime;
	layout->create_cno = cno;

	/* volume header initialization */
	vh->magic.common = cpu_to_le32(SSDFS_SUPER_MAGIC);
	vh->magic.version.major = cpu_to_le8(SSDFS_MAJOR_REVISION);
	vh->magic.version.minor = cpu_to_le8(SSDFS_MINOR_REVISION);

	SSDFS_DBG(layout->env.show_debug, "revision: %d.%d\n",
		  le8_to_cpu(vh->magic.version.major),
		  le8_to_cpu(vh->magic.version.minor));

	vh->log_pagesize = cpu_to_le8((u8)ilog2(pagesize));
	vh->log_erasesize = cpu_to_le8((u8)ilog2(erasesize));
	vh->log_segsize = cpu_to_le8((u8)ilog2(segsize));
	vh->log_pebs_per_seg = cpu_to_le8((u8)ilog2(segsize / erasesize));

	SSDFS_DBG(layout->env.show_debug,
		  "log_pagesize %d, log_erasesize %d, "
		  "log_segsize %d, log_pebs_per_seg %d\n",
		  le8_to_cpu(vh->log_pagesize),
		  le8_to_cpu(vh->log_erasesize),
		  le8_to_cpu(vh->log_segsize),
		  le8_to_cpu(vh->log_pebs_per_seg));

	vh->create_time = cpu_to_le64(ctime);
	vh->create_cno = cpu_to_le64(cno);

	SSDFS_DBG(layout->env.show_debug,
		  "create_time %llu, create_cno %llu\n",
		  le64_to_cpu(vh->create_time),
		  le64_to_cpu(vh->create_cno));

	sb_set_lnodes_log_pages(layout);
	sb_set_hnodes_log_pages(layout);
	sb_set_inodes_log_pages(layout);

	err = sb_dentries_btree_desc_prepare(layout);
	if (err) {
		SSDFS_ERR("fail to prepare dentries tree's descriptor: "
			  "err %d\n",
			  err);
		return err;
	}

	err = sb_extents_btree_desc_prepare(layout);
	if (err) {
		SSDFS_ERR("fail to prepare extents tree's descriptor: "
			  "err %d\n",
			  err);
		return err;
	}

	err = sb_xattrs_btree_desc_prepare(layout);
	if (err) {
		SSDFS_ERR("fail to prepare xattrs tree's descriptor: "
			  "err %d\n",
			  err);
		return err;
	}

	/* volume state initialization */
	vs->magic.common = cpu_to_le32(SSDFS_SUPER_MAGIC);
	vs->magic.version.major = cpu_to_le8(SSDFS_MAJOR_REVISION);
	vs->magic.version.minor = cpu_to_le8(SSDFS_MINOR_REVISION);

	segs_count = layout->env.fs_size / segsize;
	vs->nsegs = cpu_to_le64(segs_count);

	SSDFS_DBG(layout->env.show_debug,
		  "segments count %llu\n", le64_to_cpu(vs->nsegs));

	vs->timestamp = cpu_to_le64(ctime);
	vs->cno = cpu_to_le64(cno);

	vs->flags = cpu_to_le32(SSDFS_HAS_INLINE_INODES_TREE);
	vs->state = cpu_to_le16(SSDFS_VALID_FS);
	vs->errors = cpu_to_le16(SSDFS_ERRORS_DEFAULT);

	/* set uuid using libuuid */
	uuid_generate(vs->uuid);

	SSDFS_DBG(layout->env.show_debug,
		  "UUID: %s\n", uuid_string(vs->uuid));

	memcpy(vs->label, layout->volume_label, sizeof(vs->label));

	SSDFS_DBG(layout->env.show_debug, "label: %s\n", vs->label);

	memset(vs->cur_segs, 0xFF, sizeof(__le64) * SSDFS_CUR_SEGS_COUNT);

	vs->migration_threshold = cpu_to_le16(layout->migration_threshold);

	err = prepare_block_bitmap_options(layout);
	if (err) {
		SSDFS_ERR("fail to prepare block bitmap options: "
			  "err %d\n", err);
		return err;
	}

	err = prepare_blk2off_table_options(layout);
	if (err) {
		SSDFS_ERR("fail to prepare offset translation table options: "
			  "err %d\n", err);
		return err;
	}

	err = sb_prepare_inodes_btree(layout);
	if (err) {
		SSDFS_ERR("fail to prepare inodes btree: "
			  "err %d\n", err);
		return err;
	}

	err = sb_prepare_shared_extents_btree(layout);
	if (err) {
		SSDFS_ERR("fail to prepare shared extents btree: "
			  "err %d\n", err);
		return err;
	}

	err = sb_prepare_shared_dict_btree(layout);
	if (err) {
		SSDFS_ERR("fail to prepare shared dictionary btree: "
			  "err %d\n", err);
		return err;
	}

	return 0;
}

int sb_mkfs_validate(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_volume_state *vs = &layout->sb.vs;
	u32 segsize = layout->seg_size;
	u32 pagesize = layout->page_size;
	u64 segs_count;
	u64 free_segs;
	u32 pages_per_seg;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	segs_count = layout->env.fs_size / segsize;
	free_segs = segs_count - layout->segs_capacity;
	pages_per_seg = segsize / pagesize;
	vs->free_pages = cpu_to_le64(free_segs * pages_per_seg);

	SSDFS_DBG(layout->env.show_debug,
		  "free pages %llu\n", le64_to_cpu(vs->free_pages));

	return 0;
}

static void sb_set_log_pages(struct ssdfs_volume_layout *layout,
			     u32 blks)
{
	u32 erasesize;
	u32 pagesize;
	u32 pages_per_peb;

	SSDFS_DBG(layout->env.show_debug,
		  "log_pages %d, blks_count %u\n",
		  layout->sb.log_pages, blks);

	BUG_ON(blks == 0);
	BUG_ON(blks == U16_MAX);

	erasesize = layout->env.erase_size;
	pagesize = layout->page_size;
	pages_per_peb = erasesize / pagesize;

	BUG_ON((blks / 2) > pages_per_peb);

	if (pages_per_peb % blks) {
		SSDFS_WARN("pages_per_peb %u, blks %u\n",
			   pages_per_peb, blks);
	}

	layout->sb.log_pages = blks;
	layout->sb.vh.sb_seg_log_pages = cpu_to_le16(blks);
}

int sb_mkfs_define_layout(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_metadata_desc *desc;
	int segs_count;
	int i, j, k;
	int seg_index;
	int peb_index = 0;
	u32 fragments;
	u32 log_pages = 0;
	size_t hdr_size = sizeof(struct ssdfs_segment_header);
	u32 inline_capacity = PAGE_CACHE_SIZE - hdr_size;
	int err;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	desc = &layout->meta_array[SSDFS_SUPERBLOCK];
	segs_count = desc->segs_count;

	if (segs_count <= 0 ||
	    segs_count > (SSDFS_SB_CHAIN_MAX * SSDFS_SB_SEG_COPY_MAX)) {
		SSDFS_ERR("invalid segs_count %d\n", segs_count);
		return -ERANGE;
	}

	if (desc->start_seg_index >= layout->segs_capacity) {
		SSDFS_ERR("start_seg_index %d >= segs_capacity %d\n",
			  desc->start_seg_index,
			  layout->segs_capacity);
		return -ERANGE;
	}

	if ((layout->segs_count + segs_count) > layout->segs_capacity) {
		SSDFS_ERR("not enough space for commit: "
			  "segs_count %d, request %d, capacity %d\n",
			  layout->segs_count, segs_count,
			  layout->segs_capacity);
		return -E2BIG;
	}

	seg_index = desc->start_seg_index;
	fragments = layout->maptbl_cache.fragments_count;

	for (i = 0; i <= SSDFS_RESERVED_SB_SEG; i++) {
		for (j = 0; j < SSDFS_SB_SEG_COPY_MAX; j++) {
			struct ssdfs_peb_content *peb_desc;
			struct ssdfs_extent_desc *extent;
			size_t peb_buffer_size;
			u32 blks;

			if (i != SSDFS_CUR_SB_SEG)
				goto inc_seg_index;

			layout->segs[seg_index].pebs_count = 1;
			BUG_ON(layout->segs[seg_index].pebs_count >
				layout->segs[seg_index].pebs_capacity);
			peb_desc = &layout->segs[seg_index].pebs[peb_index];

			err = set_extent_start_offset(layout, peb_desc,
							SSDFS_SEG_HEADER);
			if (err) {
				SSDFS_ERR("fail to define extent's offset: "
					  "err %d\n", err);
				return err;
			}

			err = define_segment_header_layout(layout, seg_index,
							   peb_index);
			if (err) {
				SSDFS_ERR("fail to define seg header's layout: "
					  "err %d\n", err);
				return err;
			}

			err = set_extent_start_offset(layout, peb_desc,
							SSDFS_MAPTBL_CACHE);
			if (err) {
				SSDFS_ERR("fail to define extent's offset: "
					  "err %d\n", err);
				return err;
			}

			extent = &peb_desc->extents[SSDFS_MAPTBL_CACHE];

			BUG_ON(extent->buf);
			BUG_ON(layout->maptbl_cache.fragment_size !=
				PAGE_CACHE_SIZE);

			peb_buffer_size = fragments * PAGE_CACHE_SIZE;

			extent->buf = malloc(peb_buffer_size);
			if (!extent->buf) {
				SSDFS_ERR("fail to allocate memory: "
					  "size %zu\n",
					  peb_buffer_size);
				return -ENOMEM;
			}

			for (k = 0; k < fragments; k++) {
				void *sptr;
				u8 *dptr;

				sptr = layout->maptbl_cache.fragments_array[k];
				dptr = (u8 *)extent->buf +
						(k * PAGE_CACHE_SIZE);

				BUG_ON(!sptr || !dptr);

				memcpy(dptr, sptr, PAGE_CACHE_SIZE);
			}

			extent->bytes_count = layout->maptbl_cache.bytes_count;

			if (extent->bytes_count <= inline_capacity) {
				struct ssdfs_extent_desc *sh_extent;

				sh_extent =
					&peb_desc->extents[SSDFS_SEG_HEADER];

				extent->offset = sh_extent->offset +
							sh_extent->bytes_count;
			}

			err = set_extent_start_offset(layout, peb_desc,
							SSDFS_LOG_FOOTER);
			if (err) {
				SSDFS_ERR("fail to define extent's offset: "
					  "err %d\n", err);
				return err;
			}

			err = define_log_footer_layout(layout, seg_index,
							peb_index);
			if (err) {
				SSDFS_ERR("fail to define seg footer's layout: "
					  "err %d\n", err);
				return err;
			}

			blks = calculate_log_pages(layout, peb_desc);
			log_pages = max_t(u32, blks, log_pages);

inc_seg_index:
			seg_index++;
		}
	}

	for (k = 0; k < fragments; k++) {
		free(layout->maptbl_cache.fragments_array[k]);
		layout->maptbl_cache.fragments_array[k] = NULL;
	}

	sb_set_log_pages(layout, log_pages);

	return 0;
}

int sb_mkfs_commit(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_metadata_desc *desc;
	int segs_count;
	int i, j;
	int seg_index;
	int peb_index = 0;
	int err;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	desc = &layout->meta_array[SSDFS_SUPERBLOCK];
	segs_count = desc->segs_count;

	if (segs_count <= 0 ||
	    segs_count > (SSDFS_SB_CHAIN_MAX * SSDFS_SB_SEG_COPY_MAX)) {
		SSDFS_ERR("invalid segs_count %d\n", segs_count);
		return -ERANGE;
	}

	if (desc->start_seg_index >= layout->segs_capacity) {
		SSDFS_ERR("start_seg_index %d >= segs_capacity %d\n",
			  desc->start_seg_index,
			  layout->segs_capacity);
		return -ERANGE;
	}

	seg_index = desc->start_seg_index;

	for (i = 0; i < (SSDFS_SB_CHAIN_MAX - 1); i++) {
		for (j = 0; j < SSDFS_SB_SEG_COPY_MAX; j++) {
			struct ssdfs_peb_content *peb_desc;
			u32 blks;

			if (i != SSDFS_CUR_SB_SEG)
				goto inc_seg_index;

			err = pre_commit_segment_header(layout, seg_index,
							peb_index,
							SSDFS_SB_SEG_TYPE);
			if (err)
				return err;

			err = pre_commit_log_footer(layout, seg_index,
						    peb_index);
			if (err)
				return err;

			peb_desc = &layout->segs[seg_index].pebs[peb_index];
			blks = calculate_log_pages(layout, peb_desc);
			commit_log_footer(layout, seg_index, peb_index, blks);
			commit_segment_header(layout, seg_index, peb_index,
					      blks);

inc_seg_index:
			seg_index++;
		}
	}

	layout->segs_count += segs_count;
	return 0;
}
