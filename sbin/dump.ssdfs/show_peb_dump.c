//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/dump.ssdfs/show_peb_dump.c - show PEB dump command.
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

#include "dumpfs.h"

/************************************************************************
 *                     Show PEB dump command                            *
 ************************************************************************/

union ssdfs_log_header {
	struct ssdfs_segment_header seg_hdr;
	struct ssdfs_partial_log_header pl_hdr;
	struct ssdfs_signature magic;
};

static
int is_ssdfs_dumpfs_area_valid(struct ssdfs_metadata_descriptor *desc)
{
	u32 area_offset = le32_to_cpu(desc->offset);
	u32 area_size = le32_to_cpu(desc->size);

	if (area_size == 0 || area_size >= U32_MAX)
		return SSDFS_FALSE;

	if (area_offset == 0 || area_offset >= U32_MAX)
		return SSDFS_FALSE;

	return SSDFS_TRUE;
}

static
void ssdfs_dumpfs_parse_magic(struct ssdfs_dumpfs_environment *env,
			      struct ssdfs_signature *magic)
{
	u8 *magic_common = (u8 *)&magic->common;
	u8 *magic_key = (u8 *)&magic->key;

	SSDFS_DUMPFS_DUMP(env, "MAGIC: %c%c%c%c %c%c\n",
			  *magic_common, *(magic_common + 1),
			  *(magic_common + 2), *(magic_common + 3),
			  *magic_key, *(magic_key + 1));
	SSDFS_DUMPFS_DUMP(env, "VERSION: v.%u.%u\n",
			  magic->version.major,
			  magic->version.minor);
}

static void
ssdfs_dumpfs_parse_fragments_chain_hdr(struct ssdfs_dumpfs_environment *env,
					struct ssdfs_fragments_chain_header *hdr)
{
	u32 compr_bytes;
	u32 uncompr_bytes;
	u16 fragments_count;
	u16 desc_size;
	u16 flags;

	compr_bytes = le32_to_cpu(hdr->compr_bytes);
	uncompr_bytes = le32_to_cpu(hdr->uncompr_bytes);
	fragments_count = le16_to_cpu(hdr->fragments_count);
	desc_size = le16_to_cpu(hdr->desc_size);
	flags = le16_to_cpu(hdr->flags);

	SSDFS_DUMPFS_DUMP(env, "CHAIN HEADER:\n");
	SSDFS_DUMPFS_DUMP(env, "COMPRESSED BYTES: %u bytes\n", compr_bytes);
	SSDFS_DUMPFS_DUMP(env, "UNCOMPRESSED BYTES: %u bytes\n", uncompr_bytes);
	SSDFS_DUMPFS_DUMP(env, "FRAGMENTS COUNT: %u\n", fragments_count);
	SSDFS_DUMPFS_DUMP(env, "DESC_SIZE: %u bytes\n", desc_size);
	SSDFS_DUMPFS_DUMP(env, "MAGIC: %c\n", hdr->magic);

	switch (hdr->type) {
	case SSDFS_LOG_AREA_CHAIN_HDR:
		SSDFS_DUMPFS_DUMP(env,
			"CHAIN TYPE: SSDFS_LOG_AREA_CHAIN_HDR\n");
		break;

	case SSDFS_BLK_STATE_CHAIN_HDR:
		SSDFS_DUMPFS_DUMP(env,
			"CHAIN TYPE: SSDFS_BLK_STATE_CHAIN_HDR\n");
		break;
	case SSDFS_BLK_DESC_CHAIN_HDR:
		SSDFS_DUMPFS_DUMP(env,
			"CHAIN TYPE: SSDFS_BLK_DESC_CHAIN_HDR\n");
		break;

	case SSDFS_BLK_BMAP_CHAIN_HDR:
		SSDFS_DUMPFS_DUMP(env,
			"CHAIN TYPE: SSDFS_BLK_BMAP_CHAIN_HDR\n");
		break;

	default:
		SSDFS_DUMPFS_DUMP(env, "CHAIN TYPE: UNKNOWN\n");
		break;
	}

	SSDFS_DUMPFS_DUMP(env, "CHAIN FLAGS: ");

	if (flags & SSDFS_MULTIPLE_HDR_CHAIN)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_MULTIPLE_HDR_CHAIN ");

	if (flags == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");

	SSDFS_DUMPFS_DUMP(env, "\n");
}

static
void ssdfs_dumpfs_parse_fragment_header(struct ssdfs_dumpfs_environment *env,
					struct ssdfs_fragment_desc *hdr)
{
	SSDFS_DUMPFS_DUMP(env, "FRAGMENT HEADER:\n");
	SSDFS_DUMPFS_DUMP(env, "OFFSET: %u bytes\n",
		   le32_to_cpu(hdr->offset));
	SSDFS_DUMPFS_DUMP(env, "COMPRESSED_SIZE: %u bytes\n",
		   le16_to_cpu(hdr->compr_size));
	SSDFS_DUMPFS_DUMP(env, "UNCOMPRESSED_SIZE: %u bytes\n",
		   le16_to_cpu(hdr->uncompr_size));
	SSDFS_DUMPFS_DUMP(env, "CHECKSUM: %#x\n",
		   le32_to_cpu(hdr->checksum));
	SSDFS_DUMPFS_DUMP(env, "SEQUENCE_ID: %u\n",
		   hdr->sequence_id);
	SSDFS_DUMPFS_DUMP(env, "MAGIC: %c\n",
		   hdr->magic);

	switch (hdr->type) {
	case SSDFS_FRAGMENT_UNCOMPR_BLOB:
		SSDFS_DUMPFS_DUMP(env,
			"FRAGMENT TYPE: SSDFS_FRAGMENT_UNCOMPR_BLOB\n");
		break;

	case SSDFS_FRAGMENT_ZLIB_BLOB:
		SSDFS_DUMPFS_DUMP(env,
			"FRAGMENT TYPE: SSDFS_FRAGMENT_ZLIB_BLOB\n");
		break;

	case SSDFS_FRAGMENT_LZO_BLOB:
		SSDFS_DUMPFS_DUMP(env,
			"FRAGMENT TYPE: SSDFS_FRAGMENT_LZO_BLOB\n");
		break;

	case SSDFS_DATA_BLK_STATE_DESC:
		SSDFS_DUMPFS_DUMP(env,
			"FRAGMENT TYPE: SSDFS_DATA_BLK_STATE_DESC\n");
		break;

	case SSDFS_DATA_BLK_DESC:
		SSDFS_DUMPFS_DUMP(env,
			"FRAGMENT TYPE: SSDFS_DATA_BLK_DESC\n");
		break;

	case SSDFS_NEXT_TABLE_DESC:
		SSDFS_DUMPFS_DUMP(env,
			"FRAGMENT TYPE: SSDFS_NEXT_TABLE_DESC\n");
		break;

	default:
		SSDFS_DUMPFS_DUMP(env, "FRAGMENT TYPE: UNKNOWN\n");
		break;
	}

	SSDFS_DUMPFS_DUMP(env, "FRAGMENT FLAGS: ");

	if (hdr->flags & SSDFS_FRAGMENT_HAS_CSUM)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_FRAGMENT_HAS_CSUM ");

	if (hdr->flags == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");

	SSDFS_DUMPFS_DUMP(env, "\n");
}

static
void ssdfs_dumpfs_parse_btree_descriptor(struct ssdfs_dumpfs_environment *env,
					 struct ssdfs_btree_descriptor *desc)
{
	u8 *magic = (u8 *)&desc->magic;
	u16 flags = le16_to_cpu(desc->flags);
	u32 node_size = 1 << desc->log_node_size;
	u16 index_size = le16_to_cpu(desc->index_size);
	u16 item_size = le16_to_cpu(desc->item_size);
	u16 index_area_min_size = le16_to_cpu(desc->index_area_min_size);

	SSDFS_DUMPFS_DUMP(env, "B-TREE HEADER:\n");

	SSDFS_DUMPFS_DUMP(env, "MAGIC: %c%c%c%c\n",
			  *magic, *(magic + 1),
			  *(magic + 2), *(magic + 3));

	SSDFS_DUMPFS_DUMP(env, "B-TREE FLAGS: ");

	if (flags & SSDFS_BTREE_DESC_INDEX_AREA_RESIZABLE)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_BTREE_DESC_INDEX_AREA_RESIZABLE ");

	if (flags == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");

	SSDFS_DUMPFS_DUMP(env, "\n");

	switch (desc->type) {
	case SSDFS_INODES_BTREE:
		SSDFS_DUMPFS_DUMP(env,
			"B-TREE TYPE: SSDFS_INODES_BTREE\n");
		break;

	case SSDFS_DENTRIES_BTREE:
		SSDFS_DUMPFS_DUMP(env,
			"B-TREE TYPE: SSDFS_DENTRIES_BTREE\n");
		break;

	case SSDFS_EXTENTS_BTREE:
		SSDFS_DUMPFS_DUMP(env,
			"B-TREE TYPE: SSDFS_EXTENTS_BTREE\n");
		break;

	case SSDFS_SHARED_EXTENTS_BTREE:
		SSDFS_DUMPFS_DUMP(env,
			"B-TREE TYPE: SSDFS_SHARED_EXTENTS_BTREE\n");
		break;

	case SSDFS_XATTR_BTREE:
		SSDFS_DUMPFS_DUMP(env,
			"B-TREE TYPE: SSDFS_XATTR_BTREE\n");
		break;

	case SSDFS_SHARED_XATTR_BTREE:
		SSDFS_DUMPFS_DUMP(env,
			"B-TREE TYPE: SSDFS_SHARED_XATTR_BTREE\n");
		break;

	case SSDFS_SHARED_DICTIONARY_BTREE:
		SSDFS_DUMPFS_DUMP(env,
			"B-TREE TYPE: SSDFS_SHARED_DICTIONARY_BTREE\n");
		break;

	default:
		SSDFS_DUMPFS_DUMP(env,
			"B-TREE TYPE: UNKNOWN_BTREE_TYPE\n");
		break;
	}

	SSDFS_DUMPFS_DUMP(env, "NODE_SIZE: %u bytes\n",
						node_size);
	SSDFS_DUMPFS_DUMP(env, "PAGES_PER_NODE: %u\n",
						desc->pages_per_node);
	SSDFS_DUMPFS_DUMP(env, "NODE_PTR_SIZE: %u bytes\n",
						desc->node_ptr_size);
	SSDFS_DUMPFS_DUMP(env, "INDEX_SIZE: %u bytes\n",
						index_size);
	SSDFS_DUMPFS_DUMP(env, "ITEM_SIZE: %u bytes\n",
						item_size);
	SSDFS_DUMPFS_DUMP(env, "INDEX_AREA_MIN_SIZE: %u bytes\n",
						index_area_min_size);
}

static void
ssdfs_dumpfs_parse_dentries_btree_descriptor(struct ssdfs_dumpfs_environment *env,
				struct ssdfs_dentries_btree_descriptor *tree)
{
	SSDFS_DUMPFS_DUMP(env, "DENTRIES B-TREE HEADER:\n");

	ssdfs_dumpfs_parse_btree_descriptor(env, &tree->desc);
}

static void
ssdfs_dumpfs_parse_extents_btree_descriptor(struct ssdfs_dumpfs_environment *env,
				struct ssdfs_extents_btree_descriptor *tree)
{
	SSDFS_DUMPFS_DUMP(env, "EXTENTS B-TREE HEADER:\n");

	ssdfs_dumpfs_parse_btree_descriptor(env, &tree->desc);
}

static void
ssdfs_dumpfs_parse_xattr_btree_descriptor(struct ssdfs_dumpfs_environment *env,
					struct ssdfs_xattr_btree_descriptor *tree)
{
	SSDFS_DUMPFS_DUMP(env, "XATTRS B-TREE HEADER:\n");

	ssdfs_dumpfs_parse_btree_descriptor(env, &tree->desc);
}

static
void ssdfs_dumpfs_parse_raw_inode(struct ssdfs_dumpfs_environment *env,
				  struct ssdfs_inode *inode)
{
	u8 *magic = (u8 *)&inode->magic;
	u16 flags;

	SSDFS_DUMPFS_DUMP(env, "RAW INODE:\n");
	SSDFS_DUMPFS_DUMP(env, "MAGIC: %c%c\n",
			  *magic, *(magic + 1));
	SSDFS_DUMPFS_DUMP(env, "MODE: %#x\n", le16_to_cpu(inode->mode));
	SSDFS_DUMPFS_DUMP(env, "FLAGS: %#x\n", le32_to_cpu(inode->flags));
	SSDFS_DUMPFS_DUMP(env, "UID: %#x\n", le32_to_cpu(inode->uid));
	SSDFS_DUMPFS_DUMP(env, "GID: %#x\n", le32_to_cpu(inode->gid));

	SSDFS_DUMPFS_DUMP(env, "ACCESS TIME: %llu\n",
				le64_to_cpu(inode->atime));
	SSDFS_DUMPFS_DUMP(env, "ACCESS TIME NSEC: %u\n",
				le32_to_cpu(inode->atime_nsec));
	SSDFS_DUMPFS_DUMP(env, "CHANGE TIME: %llu\n",
				le64_to_cpu(inode->ctime));
	SSDFS_DUMPFS_DUMP(env, "CHANGE TIME NSEC: %u\n",
				le32_to_cpu(inode->ctime_nsec));
	SSDFS_DUMPFS_DUMP(env, "MODIFICATION TIME: %llu\n",
				le64_to_cpu(inode->mtime));
	SSDFS_DUMPFS_DUMP(env, "MODIFICATION TIME NSEC: %u\n",
				le32_to_cpu(inode->mtime_nsec));
	SSDFS_DUMPFS_DUMP(env, "BIRTH TIME: %llu\n",
				le64_to_cpu(inode->birthtime));
	SSDFS_DUMPFS_DUMP(env, "BIRTH TIME NSEC: %u\n",
		le32_to_cpu(inode->birthtime_nsec));

	SSDFS_DUMPFS_DUMP(env, "FILE VERSION (NFS): %llu\n",
		le64_to_cpu(inode->generation));
	SSDFS_DUMPFS_DUMP(env, "FILE SIZE: %llu bytes\n",
		le64_to_cpu(inode->size));
	SSDFS_DUMPFS_DUMP(env, "BLOCKS: %llu\n",
		le64_to_cpu(inode->blocks));
	SSDFS_DUMPFS_DUMP(env, "PARENT_INO: %llu\n",
		le64_to_cpu(inode->parent_ino));
	SSDFS_DUMPFS_DUMP(env, "LINKS COUNT: %u\n",
		le32_to_cpu(inode->refcount));
	SSDFS_DUMPFS_DUMP(env, "CHECKSUM: %#x\n",
		le32_to_cpu(inode->checksum));
	SSDFS_DUMPFS_DUMP(env, "INODE ID: %llu\n",
		le64_to_cpu(inode->ino));
	SSDFS_DUMPFS_DUMP(env, "FILE NAME HASH CODE: %#llx\n",
		le64_to_cpu(inode->hash_code));
	SSDFS_DUMPFS_DUMP(env, "NAME LENGTH: %u\n",
		le16_to_cpu(inode->name_len));

	flags = le16_to_cpu(inode->private_flags);

	SSDFS_DUMPFS_DUMP(env, "INODE PRIVATE FLAGS: ");

	if (flags & SSDFS_INODE_HAS_INLINE_EXTENTS)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_INODE_HAS_INLINE_EXTENTS ");

	if (flags & SSDFS_INODE_HAS_EXTENTS_BTREE)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_INODE_HAS_EXTENTS_BTREE ");

	if (flags & SSDFS_INODE_HAS_INLINE_DENTRIES)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_INODE_HAS_INLINE_DENTRIES ");

	if (flags & SSDFS_INODE_HAS_DENTRIES_BTREE)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_INODE_HAS_DENTRIES_BTREE ");

	if (flags & SSDFS_INODE_HAS_INLINE_XATTR)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_INODE_HAS_INLINE_XATTR ");

	if (flags & SSDFS_INODE_HAS_XATTR_BTREE)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_INODE_HAS_XATTR_BTREE ");

	if (flags & SSDFS_INODE_HAS_INLINE_FILE)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_INODE_HAS_INLINE_FILE ");

	if (flags == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "COUNT OF (FORKS/DENTRIES): %u\n",
		le32_to_cpu(inode->count_of.forks));

	/* TODO: parse struct ssdfs_inode_private_area internal */
}

static void
ssdfs_dumpfs_parse_inline_root_node(struct ssdfs_dumpfs_environment *env,
				    struct ssdfs_btree_inline_root_node *ptr)
{
	u8 flags;
	int i;

	SSDFS_DUMPFS_DUMP(env, "BTREE INLINE ROOT NODE:\n");
	SSDFS_DUMPFS_DUMP(env, "BTREE HEIGHT: %u\n", ptr->header.height);
	SSDFS_DUMPFS_DUMP(env, "ROOT NODE's ITEMS_COUNT: %u\n",
						ptr->header.items_count);

	flags = ptr->header.flags;

	SSDFS_DUMPFS_DUMP(env, "ROOT NODE FLAGS: ");

	if (flags == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");
	else
		SSDFS_DUMPFS_DUMP(env, "%#x", flags);

	SSDFS_DUMPFS_DUMP(env, "\n");

	switch (ptr->header.type) {
	case SSDFS_BTREE_ROOT_NODE:
		SSDFS_DUMPFS_DUMP(env, "NODE TYPE: SSDFS_BTREE_ROOT_NODE\n");
		break;

	case SSDFS_BTREE_INDEX_NODE:
		SSDFS_DUMPFS_DUMP(env, "NODE TYPE: SSDFS_BTREE_INDEX_NODE\n");
		break;

	case SSDFS_BTREE_HYBRID_NODE:
		SSDFS_DUMPFS_DUMP(env, "NODE TYPE: SSDFS_BTREE_HYBRID_NODE\n");
		break;

	case SSDFS_BTREE_LEAF_NODE:
		SSDFS_DUMPFS_DUMP(env, "NODE TYPE: SSDFS_BTREE_LEAF_NODE\n");
		break;

	default:
		SSDFS_DUMPFS_DUMP(env, "NODE TYPE: UNKNOWN\n");
		break;
	}

	SSDFS_DUMPFS_DUMP(env, "UPPER NODE ID: %u\n",
		le32_to_cpu(ptr->header.upper_node_id));
	SSDFS_DUMPFS_DUMP(env, "NODE_ID: left %u, right %u\n",
		le32_to_cpu(ptr->header.node_ids[0]),
		le32_to_cpu(ptr->header.node_ids[1]));

	for (i = 0; i < SSDFS_BTREE_ROOT_NODE_INDEX_COUNT; i++) {
		SSDFS_DUMPFS_DUMP(env, "BTREE INDEX: #%d\n", i);
		SSDFS_DUMPFS_DUMP(env, "HASH: %llu\n",
			le64_to_cpu(ptr->indexes[i].hash));
		SSDFS_DUMPFS_DUMP(env, "SEGMENT ID: %llu\n",
			le64_to_cpu(ptr->indexes[i].extent.seg_id));
		SSDFS_DUMPFS_DUMP(env, "LOGICAL BLOCK: %u\n",
			le32_to_cpu(ptr->indexes[i].extent.logical_blk));
		SSDFS_DUMPFS_DUMP(env, "LENGTH: %u\n",
			le32_to_cpu(ptr->indexes[i].extent.len));
	}
}

static int
ssdfs_dumpfs_parse_block_bitmap_fragment(struct ssdfs_dumpfs_environment *env,
					 void *area_buf,
					 u32 offset, u32 size,
					 u32 *parsed_bytes)
{
	struct ssdfs_block_bitmap_fragment *hdr;
	u16 peb_index;
	u16 sequence_id;
	u16 flags;
	u16 type;
	u16 last_free_blk;
	u16 metadata_blks;
	u16 invalid_blks;
	u16 fragments_count;
	int i;
	int res;

	*parsed_bytes = 0;

	if (size < sizeof(struct ssdfs_block_bitmap_fragment)) {
		SSDFS_ERR("size %u is lesser than %zu\n",
			  size,
			  sizeof(struct ssdfs_block_bitmap_fragment));
		return -EINVAL;
	}

	hdr = (struct ssdfs_block_bitmap_fragment *)((u8 *)area_buf + offset);
	peb_index = le16_to_cpu(hdr->peb_index);
	sequence_id = le16_to_cpu(hdr->sequence_id);
	flags = le16_to_cpu(hdr->flags);
	type = le16_to_cpu(hdr->type);
	last_free_blk = le16_to_cpu(hdr->last_free_blk);
	metadata_blks = le16_to_cpu(hdr->metadata_blks);
	invalid_blks = le16_to_cpu(hdr->invalid_blks);

	SSDFS_DUMPFS_DUMP(env, "PEB_INDEX: %u\n", peb_index);
	SSDFS_DUMPFS_DUMP(env, "SEQUENCE_ID: %u\n", sequence_id);

	SSDFS_DUMPFS_DUMP(env, "FRAGMENT FLAGS: ");

	if (flags & SSDFS_MIGRATING_BLK_BMAP)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_MIGRATING_BLK_BMAP ");

	if (flags & SSDFS_PEB_HAS_EXT_PTR)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_PEB_HAS_EXT_PTR ");

	if (flags & SSDFS_PEB_HAS_RELATION)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_PEB_HAS_RELATION ");

	if (flags == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");

	SSDFS_DUMPFS_DUMP(env, "\n");

	switch (type) {
	case SSDFS_SRC_BLK_BMAP:
		SSDFS_DUMPFS_DUMP(env, "FRAGMENT TYPE: SSDFS_SRC_BLK_BMAP\n");
		break;

	case SSDFS_DST_BLK_BMAP:
		SSDFS_DUMPFS_DUMP(env, "FRAGMENT TYPE: SSDFS_DST_BLK_BMAP\n");
		break;

	default:
		SSDFS_DUMPFS_DUMP(env, "FRAGMENT TYPE: UNKNOWN\n");
		break;
	}

	SSDFS_DUMPFS_DUMP(env, "LAST_FREE_BLK: %u\n", last_free_blk);
	SSDFS_DUMPFS_DUMP(env, "METADATA_BLKS: %u\n", metadata_blks);
	SSDFS_DUMPFS_DUMP(env, "INVALID_BLKS: %u\n", invalid_blks);

	ssdfs_dumpfs_parse_fragments_chain_hdr(env, &hdr->chain_hdr);

	*parsed_bytes += sizeof(struct ssdfs_block_bitmap_fragment);

	fragments_count = le16_to_cpu(hdr->chain_hdr.fragments_count);

	for (i = 0; i < fragments_count; i++) {
		size_t frag_desc_size = sizeof(struct ssdfs_fragment_desc);
		struct ssdfs_fragment_desc *frag;
		u8 *data;

		if ((size - *parsed_bytes) < frag_desc_size) {
			SSDFS_ERR("size %u is lesser than %zu\n",
				  size - *parsed_bytes,
				  frag_desc_size);
			return -EINVAL;
		}

		frag = (struct ssdfs_fragment_desc *)((u8 *)area_buf + offset +
							*parsed_bytes);

		SSDFS_DUMPFS_DUMP(env, "\n");
		SSDFS_DUMPFS_DUMP(env, "FRAGMENT_INDEX: #%d\n", i);

		ssdfs_dumpfs_parse_fragment_header(env, frag);

		*parsed_bytes += frag_desc_size;

		if ((size - *parsed_bytes) < le16_to_cpu(frag->compr_size)) {
			SSDFS_ERR("size %u is lesser than %u\n",
				  size - *parsed_bytes,
				  le16_to_cpu(frag->compr_size));
			return -EINVAL;
		}

		data = (u8 *)area_buf + offset + *parsed_bytes;

		SSDFS_DUMPFS_DUMP(env, "RAW DATA:\n");

		res = ssdfs_dumpfs_show_raw_string(env, 0, data,
						le16_to_cpu(frag->compr_size));
		if (res < 0) {
			SSDFS_ERR("fail to dump raw data: size %u, err %d\n",
				  le16_to_cpu(frag->compr_size), res);
			return res;
		}

		*parsed_bytes += le16_to_cpu(frag->compr_size);
	}

	SSDFS_DUMPFS_DUMP(env, "\n");

	return 0;
}

static
int ssdfs_dumpfs_parse_block_bitmap(struct ssdfs_dumpfs_environment *env,
				    void *area_buf, u32 area_size)
{
	struct ssdfs_block_bitmap_header *hdr =
			(struct ssdfs_block_bitmap_header *)area_buf;
	u16 fragments_count = le16_to_cpu(hdr->fragments_count);
	u32 bytes_count = le32_to_cpu(hdr->bytes_count);
	u8 flags = hdr->flags;
	u8 type = hdr->type;
	u32 offset;
	u32 size;
	int i;
	int err;

	if (area_size < bytes_count) {
		SSDFS_ERR("area_size %u < bytes_count %u\n",
			  area_size, bytes_count);
		return -EINVAL;
	}

	SSDFS_DUMPFS_DUMP(env, "BLOCK BITMAP:\n");

	ssdfs_dumpfs_parse_magic(env, &hdr->magic);

	SSDFS_DUMPFS_DUMP(env, "FRAGMENTS_COUNT: %u\n", fragments_count);
	SSDFS_DUMPFS_DUMP(env, "BYTES_COUNT: %u bytes\n", bytes_count);

	SSDFS_DUMPFS_DUMP(env, "BLOCK BITMAP FLAGS: ");

	if (flags & SSDFS_BLK_BMAP_BACKUP)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_BLK_BMAP_BACKUP ");

	if (flags & SSDFS_BLK_BMAP_COMPRESSED)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_BLK_BMAP_COMPRESSED ");

	if (flags == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");

	SSDFS_DUMPFS_DUMP(env, "\n");

	switch (type) {
	case SSDFS_BLK_BMAP_UNCOMPRESSED_BLOB:
		SSDFS_DUMPFS_DUMP(env,
		    "BLOCK BITMAP TYPE: SSDFS_BLK_BMAP_UNCOMPRESSED_BLOB\n");
		break;

	case SSDFS_BLK_BMAP_ZLIB_BLOB:
		SSDFS_DUMPFS_DUMP(env,
			"BLOCK BITMAP TYPE: SSDFS_BLK_BMAP_ZLIB_BLOB\n");
		break;

	case SSDFS_BLK_BMAP_LZO_BLOB:
		SSDFS_DUMPFS_DUMP(env,
			"BLOCK BITMAP TYPE: SSDFS_BLK_BMAP_LZO_BLOB\n");
		break;

	default:
		SSDFS_DUMPFS_DUMP(env, "BLOCK BITMAP TYPE: UNKNOWN\n");
		break;
	}

	SSDFS_DUMPFS_DUMP(env, "\n");

	offset = sizeof(struct ssdfs_block_bitmap_header);
	size = area_size - offset;

	for (i = 0; i < fragments_count; i++) {
		u32 parsed_bytes = 0;

		SSDFS_DUMPFS_DUMP(env, "BLOCK BITMAP FRAGMENT: #%d\n", i);

		err = ssdfs_dumpfs_parse_block_bitmap_fragment(env, area_buf,
								offset, size,
								&parsed_bytes);
		if (err) {
			SSDFS_ERR("fail to parse block bitmap fragment: "
				  "offset %u, size %u, err %d\n",
				  offset, size, err);
			return err;
		}

		offset += parsed_bytes;
		size = area_size - offset;
	}

	return 0;
}

static
int ssdfs_dumpfs_parse_block_bitmap_area(struct ssdfs_dumpfs_environment *env,
					 struct ssdfs_metadata_descriptor *desc)
{
	void *area_buf = NULL;
	u32 area_offset;
	u32 area_size;
	u64 offset;
	int err = 0;

	area_offset = le32_to_cpu(desc->offset);
	area_size = le32_to_cpu(desc->size);

	if (is_ssdfs_dumpfs_area_valid(desc)) {
		area_buf = malloc(area_size);
		if (!area_buf) {
			err = -ENOMEM;
			SSDFS_ERR("fail to allocate memory: "
				  "size %u\n", area_size);
			goto finish_parse_block_bitmap_area;
		}

		memset(area_buf, 0, area_size);

		err = ssdfs_dumpfs_read_block_bitmap(env,
						     env->peb.id,
						     env->peb.peb_size,
						     env->peb.log_offset,
						     env->peb.log_size,
						     area_offset,
						     area_size,
						     area_buf);
		if (err) {
			SSDFS_ERR("fail to read block bitmap: "
				  "peb_id %llu, peb_size %u, "
				  "log_index %u, err %d\n",
				  env->peb.id, env->peb.peb_size,
				  env->peb.log_index, err);
			goto finish_parse_block_bitmap;
		}

		err = ssdfs_dumpfs_parse_block_bitmap(env, area_buf, area_size);
		if (err) {
			SSDFS_ERR("fail to parse block bitmap: "
				  "peb_id %llu, log_index %u, err %d\n",
				  env->peb.id, env->peb.log_index, err);
			goto finish_parse_block_bitmap;
		}

finish_parse_block_bitmap:
		free(area_buf);
		area_buf = NULL;

		if (err)
			goto finish_parse_block_bitmap_area;

		SSDFS_DUMPFS_DUMP(env, "\n");

		if (env->is_raw_dump_requested) {
			offset = env->peb.id * env->peb.peb_size;
			env->raw_dump.offset = offset + area_offset;
			env->raw_dump.size = area_size;

			err = ssdfs_dumpfs_show_raw_dump(env);
			if (err) {
				SSDFS_ERR("fail to make block bitmap "
					  "raw dump: "
					  "peb_id %llu, err %d\n",
					  env->peb.id, err);
				goto finish_parse_block_bitmap_area;
			}

			SSDFS_DUMPFS_DUMP(env, "\n");
		}
	}

finish_parse_block_bitmap_area:
	return err;
}

static
int ssdfs_dumpfs_parse_blk2off_table_fragment(struct ssdfs_dumpfs_environment *env,
					      void *area_buf, u32 area_size,
					      u32 *parsed_bytes)
{
	struct ssdfs_phys_offset_table_header *pot_table = NULL;
	size_t pot_desc_size = sizeof(struct ssdfs_phys_offset_table_header);
	struct ssdfs_phys_offset_descriptor *off_desc = NULL;
	size_t off_desc_size = sizeof(struct ssdfs_phys_offset_descriptor);
	u32 byte_size;
	u16 start_id;
	u16 id_count;
	u16 flags;
	u32 pot_magic;
	u8 *magic_common;
	int i;

	if (area_size < pot_desc_size) {
		SSDFS_ERR("area_size %u, pot_desc_size %zu\n",
			  area_size, pot_desc_size);
		return -EINVAL;
	}

	pot_table = (struct ssdfs_phys_offset_table_header *)area_buf;
	SSDFS_DUMPFS_DUMP(env, "PHYSICAL OFFSETS TABLE HEADER:\n");
	start_id = le16_to_cpu(pot_table->start_id);
	SSDFS_DUMPFS_DUMP(env, "START_ID: %u\n", start_id);
	id_count = le16_to_cpu(pot_table->id_count);
	SSDFS_DUMPFS_DUMP(env, "ID_COUNT: %u\n", id_count);
	byte_size = le32_to_cpu(pot_table->byte_size);
	SSDFS_DUMPFS_DUMP(env, "BYTE_SIZE: %u bytes\n", byte_size);
	SSDFS_DUMPFS_DUMP(env, "PEB INDEX: %u\n",
				le16_to_cpu(pot_table->peb_index));
	SSDFS_DUMPFS_DUMP(env, "SEQUENCE_ID: %u\n",
				le16_to_cpu(pot_table->sequence_id));

	switch (le16_to_cpu(pot_table->type)) {
	case SSDFS_SEG_OFF_TABLE:
		SSDFS_DUMPFS_DUMP(env,
			"OFFSET TABLE TYPE: SSDFS_SEG_OFF_TABLE\n");
		break;

	default:
		SSDFS_DUMPFS_DUMP(env,
			"OFFSET TABLE TYPE: UNKNOWN\n");
		break;
	}

	flags = le16_to_cpu(pot_table->flags);

	SSDFS_DUMPFS_DUMP(env, "OFFSET TABLE FLAGS: ");

	if (flags & SSDFS_OFF_TABLE_HAS_CSUM)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_OFF_TABLE_HAS_CSUM ");

	if (flags & SSDFS_OFF_TABLE_HAS_NEXT_FRAGMENT)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_OFF_TABLE_HAS_NEXT_FRAGMENT ");

	if (flags == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");

	SSDFS_DUMPFS_DUMP(env, "\n");

	pot_magic = le32_to_cpu(pot_table->magic);
	magic_common = (u8 *)&pot_magic;

	SSDFS_DUMPFS_DUMP(env, "OFFSET TABLE MAGIC: %c%c%c%c\n",
		   *magic_common, *(magic_common + 1),
		   *(magic_common + 2), *(magic_common + 3));

	SSDFS_DUMPFS_DUMP(env, "CHECKSUM: %#x\n",
			le32_to_cpu(pot_table->checksum));
	SSDFS_DUMPFS_DUMP(env, "USED LOGICAL BLOCKS: %u\n",
			le16_to_cpu(pot_table->used_logical_blks));
	SSDFS_DUMPFS_DUMP(env, "FREE LOGICAL BLOCKS: %u\n",
			le16_to_cpu(pot_table->free_logical_blks));
	SSDFS_DUMPFS_DUMP(env, "LAST ALLOCATED BLOCK: %u\n",
			le16_to_cpu(pot_table->last_allocated_blk));
	SSDFS_DUMPFS_DUMP(env, "NEXT FRAGMENT OFFSET: %u bytes\n",
			le16_to_cpu(pot_table->next_fragment_off));

	SSDFS_DUMPFS_DUMP(env, "\n");

	if (area_size < byte_size) {
		SSDFS_ERR("area_size %u, byte_size %u\n",
			  area_size, byte_size);
		return -EINVAL;
	}

	if (area_size < (off_desc_size * id_count)) {
		SSDFS_ERR("area_size %u, id_count %u, off_desc_size %zu\n",
			  area_size, id_count, off_desc_size);
		return -ERANGE;
	}

	for (i = 0; i < id_count; i++) {
		off_desc =
			(struct ssdfs_phys_offset_descriptor *)((u8*)area_buf +
							pot_desc_size +
							(off_desc_size * i));

		SSDFS_DUMPFS_DUMP(env, "OFFSET ID: %u\n", start_id + i);
		SSDFS_DUMPFS_DUMP(env, "LOGICAL OFFSET: %u page(s)\n",
			    le32_to_cpu(off_desc->page_desc.logical_offset));
		SSDFS_DUMPFS_DUMP(env, "LOGICAL BLOCK: %u\n",
			    le16_to_cpu(off_desc->page_desc.logical_blk));
		SSDFS_DUMPFS_DUMP(env, "PEB_PAGE: %u\n",
			    le16_to_cpu(off_desc->page_desc.peb_page));

		SSDFS_DUMPFS_DUMP(env, "LOG_START_PAGE: %u\n",
			    le16_to_cpu(off_desc->blk_state.log_start_page));

		switch (off_desc->blk_state.log_area) {
		case SSDFS_LOG_BLK_DESC_AREA:
			SSDFS_DUMPFS_DUMP(env,
				"LOG AREA TYPE: SSDFS_LOG_BLK_DESC_AREA\n");
			break;

		case SSDFS_LOG_MAIN_AREA:
			SSDFS_DUMPFS_DUMP(env,
				"LOG AREA TYPE: SSDFS_LOG_MAIN_AREA\n");
			break;

		case SSDFS_LOG_DIFFS_AREA:
			SSDFS_DUMPFS_DUMP(env,
				"LOG AREA TYPE: SSDFS_LOG_DIFFS_AREA\n");
			break;

		case SSDFS_LOG_JOURNAL_AREA:
			SSDFS_DUMPFS_DUMP(env,
				"LOG AREA TYPE: SSDFS_LOG_JOURNAL_AREA\n");
			break;

		default:
			SSDFS_DUMPFS_DUMP(env, "LOG AREA TYPE: UNKNOWN\n");
			break;
		}

		SSDFS_DUMPFS_DUMP(env, "PEB_MIGRATION_ID: %u\n",
			    off_desc->blk_state.peb_migration_id);
		SSDFS_DUMPFS_DUMP(env, "BYTE_OFFSET: %u\n",
			    le32_to_cpu(off_desc->blk_state.byte_offset));

		SSDFS_DUMPFS_DUMP(env, "\n");
	}

	SSDFS_DUMPFS_DUMP(env, "\n");

	*parsed_bytes += byte_size;

	if (flags & SSDFS_OFF_TABLE_HAS_NEXT_FRAGMENT)
		return -EAGAIN;

	return 0;
}

static
int ssdfs_dumpfs_parse_blk2off_table(struct ssdfs_dumpfs_environment *env,
				     void *area_buf, u32 area_size)
{
	struct ssdfs_blk2off_table_header *hdr =
			(struct ssdfs_blk2off_table_header *)area_buf;
	size_t hdr_size = sizeof(struct ssdfs_blk2off_table_header);
	struct ssdfs_translation_extent *extents = NULL;
	size_t extent_desc_size = sizeof(struct ssdfs_translation_extent);
	u8 *fragment = NULL;
	u16 flags;
	u16 extents_count;
	u16 offset_table_off;
	size_t size;
	u32 parsed_bytes = 0;
	int i;
	int err;

	if (area_size < hdr_size) {
		SSDFS_ERR("area_size %u < hdr_size %zu\n",
			  area_size, hdr_size);
		return -EINVAL;
	}

	SSDFS_DUMPFS_DUMP(env, "BLK2OFF TABLE:\n");

	ssdfs_dumpfs_parse_magic(env, &hdr->magic);

	SSDFS_DUMPFS_DUMP(env, "METADATA CHECK:\n");
	SSDFS_DUMPFS_DUMP(env, "BYTES: %u\n",
			le16_to_cpu(hdr->check.bytes));

	flags = le16_to_cpu(hdr->check.flags);

	SSDFS_DUMPFS_DUMP(env, "METADATA CHECK FLAGS: ");

	if (flags & SSDFS_CRC32)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_CRC32 ");

	if (flags & SSDFS_BLK2OFF_TBL_ZLIB_COMPR)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_BLK2OFF_TBL_ZLIB_COMPR ");

	if (flags & SSDFS_BLK2OFF_TBL_LZO_COMPR)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_BLK2OFF_TBL_LZO_COMPR ");

	if (flags == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "CHECKSUM: %#x\n",
				le32_to_cpu(hdr->check.csum));

	SSDFS_DUMPFS_DUMP(env, "EXTENTS OFFSET: %u bytes\n",
			le16_to_cpu(hdr->extents_off));
	extents_count = le16_to_cpu(hdr->extents_count);
	SSDFS_DUMPFS_DUMP(env, "EXTENTS COUNT: %u\n", extents_count);
	offset_table_off = le16_to_cpu(hdr->offset_table_off);
	SSDFS_DUMPFS_DUMP(env, "OFFSETS TABLE OFFSET: %u bytes\n",
						offset_table_off);
	SSDFS_DUMPFS_DUMP(env, "FRAGMENTS COUNT: %u\n",
			le16_to_cpu(hdr->fragments_count));

	if (extents_count > 1) {
		size = hdr_size + ((extents_count - 1) * extent_desc_size);

		if (area_size < size) {
			SSDFS_ERR("area_size %u < size %zu\n",
				  area_size, size);
			return -EINVAL;
		}
	}

	SSDFS_DUMPFS_DUMP(env, "\n");

	extents = &hdr->sequence[0];

	for (i = 0; i < extents_count; i++) {
		SSDFS_DUMPFS_DUMP(env, "EXTENT#%d:\n", i);
		SSDFS_DUMPFS_DUMP(env, "LOGICAL BLOCK: %u\n",
			    le16_to_cpu(extents[i].logical_blk));
		SSDFS_DUMPFS_DUMP(env, "OFFSET_ID: %u\n",
			    le16_to_cpu(extents[i].offset_id));
		SSDFS_DUMPFS_DUMP(env, "LENGTH: %u\n",
			    le16_to_cpu(extents[i].len));
		SSDFS_DUMPFS_DUMP(env, "SEQUENCE_ID: %u\n",
			    extents[i].sequence_id);

		switch (extents[i].state) {
		case SSDFS_LOGICAL_BLK_FREE:
			SSDFS_DUMPFS_DUMP(env,
				"EXTENT STATE: SSDFS_LOGICAL_BLK_FREE\n");
			break;

		case SSDFS_LOGICAL_BLK_USED:
			SSDFS_DUMPFS_DUMP(env,
				"EXTENT STATE: SSDFS_LOGICAL_BLK_USED\n");
			break;

		default:
			SSDFS_DUMPFS_DUMP(env, "EXTENT STATE: UNKNOWN\n");
			break;
		}

		SSDFS_DUMPFS_DUMP(env, "\n");
	}

	parsed_bytes += offset_table_off;

try_next_fragment:
	if (area_size <= parsed_bytes) {
		SSDFS_ERR("area_size %u, parsed_bytes %u\n",
			  area_size, parsed_bytes);
		return -ERANGE;
	}

	fragment = (u8 *)area_buf + parsed_bytes;
	err = ssdfs_dumpfs_parse_blk2off_table_fragment(env, fragment,
						area_size - parsed_bytes,
						&parsed_bytes);
	if (err == -EAGAIN)
		goto try_next_fragment;
	else if (err) {
		SSDFS_ERR("fail to parse fragment: err %d\n", err);
		return err;
	}

	return 0;
}

static
int ssdfs_dumpfs_parse_blk2off_area(struct ssdfs_dumpfs_environment *env,
				    struct ssdfs_metadata_descriptor *desc)
{
	void *area_buf = NULL;
	u32 area_offset;
	u32 area_size;
	u64 offset;
	int err = 0;

	area_offset = le32_to_cpu(desc->offset);
	area_size = le32_to_cpu(desc->size);

	if (is_ssdfs_dumpfs_area_valid(desc)) {
		area_buf = malloc(area_size);
		if (!area_buf) {
			err = -ENOMEM;
			SSDFS_ERR("fail to allocate memory: "
				  "size %u\n", area_size);
			goto finish_parse_blk2off_area;
		}

		memset(area_buf, 0, area_size);

		err = ssdfs_dumpfs_read_blk2off_table(env,
						     env->peb.id,
						     env->peb.peb_size,
						     env->peb.log_offset,
						     env->peb.log_size,
						     area_offset,
						     area_size,
						     area_buf);
		if (err) {
			SSDFS_ERR("fail to read blk2off table: "
				  "peb_id %llu, peb_size %u, "
				  "log_index %u, err %d\n",
				  env->peb.id, env->peb.peb_size,
				  env->peb.log_index, err);
			goto finish_parse_blk2off_table;
		}

		err = ssdfs_dumpfs_parse_blk2off_table(env, area_buf,
							area_size);
		if (err) {
			SSDFS_ERR("fail to parse blk2off table: "
				  "peb_id %llu, log_index %u, err %d\n",
				  env->peb.id, env->peb.log_index, err);
			goto finish_parse_blk2off_table;
		}

finish_parse_blk2off_table:
		free(area_buf);
		area_buf = NULL;

		if (err)
			goto finish_parse_blk2off_area;

		SSDFS_DUMPFS_DUMP(env, "\n");

		if (env->is_raw_dump_requested) {
			offset = env->peb.id * env->peb.peb_size;
			env->raw_dump.offset = offset + area_offset;
			env->raw_dump.size = area_size;

			err = ssdfs_dumpfs_show_raw_dump(env);
			if (err) {
				SSDFS_ERR("fail to make blk2off table "
					  "raw dump: "
					  "peb_id %llu, err %d\n",
					  env->peb.id, err);
				goto finish_parse_blk2off_area;
			}

			SSDFS_DUMPFS_DUMP(env, "\n");
		}
	}

finish_parse_blk2off_area:
	return err;
}

static
int __ssdfs_dumpfs_parse_log_footer(struct ssdfs_dumpfs_environment *env,
				    u32 area_offset,
				    void *area_buf, u32 area_size)
{
	struct ssdfs_log_footer *log_footer = NULL;
	size_t lf_size = sizeof(struct ssdfs_log_footer);
	struct ssdfs_volume_state *vs = NULL;
	struct ssdfs_metadata_descriptor *desc;
	u32 flags;
	u64 feature_compat;
	u64 feature_compat_ro;
	u64 feature_incompat;
	char label[SSDFS_VOLUME_LABEL_MAX + 1];
	u64 offset;
	int err = 0;

	if (area_size < lf_size) {
		SSDFS_ERR("area_size %u < log footer size %zu\n",
			  area_size, lf_size);
		return -EINVAL;
	}

	log_footer = (struct ssdfs_log_footer *)area_buf;
	vs = &log_footer->volume_state;

	SSDFS_DUMPFS_DUMP(env, "LOG FOOTER:\n");

	ssdfs_dumpfs_parse_magic(env, &log_footer->volume_state.magic);

	SSDFS_DUMPFS_DUMP(env, "METADATA CHECK:\n");
	SSDFS_DUMPFS_DUMP(env, "BYTES: %u\n", le16_to_cpu(vs->check.bytes));

	flags = le16_to_cpu(vs->check.flags);

	SSDFS_DUMPFS_DUMP(env, "METADATA CHECK FLAGS: ");

	if (flags & SSDFS_CRC32)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_CRC32 ");

	if (flags == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "CHECKSUM: %#x\n", le32_to_cpu(vs->check.csum));

	SSDFS_DUMPFS_DUMP(env, "SEGMENT NUMBERS: %llu\n",
						le64_to_cpu(vs->nsegs));
	SSDFS_DUMPFS_DUMP(env, "FREE PAGES: %llu\n",
						le64_to_cpu(vs->free_pages));
	SSDFS_DUMPFS_DUMP(env, "LOG_CREATION_TIME: %s\n",
		   ssdfs_nanoseconds_to_time(le64_to_cpu(vs->timestamp)));
	SSDFS_DUMPFS_DUMP(env, "CHECKPOINT: %llu\n", le64_to_cpu(vs->cno));

	flags = le32_to_cpu(vs->flags);

	SSDFS_DUMPFS_DUMP(env, "VOLUME STATE FLAGS: ");

	if (flags & SSDFS_HAS_INLINE_INODES_TREE)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_HAS_INLINE_INODES_TREE ");

	if (flags == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");

	SSDFS_DUMPFS_DUMP(env, "\n");

	switch (le16_to_cpu(vs->state)) {
	case SSDFS_MOUNTED_FS:
		SSDFS_DUMPFS_DUMP(env, "FS STATE: SSDFS_MOUNTED_FS\n");
		break;

	case SSDFS_VALID_FS:
		SSDFS_DUMPFS_DUMP(env, "FS STATE: SSDFS_VALID_FS\n");
		break;

	case SSDFS_ERROR_FS:
		SSDFS_DUMPFS_DUMP(env, "FS STATE: SSDFS_ERROR_FS\n");
		break;

	case SSDFS_RESIZE_FS:
		SSDFS_DUMPFS_DUMP(env, "FS STATE: SSDFS_RESIZE_FS\n");
		break;

	default:
		SSDFS_DUMPFS_DUMP(env, "FS STATE: UNKNOWN\n");
		break;
	}

	switch (le16_to_cpu(vs->errors)) {
	case SSDFS_ERRORS_CONTINUE:
		SSDFS_DUMPFS_DUMP(env, "BEHAVIOR: SSDFS_ERRORS_CONTINUE\n");
		break;

	case SSDFS_ERRORS_RO:
		SSDFS_DUMPFS_DUMP(env, "BEHAVIOR: SSDFS_ERRORS_RO\n");
		break;

	case SSDFS_ERRORS_PANIC:
		SSDFS_DUMPFS_DUMP(env, "BEHAVIOR: SSDFS_ERRORS_PANIC\n");
		break;

	default:
		SSDFS_DUMPFS_DUMP(env, "BEHAVIOR: UNKNOWN\n");
		break;
	}

	feature_compat = le64_to_cpu(vs->feature_compat);

	SSDFS_DUMPFS_DUMP(env, "FEATURE_COMPATIBLE FLAGS: ");

	if (feature_compat & SSDFS_HAS_SEGBMAP_COMPAT_FLAG)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_HAS_SEGBMAP_COMPAT_FLAG ");

	if (feature_compat & SSDFS_HAS_MAPTBL_COMPAT_FLAG)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_HAS_MAPTBL_COMPAT_FLAG ");

	if (feature_compat & SSDFS_HAS_SHARED_EXTENTS_COMPAT_FLAG)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_HAS_SHARED_EXTENTS_COMPAT_FLAG ");

	if (feature_compat & SSDFS_HAS_SHARED_XATTRS_COMPAT_FLAG)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_HAS_SHARED_XATTRS_COMPAT_FLAG ");

	if (feature_compat & SSDFS_HAS_SHARED_DICT_COMPAT_FLAG)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_HAS_SHARED_DICT_COMPAT_FLAG ");

	if (feature_compat & SSDFS_HAS_INODES_TREE_COMPAT_FLAG)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_HAS_INODES_TREE_COMPAT_FLAG ");

	if (feature_compat == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");

	SSDFS_DUMPFS_DUMP(env, "\n");

	feature_compat_ro = le64_to_cpu(vs->feature_compat_ro);

	SSDFS_DUMPFS_DUMP(env, "FEATURE_COMPATIBLE_RO FLAGS: ");

	if (feature_compat_ro & SSDFS_ZLIB_COMPAT_RO_FLAG)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_ZLIB_COMPAT_RO_FLAG ");

	if (feature_compat_ro & SSDFS_LZO_COMPAT_RO_FLAG)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_LZO_COMPAT_RO_FLAG ");

	if (feature_compat_ro == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");

	SSDFS_DUMPFS_DUMP(env, "\n");

	feature_incompat = le64_to_cpu(vs->feature_incompat);

	SSDFS_DUMPFS_DUMP(env, "FEATURE_INCOMPATIBLE FLAGS: ");

	if (feature_incompat == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");
	else
		SSDFS_DUMPFS_DUMP(env, "%llu", feature_incompat);

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "UUID: %s\n", uuid_string(vs->uuid));

	memset(label, 0, SSDFS_VOLUME_LABEL_MAX + 1);
	memcpy(label, vs->label, SSDFS_VOLUME_LABEL_MAX);
	SSDFS_DUMPFS_DUMP(env, "LABEL: %s\n", label);

	SSDFS_DUMPFS_DUMP(env, "CUR_DATA_SEG: %llu\n",
		   le64_to_cpu(vs->cur_segs[SSDFS_CUR_DATA_SEG]));
	SSDFS_DUMPFS_DUMP(env, "CUR_LNODE_SEG: %llu\n",
		   le64_to_cpu(vs->cur_segs[SSDFS_CUR_LNODE_SEG]));
	SSDFS_DUMPFS_DUMP(env, "CUR_HNODE_SEG: %llu\n",
		   le64_to_cpu(vs->cur_segs[SSDFS_CUR_HNODE_SEG]));
	SSDFS_DUMPFS_DUMP(env, "CUR_CUR_IDXNODE_SEG: %llu\n",
		   le64_to_cpu(vs->cur_segs[SSDFS_CUR_IDXNODE_SEG]));

	SSDFS_DUMPFS_DUMP(env, "MIGRATION THRESHOLD: %u\n",
		   le16_to_cpu(vs->migration_threshold));

	SSDFS_DUMPFS_DUMP(env, "BLOCK BITMAP OPTIONS:\n");

	flags = le16_to_cpu(vs->blkbmap.flags);

	SSDFS_DUMPFS_DUMP(env, "FLAGS: ");

	if (flags & SSDFS_BLK_BMAP_CREATE_COPY)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_BLK_BMAP_CREATE_COPY ");

	if (flags & SSDFS_BLK_BMAP_MAKE_COMPRESSION)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_BLK_BMAP_MAKE_COMPRESSION ");

	if (flags == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");

	SSDFS_DUMPFS_DUMP(env, "\n");

	switch (vs->blkbmap.compression) {
	case SSDFS_BLK_BMAP_NOCOMPR_TYPE:
		SSDFS_DUMPFS_DUMP(env,
			"COMPRESSION: SSDFS_BLK_BMAP_NOCOMPR_TYPE\n");
		break;

	case SSDFS_BLK_BMAP_ZLIB_COMPR_TYPE:
		SSDFS_DUMPFS_DUMP(env,
			"COMPRESSION: SSDFS_BLK_BMAP_ZLIB_COMPR_TYPE\n");
		break;

	case SSDFS_BLK_BMAP_LZO_COMPR_TYPE:
		SSDFS_DUMPFS_DUMP(env,
			"COMPRESSION: SSDFS_BLK_BMAP_LZO_COMPR_TYPE\n");
		break;

	default:
		SSDFS_DUMPFS_DUMP(env, "COMPRESSION: UNKNOWN\n");
		break;
	}

	SSDFS_DUMPFS_DUMP(env, "BLK2OFF TABLE OPTIONS:\n");

	flags = le16_to_cpu(vs->blk2off_tbl.flags);

	SSDFS_DUMPFS_DUMP(env, "FLAGS: ");

	if (flags & SSDFS_BLK2OFF_TBL_CREATE_COPY)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_BLK2OFF_TBL_CREATE_COPY ");

	if (flags & SSDFS_BLK2OFF_TBL_MAKE_COMPRESSION)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_BLK2OFF_TBL_MAKE_COMPRESSION ");

	if (flags == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");

	SSDFS_DUMPFS_DUMP(env, "\n");

	switch (vs->blk2off_tbl.compression) {
	case SSDFS_BLK2OFF_TBL_NOCOMPR_TYPE:
		SSDFS_DUMPFS_DUMP(env,
			"COMPRESSION: SSDFS_BLK2OFF_TBL_NOCOMPR_TYPE\n");
		break;

	case SSDFS_BLK2OFF_TBL_ZLIB_COMPR_TYPE:
		SSDFS_DUMPFS_DUMP(env,
			"COMPRESSION: SSDFS_BLK2OFF_TBL_ZLIB_COMPR_TYPE\n");
		break;

	case SSDFS_BLK2OFF_TBL_LZO_COMPR_TYPE:
		SSDFS_DUMPFS_DUMP(env,
			"COMPRESSION: SSDFS_BLK2OFF_TBL_LZO_COMPR_TYPE\n");
		break;

	default:
		SSDFS_DUMPFS_DUMP(env, "COMPRESSION: UNKNOWN\n");
		break;
	}

	SSDFS_DUMPFS_DUMP(env, "USER DATA OPTIONS:\n");

	flags = le16_to_cpu(vs->user_data.flags);

	SSDFS_DUMPFS_DUMP(env, "FLAGS: ");

	if (flags & SSDFS_USER_DATA_MAKE_COMPRESSION)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_USER_DATA_MAKE_COMPRESSION ");

	if (flags == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");

	SSDFS_DUMPFS_DUMP(env, "\n");

	switch (vs->user_data.compression) {
	case SSDFS_USER_DATA_NOCOMPR_TYPE:
		SSDFS_DUMPFS_DUMP(env,
			"COMPRESSION: SSDFS_USER_DATA_NOCOMPR_TYPE\n");
		break;

	case SSDFS_USER_DATA_ZLIB_COMPR_TYPE:
		SSDFS_DUMPFS_DUMP(env,
			"COMPRESSION: SSDFS_USER_DATA_ZLIB_COMPR_TYPE\n");
		break;

	case SSDFS_USER_DATA_LZO_COMPR_TYPE:
		SSDFS_DUMPFS_DUMP(env,
			"COMPRESSION: SSDFS_USER_DATA_LZO_COMPR_TYPE\n");
		break;

	default:
		SSDFS_DUMPFS_DUMP(env, "COMPRESSION: UNKNOWN\n");
		break;
	}

	SSDFS_DUMPFS_DUMP(env, "MIGRATION THRESHOLD: %u\n",
		   le16_to_cpu(vs->user_data.migration_threshold));

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "ROOT FOLDER:\n");
	ssdfs_dumpfs_parse_raw_inode(env, &vs->root_folder);

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "INODES B-TREE HEADER:\n");
	ssdfs_dumpfs_parse_btree_descriptor(env, &vs->inodes_btree.desc);
	SSDFS_DUMPFS_DUMP(env, "ALLOCATED INODES: %llu\n",
		   le64_to_cpu(vs->inodes_btree.allocated_inodes));
	SSDFS_DUMPFS_DUMP(env, "FREE INODES: %llu\n",
		   le64_to_cpu(vs->inodes_btree.free_inodes));
	SSDFS_DUMPFS_DUMP(env, "INODES CAPACITY: %llu\n",
		   le64_to_cpu(vs->inodes_btree.inodes_capacity));
	SSDFS_DUMPFS_DUMP(env, "LEAF NODES: %u\n",
		   le32_to_cpu(vs->inodes_btree.leaf_nodes));
	SSDFS_DUMPFS_DUMP(env, "NODES COUNT: %u\n",
		   le32_to_cpu(vs->inodes_btree.nodes_count));
	SSDFS_DUMPFS_DUMP(env, "UPPER_ALLOCATED_INO: %llu\n",
		   le64_to_cpu(vs->inodes_btree.upper_allocated_ino));
	ssdfs_dumpfs_parse_inline_root_node(env, &vs->inodes_btree.root_node);

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "SHARED EXTENTS B-TREE HEADER:\n");
	ssdfs_dumpfs_parse_btree_descriptor(env,
				&vs->shared_extents_btree.desc);
	ssdfs_dumpfs_parse_inline_root_node(env,
				&vs->shared_extents_btree.root_node);

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "SHARED DICTIONARY B-TREE HEADER:\n");
	ssdfs_dumpfs_parse_btree_descriptor(env,
				&vs->shared_dict_btree.desc);
	ssdfs_dumpfs_parse_inline_root_node(env,
				&vs->shared_dict_btree.root_node);

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "TIMESTAMP: %s\n",
		ssdfs_nanoseconds_to_time(le64_to_cpu(log_footer->timestamp)));
	SSDFS_DUMPFS_DUMP(env, "CHECKPOINT: %llu\n",
				le64_to_cpu(log_footer->cno));
	SSDFS_DUMPFS_DUMP(env, "LOG BYTES: %u bytes\n",
				le32_to_cpu(log_footer->log_bytes));

	flags = le32_to_cpu(log_footer->log_flags);

	SSDFS_DUMPFS_DUMP(env, "LOG FOOTER FLAGS: ");

	if (flags & SSDFS_LOG_FOOTER_HAS_BLK_BMAP)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_LOG_FOOTER_HAS_BLK_BMAP ");

	if (flags & SSDFS_LOG_FOOTER_HAS_OFFSET_TABLE)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_LOG_FOOTER_HAS_OFFSET_TABLE ");

	if (flags & SSDFS_PARTIAL_LOG_FOOTER)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_PARTIAL_LOG_FOOTER ");

	if (flags & SSDFS_ENDING_LOG_FOOTER)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_ENDING_LOG_FOOTER ");

	if (flags == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");

	SSDFS_DUMPFS_DUMP(env, "\n");

	desc = &log_footer->desc_array[SSDFS_BLK_BMAP_INDEX];
	SSDFS_DUMPFS_DUMP(env, "BLOCK_BITMAP: offset %u, size %u\n",
			  le32_to_cpu(desc->offset),
			  le32_to_cpu(desc->size));

	desc = &log_footer->desc_array[SSDFS_OFF_TABLE_INDEX];
	SSDFS_DUMPFS_DUMP(env, "OFFSETS_TABLE: offset %u, size %u\n",
			  le32_to_cpu(desc->offset),
			  le32_to_cpu(desc->size));

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "\n");

	if (env->is_raw_dump_requested) {
		offset = env->peb.id * env->peb.peb_size;
		env->raw_dump.offset = offset + area_offset;
		env->raw_dump.size = area_size;

		err = ssdfs_dumpfs_show_raw_dump(env);
		if (err) {
			SSDFS_ERR("fail to make log footer "
				  "raw dump: "
				  "peb_id %llu, err %d\n",
				  env->peb.id, err);
			return err;
		}

		SSDFS_DUMPFS_DUMP(env, "\n");
	}

	err = ssdfs_dumpfs_parse_block_bitmap_area(env,
			&log_footer->desc_array[SSDFS_BLK_BMAP_INDEX]);
	if (err) {
		SSDFS_ERR("fail to parse block bitmap: err %d\n", err);
		return err;
	}

	err = ssdfs_dumpfs_parse_blk2off_area(env,
			&log_footer->desc_array[SSDFS_OFF_TABLE_INDEX]);
	if (err) {
		SSDFS_ERR("fail to parse blk2 off table: err %d\n", err);
		return err;
	}

	return 0;
}

static
void ssdfs_dumpfs_parse_blk_state_offset(struct ssdfs_dumpfs_environment *env,
					 struct ssdfs_blk_state_offset *ptr)
{
	SSDFS_DUMPFS_DUMP(env, "LOG_START_PAGE: %u\n",
			le16_to_cpu(ptr->log_start_page));

	switch (ptr->log_area) {
	case SSDFS_LOG_BLK_DESC_AREA:
		SSDFS_DUMPFS_DUMP(env,
			"LOG AREA TYPE: SSDFS_LOG_BLK_DESC_AREA\n");
		break;

	case SSDFS_LOG_MAIN_AREA:
		SSDFS_DUMPFS_DUMP(env,
			"LOG AREA TYPE: SSDFS_LOG_MAIN_AREA\n");
		break;

	case SSDFS_LOG_DIFFS_AREA:
		SSDFS_DUMPFS_DUMP(env,
			"LOG AREA TYPE: SSDFS_LOG_DIFFS_AREA\n");
		break;

	case SSDFS_LOG_JOURNAL_AREA:
		SSDFS_DUMPFS_DUMP(env,
			"LOG AREA TYPE: SSDFS_LOG_JOURNAL_AREA\n");
		break;

	default:
		SSDFS_DUMPFS_DUMP(env,
			"LOG AREA TYPE: UNKNOWN\n");
		break;
	}

	SSDFS_DUMPFS_DUMP(env, "PEB_MIGRATION_ID: %u\n",
		   ptr->peb_migration_id);
	SSDFS_DUMPFS_DUMP(env, "BYTE_OFFSET: %u bytes\n",
		   le32_to_cpu(ptr->byte_offset));

	SSDFS_DUMPFS_DUMP(env, "\n");
}

static
int ssdfs_dumpfs_parse_blk_states(struct ssdfs_dumpfs_environment *env,
				  void *data, u16 compr_size,
				  u16 uncompr_size)
{
	size_t blk_desc_size = sizeof(struct ssdfs_block_descriptor);
	struct ssdfs_block_descriptor *blk_desc;
	u16 count = uncompr_size / blk_desc_size;
	int i, j;

	for (i = 0; i < count; i++) {
		blk_desc = (struct ssdfs_block_descriptor *)((u8 *)data +
							(i * blk_desc_size));

		SSDFS_DUMPFS_DUMP(env, "BLOCK DESCRIPTOR: #%d\n", i);
		SSDFS_DUMPFS_DUMP(env, "INO: %llu\n",
					le64_to_cpu(blk_desc->ino));
		SSDFS_DUMPFS_DUMP(env, "LOGICAL OFFSET: %u page(s)\n",
				le32_to_cpu(blk_desc->logical_offset));
		SSDFS_DUMPFS_DUMP(env, "PEB_INDEX: %u\n",
				le16_to_cpu(blk_desc->peb_index));
		SSDFS_DUMPFS_DUMP(env, "PEB_PAGE: %u\n",
				le16_to_cpu(blk_desc->peb_page));

		SSDFS_DUMPFS_DUMP(env, "\n");

		for (j = 0; j < SSDFS_BLK_STATE_OFF_MAX; j++) {
			SSDFS_DUMPFS_DUMP(env, "BLOCK STATE: #%d\n", j);
			ssdfs_dumpfs_parse_blk_state_offset(env,
						    &blk_desc->state[j]);
		}

		SSDFS_DUMPFS_DUMP(env, "PREV DESCRIPTOR:\n");
		ssdfs_dumpfs_parse_blk_state_offset(env,
						&blk_desc->prev_desc);
	}

	return 0;
}

static
int ssdfs_dumpfs_parse_blk_desc_array(struct ssdfs_dumpfs_environment *env,
				      void *area_buf, u32 area_size)
{
	struct ssdfs_area_block_table *area_hdr = NULL;
	size_t area_hdr_size = sizeof(struct ssdfs_area_block_table);
	struct ssdfs_fragment_desc *frag;
	u16 fragments_count;
	u32 parsed_bytes = 0;
	int i;
	int err;

	if (area_size < area_hdr_size) {
		SSDFS_ERR("area_size %u < area_hdr_size %zu\n",
			  area_size, area_hdr_size);
		return -EINVAL;
	}

	SSDFS_DUMPFS_DUMP(env, "BLOCK DESCRIPTORS ARRAY:\n");

parse_next_area:
	area_hdr = (struct ssdfs_area_block_table *)((u8 *)area_buf +
							parsed_bytes);

	ssdfs_dumpfs_parse_fragments_chain_hdr(env, &area_hdr->chain_hdr);

	parsed_bytes += area_hdr_size;

	fragments_count = le16_to_cpu(area_hdr->chain_hdr.fragments_count);

	if (fragments_count > SSDFS_FRAGMENTS_CHAIN_MAX) {
		SSDFS_ERR("fragments_count %u > MAX %u\n",
			  fragments_count,
			  SSDFS_FRAGMENTS_CHAIN_MAX);
		return -ERANGE;
	}

	for (i = 0; i < fragments_count; i++) {
		u8 *data;

		frag = &area_hdr->blk[i];

		SSDFS_DUMPFS_DUMP(env, "\n");
		SSDFS_DUMPFS_DUMP(env, "FRAGMENT_INDEX: #%d\n", i);

		ssdfs_dumpfs_parse_fragment_header(env, frag);

		if ((area_size - parsed_bytes) < le16_to_cpu(frag->compr_size)) {
			SSDFS_ERR("size %u is lesser than %u\n",
				  area_size - parsed_bytes,
				  le16_to_cpu(frag->compr_size));
			return -EINVAL;
		}

		data = (u8 *)area_buf + parsed_bytes;

		SSDFS_DUMPFS_DUMP(env, "\n");

		err = ssdfs_dumpfs_parse_blk_states(env, data,
					le16_to_cpu(frag->compr_size),
					le16_to_cpu(frag->uncompr_size));
		if (err) {
			SSDFS_ERR("fail to parse block descriptors: "
				  "err %d\n", err);
			return err;
		}

		parsed_bytes += le16_to_cpu(frag->compr_size);
	}

	SSDFS_DUMPFS_DUMP(env, "\n");

	if (le16_to_cpu(area_hdr->chain_hdr.flags) & SSDFS_MULTIPLE_HDR_CHAIN) {
		frag = &area_hdr->blk[SSDFS_NEXT_BLK_TABLE_INDEX];

		if (le8_to_cpu(frag->type) != SSDFS_NEXT_TABLE_DESC) {
			SSDFS_ERR("type %#x is invalid\n",
				  le8_to_cpu(frag->type));
			return -ERANGE;
		}

		if (le32_to_cpu(frag->offset) != parsed_bytes) {
			SSDFS_ERR("offset %u != parsed_bytes %u\n",
				  le32_to_cpu(frag->offset),
				  parsed_bytes);
			return -ERANGE;
		}

		goto parse_next_area;
	}

	return 0;
}

static
void ssdfs_dumpfs_parse_maptbl_sb_header(struct ssdfs_dumpfs_environment *env,
					 struct ssdfs_segment_header *hdr)
{
	struct ssdfs_maptbl_sb_header *maptbl = &hdr->volume_hdr.maptbl;
	u32 fragments_count = le32_to_cpu(maptbl->fragments_count);
	u32 fragment_bytes = le32_to_cpu(maptbl->fragment_bytes);
	u64 last_peb_recover_cno = le64_to_cpu(maptbl->last_peb_recover_cno);
	u64 lebs_count = le64_to_cpu(maptbl->lebs_count);
	u64 pebs_count = le64_to_cpu(maptbl->pebs_count);
	u16 fragments_per_seg = le16_to_cpu(maptbl->fragments_per_seg);
	u16 fragments_per_peb = le16_to_cpu(maptbl->fragments_per_peb);
	u16 flags = le16_to_cpu(maptbl->flags);
	u16 pre_erase_pebs = le16_to_cpu(maptbl->pre_erase_pebs);
	u16 lebs_per_fragment = le16_to_cpu(maptbl->lebs_per_fragment);
	u16 pebs_per_fragment = le16_to_cpu(maptbl->pebs_per_fragment);
	u16 pebs_per_stripe = le16_to_cpu(maptbl->pebs_per_stripe);
	u16 stripes_per_fragment = le16_to_cpu(maptbl->stripes_per_fragment);
	int i;

	SSDFS_DUMPFS_DUMP(env, "MAPPING TABLE HEADER:\n");

	SSDFS_DUMPFS_DUMP(env, "FRAGMENTS_COUNT: %u\n", fragments_count);
	SSDFS_DUMPFS_DUMP(env, "FRAGMENT_BYTES: %u\n", fragment_bytes);
	SSDFS_DUMPFS_DUMP(env, "LAST_PEB_RECOVER_CNO: %llu\n",
							last_peb_recover_cno);
	SSDFS_DUMPFS_DUMP(env, "LEBS_COUNT: %llu\n", lebs_count);
	SSDFS_DUMPFS_DUMP(env, "PEBS_COUNT: %llu\n", pebs_count);
	SSDFS_DUMPFS_DUMP(env, "FRAGMENTS_PER_SEGMENT: %u\n",
							fragments_per_seg);
	SSDFS_DUMPFS_DUMP(env, "FRAGMENTS_PER_PEB: %u\n", fragments_per_peb);
	SSDFS_DUMPFS_DUMP(env, "PRE_ERASE_PEBS: %u\n", pre_erase_pebs);
	SSDFS_DUMPFS_DUMP(env, "LEBS_PER_FRAGMENT: %u\n", lebs_per_fragment);
	SSDFS_DUMPFS_DUMP(env, "PEBS_PER_FRAGMENT: %u\n", pebs_per_fragment);
	SSDFS_DUMPFS_DUMP(env, "PEBS_PER_STRIPE: %u\n", pebs_per_stripe);
	SSDFS_DUMPFS_DUMP(env, "STRIPES_PER_FRAGMENT: %u\n",
							stripes_per_fragment);

	SSDFS_DUMPFS_DUMP(env, "MAPPING TABLE FLAGS: ");

	if (flags & SSDFS_MAPTBL_HAS_COPY)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_MAPTBL_HAS_COPY ");

	if (flags & SSDFS_MAPTBL_ERROR)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_MAPTBL_ERROR ");

	if (flags & SSDFS_MAPTBL_MAKE_ZLIB_COMPR)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_MAPTBL_MAKE_ZLIB_COMPR ");

	if (flags & SSDFS_MAPTBL_MAKE_LZO_COMPR)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_MAPTBL_MAKE_LZO_COMPR ");

	if (flags == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "MAPPING TABLE EXTENTS:\n");

	for (i = 0; i < SSDFS_MAPTBL_RESERVED_EXTENTS; i++) {
		struct ssdfs_meta_area_extent *extent;

		extent = &maptbl->extents[i][SSDFS_MAIN_MAPTBL_SEG];

		SSDFS_DUMPFS_DUMP(env,
			   "extent[%d][MAIN]: start_id %llu, len %u, ",
			   i,
			   le64_to_cpu(extent->start_id),
			   le32_to_cpu(extent->len));

		switch (le16_to_cpu(extent->type)) {
		case SSDFS_EMPTY_EXTENT_TYPE:
			SSDFS_DUMPFS_DUMP(env, "SSDFS_EMPTY_EXTENT_TYPE, ");
			break;

		case SSDFS_SEG_EXTENT_TYPE:
			SSDFS_DUMPFS_DUMP(env, "SSDFS_SEG_EXTENT_TYPE, ");
			break;

		case SSDFS_PEB_EXTENT_TYPE:
			SSDFS_DUMPFS_DUMP(env, "SSDFS_PEB_EXTENT_TYPE, ");
			break;

		case SSDFS_BLK_EXTENT_TYPE:
			SSDFS_DUMPFS_DUMP(env, "SSDFS_BLK_EXTENT_TYPE, ");
			break;

		default:
			SSDFS_DUMPFS_DUMP(env, "UNKNOWN_EXTENT_TYPE, ");
			break;
		}

		SSDFS_DUMPFS_DUMP(env, "flags %#x\n",
			   le16_to_cpu(extent->flags));

		extent = &maptbl->extents[i][SSDFS_COPY_MAPTBL_SEG];

		SSDFS_DUMPFS_DUMP(env,
			   "extent[%d][COPY]: start_id %llu, len %u, ",
			   i,
			   le64_to_cpu(extent->start_id),
			   le32_to_cpu(extent->len));

		switch (le16_to_cpu(extent->type)) {
		case SSDFS_EMPTY_EXTENT_TYPE:
			SSDFS_DUMPFS_DUMP(env, "SSDFS_EMPTY_EXTENT_TYPE, ");
			break;

		case SSDFS_SEG_EXTENT_TYPE:
			SSDFS_DUMPFS_DUMP(env, "SSDFS_SEG_EXTENT_TYPE, ");
			break;

		case SSDFS_PEB_EXTENT_TYPE:
			SSDFS_DUMPFS_DUMP(env, "SSDFS_PEB_EXTENT_TYPE, ");
			break;

		case SSDFS_BLK_EXTENT_TYPE:
			SSDFS_DUMPFS_DUMP(env, "SSDFS_BLK_EXTENT_TYPE, ");
			break;

		default:
			SSDFS_DUMPFS_DUMP(env, "UNKNOWN_EXTENT_TYPE, ");
			break;
		}

		SSDFS_DUMPFS_DUMP(env, "flags %#x\n",
			   le16_to_cpu(extent->flags));
	}
}

static
void ssdfs_dumpfs_parse_segbmap_sb_header(struct ssdfs_dumpfs_environment *env,
					  struct ssdfs_segment_header *hdr)
{
	struct ssdfs_segbmap_sb_header *segbmap = &hdr->volume_hdr.segbmap;
	u16 fragments_count = le16_to_cpu(segbmap->fragments_count);
	u16 fragments_per_seg = le16_to_cpu(segbmap->fragments_per_seg);
	u16 fragments_per_peb = le16_to_cpu(segbmap->fragments_per_peb);
	u16 fragment_size = le16_to_cpu(segbmap->fragment_size);
	u32 bytes_count = le32_to_cpu(segbmap->bytes_count);
	u16 flags = le16_to_cpu(segbmap->flags);
	u16 segs_count = le16_to_cpu(segbmap->segs_count);
	int i;

	SSDFS_DBG(env->base.show_debug,
		  "parse segbmap sb header\n");

	SSDFS_DUMPFS_DUMP(env, "SEGMENT BITMAP HEADER:\n");

	SSDFS_DUMPFS_DUMP(env, "FRAGMENTS_COUNT: %u\n", fragments_count);
	SSDFS_DUMPFS_DUMP(env, "FRAGMENTS_PER_SEGMENT: %u\n",
							fragments_per_seg);
	SSDFS_DUMPFS_DUMP(env, "FRAGMENTS_PER_PEB: %u\n", fragments_per_peb);
	SSDFS_DUMPFS_DUMP(env, "FRAGMENTS_SIZE: %u bytes\n", fragment_size);
	SSDFS_DUMPFS_DUMP(env, "BYTES_COUNT: %u bytes\n", bytes_count);
	SSDFS_DUMPFS_DUMP(env, "SEGMENTS_COUNT: %u\n", segs_count);

	SSDFS_DUMPFS_DUMP(env, "SEGMENT BITMAP FLAGS: ");

	if (flags & SSDFS_SEGBMAP_HAS_COPY)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_SEGBMAP_HAS_COPY ");

	if (flags & SSDFS_SEGBMAP_ERROR)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_SEGBMAP_ERROR ");

	if (flags & SSDFS_SEGBMAP_MAKE_ZLIB_COMPR)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_SEGBMAP_MAKE_ZLIB_COMPR ");

	if (flags & SSDFS_SEGBMAP_MAKE_LZO_COMPR)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_SEGBMAP_MAKE_LZO_COMPR ");

	if (flags == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "SEGMENT BITMAP SEGMENTS:\n");

	for (i = 0; i < SSDFS_SEGBMAP_SEGS; i++) {
		SSDFS_DUMPFS_DUMP(env,
			   "SEG[%d][MAIN]: %llu; SEG[%d][COPY]: %llu\n",
			   i,
			   le64_to_cpu(segbmap->segs[i][SSDFS_MAIN_SEGBMAP_SEG]),
			   i,
			   le64_to_cpu(segbmap->segs[i][SSDFS_COPY_SEGBMAP_SEG]));
	}
}

static
void ssdfs_dumpfs_parse_segment_header(struct ssdfs_dumpfs_environment *env,
					struct ssdfs_segment_header *hdr)
{
	struct ssdfs_volume_header *vh = &hdr->volume_hdr;
	u32 page_size;
	u32 erase_size;
	u32 seg_size;
	u64 create_time;
	u64 create_cno;
	struct ssdfs_leb2peb_pair *pair1, *pair2;
	u16 seg_type;
	const char *seg_type_str = NULL;
	u32 seg_flags;
	struct ssdfs_metadata_descriptor *desc;

	SSDFS_DBG(env->base.show_debug,
		  "parse segment header\n");

	page_size = 1 << vh->log_pagesize;
	erase_size = 1 << vh->log_erasesize;
	seg_size = 1 << vh->log_segsize;
	create_time = le64_to_cpu(vh->create_time);
	create_cno = le64_to_cpu(vh->create_cno);
	seg_type = le16_to_cpu(hdr->seg_type);
	seg_flags = le32_to_cpu(hdr->seg_flags);

	ssdfs_dumpfs_parse_magic(env, &vh->magic);

	SSDFS_DUMPFS_DUMP(env, "PAGE: %u bytes\n", page_size);
	SSDFS_DUMPFS_DUMP(env, "PEB: %u bytes\n", erase_size);
	SSDFS_DUMPFS_DUMP(env, "PEBS_PER_SEGMENT: %u\n",
			  1 << vh->log_pebs_per_seg);
	SSDFS_DUMPFS_DUMP(env, "SEGMENT: %u bytes\n", seg_size);

	SSDFS_DUMPFS_DUMP(env, "CREATION_TIME: %s\n",
			   ssdfs_nanoseconds_to_time(create_time));
	SSDFS_DUMPFS_DUMP(env, "CREATION_CHECKPOINT: %llu\n",
			  create_cno);

	SSDFS_DUMPFS_DUMP(env, "\n");

	pair1 = &vh->sb_pebs[SSDFS_CUR_SB_SEG][SSDFS_MAIN_SB_SEG];
	pair2 = &vh->sb_pebs[SSDFS_CUR_SB_SEG][SSDFS_COPY_SB_SEG];
	SSDFS_DUMPFS_DUMP(env, "CURRENT_SUPERBLOCK_SEGMENT: "
			  "MAIN: LEB %llu, PEB %llu; "
			  "COPY: LEB %llu, PEB %llu\n",
			  le64_to_cpu(pair1->leb_id),
			  le64_to_cpu(pair1->peb_id),
			  le64_to_cpu(pair2->leb_id),
			  le64_to_cpu(pair2->peb_id));

	pair1 = &vh->sb_pebs[SSDFS_NEXT_SB_SEG][SSDFS_MAIN_SB_SEG];
	pair2 = &vh->sb_pebs[SSDFS_NEXT_SB_SEG][SSDFS_COPY_SB_SEG];
	SSDFS_DUMPFS_DUMP(env, "NEXT_SUPERBLOCK_SEGMENT: "
			  "MAIN: LEB %llu, PEB %llu; "
			  "COPY: LEB %llu, PEB %llu\n",
			  le64_to_cpu(pair1->leb_id),
			  le64_to_cpu(pair1->peb_id),
			  le64_to_cpu(pair2->leb_id),
			  le64_to_cpu(pair2->peb_id));

	pair1 = &vh->sb_pebs[SSDFS_RESERVED_SB_SEG][SSDFS_MAIN_SB_SEG];
	pair2 = &vh->sb_pebs[SSDFS_RESERVED_SB_SEG][SSDFS_COPY_SB_SEG];
	SSDFS_DUMPFS_DUMP(env, "RESERVED_SUPERBLOCK_SEGMENT: "
			  "MAIN: LEB %llu, PEB %llu; "
			  "COPY: LEB %llu, PEB %llu\n",
			  le64_to_cpu(pair1->leb_id),
			  le64_to_cpu(pair1->peb_id),
			  le64_to_cpu(pair2->leb_id),
			  le64_to_cpu(pair2->peb_id));

	pair1 = &vh->sb_pebs[SSDFS_PREV_SB_SEG][SSDFS_MAIN_SB_SEG];
	pair2 = &vh->sb_pebs[SSDFS_PREV_SB_SEG][SSDFS_COPY_SB_SEG];
	SSDFS_DUMPFS_DUMP(env, "PREVIOUS_SUPERBLOCK_SEGMENT: "
			  "MAIN: LEB %llu, PEB %llu; "
			  "COPY: LEB %llu, PEB %llu\n",
			  le64_to_cpu(pair1->leb_id),
			  le64_to_cpu(pair1->peb_id),
			  le64_to_cpu(pair2->leb_id),
			  le64_to_cpu(pair2->peb_id));

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "SB_SEGMENT_LOG_PAGES: %u\n",
			  le16_to_cpu(vh->sb_seg_log_pages));
	SSDFS_DUMPFS_DUMP(env, "SEGBMAP_LOG_PAGES: %u\n",
			  le16_to_cpu(vh->segbmap_log_pages));
	SSDFS_DUMPFS_DUMP(env, "MAPTBL_LOG_PAGES: %u\n",
			  le16_to_cpu(vh->maptbl_log_pages));
	SSDFS_DUMPFS_DUMP(env, "USER_DATA_LOG_PAGES: %u\n",
			  le16_to_cpu(vh->user_data_log_pages));

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "LOG_CREATION_TIME: %s\n",
		   ssdfs_nanoseconds_to_time(le64_to_cpu(hdr->timestamp)));
	SSDFS_DUMPFS_DUMP(env, "LOG_CREATION_CHECKPOINT: %llu\n",
			  le64_to_cpu(hdr->cno));
	SSDFS_DUMPFS_DUMP(env, "LOG_PAGES: %u\n",
			  le16_to_cpu(hdr->log_pages));

	switch (seg_type) {
	case SSDFS_UNKNOWN_SEG_TYPE:
		seg_type_str = "SSDFS_UNKNOWN_SEG_TYPE";
		break;
	case SSDFS_SB_SEG_TYPE:
		seg_type_str = "SSDFS_SB_SEG_TYPE";
		break;
	case SSDFS_INITIAL_SNAPSHOT_SEG_TYPE:
		seg_type_str = "SSDFS_INITIAL_SNAPSHOT_SEG_TYPE";
		break;
	case SSDFS_SEGBMAP_SEG_TYPE:
		seg_type_str = "SSDFS_SEGBMAP_SEG_TYPE";
		break;
	case SSDFS_MAPTBL_SEG_TYPE:
		seg_type_str = "SSDFS_MAPTBL_SEG_TYPE";
		break;
	case SSDFS_LEAF_NODE_SEG_TYPE:
		seg_type_str = "SSDFS_LEAF_NODE_SEG_TYPE";
		break;
	case SSDFS_HYBRID_NODE_SEG_TYPE:
		seg_type_str = "SSDFS_HYBRID_NODE_SEG_TYPE";
		break;
	case SSDFS_INDEX_NODE_SEG_TYPE:
		seg_type_str = "SSDFS_INDEX_NODE_SEG_TYPE";
		break;
	case SSDFS_USER_DATA_SEG_TYPE:
		seg_type_str = "SSDFS_USER_DATA_SEG_TYPE";
		break;
	default:
		BUG();
	}

	SSDFS_DUMPFS_DUMP(env, "SEG_TYPE: %s\n", seg_type_str);

	SSDFS_DUMPFS_DUMP(env, "SEG_FLAGS: ");

	if (seg_flags & SSDFS_SEG_HDR_HAS_BLK_BMAP)
		SSDFS_DUMPFS_DUMP(env, "SEG_HDR_HAS_BLK_BMAP ");

	if (seg_flags & SSDFS_SEG_HDR_HAS_OFFSET_TABLE)
		SSDFS_DUMPFS_DUMP(env, "SEG_HDR_HAS_OFFSET_TABLE ");

	if (seg_flags & SSDFS_LOG_HAS_COLD_PAYLOAD)
		SSDFS_DUMPFS_DUMP(env, "LOG_HAS_COLD_PAYLOAD ");

	if (seg_flags & SSDFS_LOG_HAS_WARM_PAYLOAD)
		SSDFS_DUMPFS_DUMP(env, "LOG_HAS_WARM_PAYLOAD ");

	if (seg_flags & SSDFS_LOG_HAS_HOT_PAYLOAD)
		SSDFS_DUMPFS_DUMP(env, "LOG_HAS_HOT_PAYLOAD ");

	if (seg_flags & SSDFS_LOG_HAS_BLK_DESC_CHAIN)
		SSDFS_DUMPFS_DUMP(env, "LOG_HAS_BLK_DESC_CHAIN ");

	if (seg_flags & SSDFS_LOG_HAS_MAPTBL_CACHE)
		SSDFS_DUMPFS_DUMP(env, "LOG_HAS_MAPTBL_CACHE ");

	if (seg_flags & SSDFS_LOG_HAS_FOOTER)
		SSDFS_DUMPFS_DUMP(env, "LOG_HAS_FOOTER ");

	if (seg_flags & SSDFS_LOG_IS_PARTIAL)
		SSDFS_DUMPFS_DUMP(env, "LOG_IS_PARTIAL ");

	if (seg_flags & SSDFS_LOG_HAS_PARTIAL_HEADER)
		SSDFS_DUMPFS_DUMP(env, "LOG_HAS_PARTIAL_HEADER ");

	if (seg_flags & SSDFS_PARTIAL_HEADER_INSTEAD_FOOTER)
		SSDFS_DUMPFS_DUMP(env, "PARTIAL_HEADER_INSTEAD_FOOTER ");

	SSDFS_DUMPFS_DUMP(env, "\n");

	desc = &hdr->desc_array[SSDFS_BLK_BMAP_INDEX];
	SSDFS_DUMPFS_DUMP(env, "BLOCK_BITMAP: offset %u, size %u\n",
			  le32_to_cpu(desc->offset),
			  le32_to_cpu(desc->size));

	desc = &hdr->desc_array[SSDFS_OFF_TABLE_INDEX];
	SSDFS_DUMPFS_DUMP(env, "OFFSETS_TABLE: offset %u, size %u\n",
			  le32_to_cpu(desc->offset),
			  le32_to_cpu(desc->size));

	desc = &hdr->desc_array[SSDFS_COLD_PAYLOAD_AREA_INDEX];
	SSDFS_DUMPFS_DUMP(env, "COLD_PAYLOAD_AREA: offset %u, size %u\n",
			  le32_to_cpu(desc->offset),
			  le32_to_cpu(desc->size));

	desc = &hdr->desc_array[SSDFS_WARM_PAYLOAD_AREA_INDEX];
	SSDFS_DUMPFS_DUMP(env, "WARM_PAYLOAD_AREA: offset %u, size %u\n",
			  le32_to_cpu(desc->offset),
			  le32_to_cpu(desc->size));

	desc = &hdr->desc_array[SSDFS_HOT_PAYLOAD_AREA_INDEX];
	SSDFS_DUMPFS_DUMP(env, "HOT_PAYLOAD_AREA: offset %u, size %u\n",
			  le32_to_cpu(desc->offset),
			  le32_to_cpu(desc->size));

	desc = &hdr->desc_array[SSDFS_BLK_DESC_AREA_INDEX];
	SSDFS_DUMPFS_DUMP(env, "BLOCK_DESCRIPTOR_AREA: offset %u, size %u\n",
			  le32_to_cpu(desc->offset),
			  le32_to_cpu(desc->size));

	desc = &hdr->desc_array[SSDFS_MAPTBL_CACHE_INDEX];
	SSDFS_DUMPFS_DUMP(env, "MAPTBL_CACHE_AREA: offset %u, size %u\n",
			  le32_to_cpu(desc->offset),
			  le32_to_cpu(desc->size));

	desc = &hdr->desc_array[SSDFS_LOG_FOOTER_INDEX];
	SSDFS_DUMPFS_DUMP(env, "LOG_FOOTER: offset %u, size %u\n",
			  le32_to_cpu(desc->offset),
			  le32_to_cpu(desc->size));

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "PREV_MIGRATING_PEB: migration_id %u\n",
			  hdr->peb_migration_id[SSDFS_PREV_MIGRATING_PEB]);
	SSDFS_DUMPFS_DUMP(env, "CUR_MIGRATING_PEB: migration_id %u\n",
			  hdr->peb_migration_id[SSDFS_CUR_MIGRATING_PEB]);

	SSDFS_DUMPFS_DUMP(env, "\n");

	ssdfs_dumpfs_parse_segbmap_sb_header(env, hdr);

	SSDFS_DUMPFS_DUMP(env, "\n");

	ssdfs_dumpfs_parse_maptbl_sb_header(env, hdr);

	SSDFS_DUMPFS_DUMP(env, "\n");

	ssdfs_dumpfs_parse_dentries_btree_descriptor(env, &vh->dentries_btree);

	SSDFS_DUMPFS_DUMP(env, "\n");

	ssdfs_dumpfs_parse_extents_btree_descriptor(env, &vh->extents_btree);

	SSDFS_DUMPFS_DUMP(env, "\n");

	ssdfs_dumpfs_parse_xattr_btree_descriptor(env, &vh->xattr_btree);
}

static
int ssdfs_dumpfs_read_footer_log_bytes(struct ssdfs_dumpfs_environment *env,
					union ssdfs_log_header *buf)
{
	struct ssdfs_partial_log_header *pl_hdr;
	struct ssdfs_log_footer *footer;
	struct ssdfs_metadata_descriptor *desc;
	void *area_buf = NULL;
	u32 area_offset;
	u32 area_size;
	u32 seg_flags;
	int err = 0;

	SSDFS_DBG(env->base.show_debug,
		  "read footer log bytes\n");

	desc = &buf->seg_hdr.desc_array[SSDFS_LOG_FOOTER_INDEX];
	area_offset = le32_to_cpu(desc->offset);
	area_size = le32_to_cpu(desc->size);
	seg_flags = le32_to_cpu(buf->seg_hdr.seg_flags);

	if (is_ssdfs_dumpfs_area_valid(desc)) {
		area_buf = malloc(area_size);
		if (!area_buf) {
			SSDFS_ERR("fail to allocate memory: "
				  "size %u\n", area_size);
			return -ENOMEM;
		}

		memset(area_buf, 0, area_size);

		if (seg_flags & SSDFS_PARTIAL_HEADER_INSTEAD_FOOTER) {
			err = ssdfs_dumpfs_read_partial_log_footer(env,
							   env->peb.id,
							   env->peb.peb_size,
							   env->peb.log_offset,
							   env->peb.log_size,
							   area_offset,
							   area_size,
							   area_buf);
			if (err) {
				SSDFS_ERR("fail to read partial log footer: "
					  "peb_id %llu, peb_size %u, "
					  "log_index %u, log_offset %u, "
					  "err %d\n",
					  env->peb.id, env->peb.peb_size,
					  env->peb.log_index,
					  env->peb.log_offset, err);
				goto finish_read_log_size;
			}

			pl_hdr = (struct ssdfs_partial_log_header *)area_buf;
			env->peb.log_size = le32_to_cpu(pl_hdr->log_bytes);
		} else if (seg_flags & SSDFS_LOG_HAS_FOOTER) {
			err = ssdfs_dumpfs_read_log_footer(env,
							   env->peb.id,
							   env->peb.peb_size,
							   env->peb.log_offset,
							   env->peb.log_size,
							   area_offset,
							   area_size,
							   area_buf);
			if (err) {
				SSDFS_ERR("fail to read log footer: "
					  "peb_id %llu, peb_size %u, "
					  "log_offset %u, err %d\n",
					  env->peb.id, env->peb.peb_size,
					  env->peb.log_offset, err);
				goto finish_read_log_size;
			}

			footer = (struct ssdfs_log_footer *)area_buf;
			env->peb.log_size = le32_to_cpu(footer->log_bytes);
		} else {
			err = -EIO;
			SSDFS_ERR("segment header is corrupted\n");
		}

finish_read_log_size:
		free(area_buf);
		area_buf = NULL;
	}

	return err;
}

static
int ssdfs_dumpfs_read_log_bytes(struct ssdfs_dumpfs_environment *env,
				union ssdfs_log_header *buf)
{
	u16 key;
	int err = 0;

	SSDFS_DBG(env->base.show_debug,
		  "read log bytes\n");

	err = ssdfs_dumpfs_read_segment_header(env, env->peb.id,
						env->peb.peb_size,
						env->peb.log_offset,
						env->peb.log_size,
						&buf->magic);
	if (err) {
		SSDFS_ERR("fail to read PEB's header: "
			  "peb_id %llu, peb_size %u, "
			  "log_offset %u, err %d\n",
			  env->peb.id, env->peb.peb_size,
			  env->peb.log_offset, err);
		return err;
	}

	if (le32_to_cpu(buf->magic.common) == SSDFS_SUPER_MAGIC) {
		key = le16_to_cpu(buf->magic.key);

		if (key == SSDFS_SEGMENT_HDR_MAGIC) {
			err = ssdfs_dumpfs_read_footer_log_bytes(env, buf);
			if (err) {
				SSDFS_ERR("fail to read footer log bytes: "
					  "err %d\n", err);
				return err;
			}
		} else if (key == SSDFS_PARTIAL_LOG_HDR_MAGIC) {
			env->peb.log_size =
				le32_to_cpu(buf->pl_hdr.log_bytes);
		} else
			BUG();
	} else {
		if (env->peb.log_size == U32_MAX) {
			SSDFS_INFO("PLEASE, DEFINE LOG SIZE\n");
			print_usage();
			return -EINVAL;
		}
	}

	return 0;
}

static int
__ssdfs_dumpfs_parse_partial_log_header(struct ssdfs_dumpfs_environment *env,
					u32 area_offset,
					void *area_buf, u32 area_size)
{
	struct ssdfs_partial_log_header *pl_hdr = NULL;
	size_t plh_size = sizeof(struct ssdfs_partial_log_header);
	struct ssdfs_metadata_descriptor *desc;
	u32 flags;
	u32 page_size;
	u32 erase_size;
	u32 seg_size;
	u16 seg_type;
	const char *seg_type_str = NULL;
	u64 offset;
	int err = 0;

	SSDFS_DBG(env->base.show_debug,
		  "parse partial log header: "
		  "area_offset %u, area_size %u\n",
		  area_offset, area_size);

	if (area_size < plh_size) {
		SSDFS_ERR("area_size %u < partial log hdr size %zu\n",
			  area_size, plh_size);
		return -EINVAL;
	}

	pl_hdr = (struct ssdfs_partial_log_header *)area_buf;

	page_size = 1 << pl_hdr->log_pagesize;
	erase_size = 1 << pl_hdr->log_erasesize;
	seg_size = 1 << pl_hdr->log_segsize;
	seg_type = le16_to_cpu(pl_hdr->seg_type);

	SSDFS_DUMPFS_DUMP(env, "PARTIAL LOG HEADER:\n");

	ssdfs_dumpfs_parse_magic(env, &pl_hdr->magic);

	SSDFS_DUMPFS_DUMP(env, "METADATA CHECK:\n");
	SSDFS_DUMPFS_DUMP(env, "BYTES: %u\n", le16_to_cpu(pl_hdr->check.bytes));

	flags = le16_to_cpu(pl_hdr->check.flags);

	SSDFS_DUMPFS_DUMP(env, "METADATA CHECK FLAGS: ");

	if (flags & SSDFS_CRC32)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_CRC32 ");

	if (flags == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "CHECKSUM: %#x\n", le32_to_cpu(pl_hdr->check.csum));

	SSDFS_DUMPFS_DUMP(env, "SEQUENCE_ID: %u\n",
			  le32_to_cpu(pl_hdr->sequence_id));
	SSDFS_DUMPFS_DUMP(env, "PAGE: %u bytes\n", page_size);
	SSDFS_DUMPFS_DUMP(env, "PEB: %u bytes\n", erase_size);
	SSDFS_DUMPFS_DUMP(env, "PEBS_PER_SEGMENT: %u\n",
			  1 << pl_hdr->log_pebs_per_seg);
	SSDFS_DUMPFS_DUMP(env, "SEGMENT: %u bytes\n", seg_size);

	SSDFS_DUMPFS_DUMP(env, "SEGMENT NUMBERS: %llu\n",
						le64_to_cpu(pl_hdr->nsegs));
	SSDFS_DUMPFS_DUMP(env, "FREE PAGES: %llu\n",
						le64_to_cpu(pl_hdr->free_pages));
	SSDFS_DUMPFS_DUMP(env, "LOG_CREATION_TIME: %s\n",
		   ssdfs_nanoseconds_to_time(le64_to_cpu(pl_hdr->timestamp)));
	SSDFS_DUMPFS_DUMP(env, "CHECKPOINT: %llu\n", le64_to_cpu(pl_hdr->cno));
	SSDFS_DUMPFS_DUMP(env, "LOG_PAGES: %u\n",
			  le16_to_cpu(pl_hdr->log_pages));

	switch (seg_type) {
	case SSDFS_UNKNOWN_SEG_TYPE:
		seg_type_str = "SSDFS_UNKNOWN_SEG_TYPE";
		break;
	case SSDFS_SB_SEG_TYPE:
		seg_type_str = "SSDFS_SB_SEG_TYPE";
		break;
	case SSDFS_INITIAL_SNAPSHOT_SEG_TYPE:
		seg_type_str = "SSDFS_INITIAL_SNAPSHOT_SEG_TYPE";
		break;
	case SSDFS_SEGBMAP_SEG_TYPE:
		seg_type_str = "SSDFS_SEGBMAP_SEG_TYPE";
		break;
	case SSDFS_MAPTBL_SEG_TYPE:
		seg_type_str = "SSDFS_MAPTBL_SEG_TYPE";
		break;
	case SSDFS_LEAF_NODE_SEG_TYPE:
		seg_type_str = "SSDFS_LEAF_NODE_SEG_TYPE";
		break;
	case SSDFS_HYBRID_NODE_SEG_TYPE:
		seg_type_str = "SSDFS_HYBRID_NODE_SEG_TYPE";
		break;
	case SSDFS_INDEX_NODE_SEG_TYPE:
		seg_type_str = "SSDFS_INDEX_NODE_SEG_TYPE";
		break;
	case SSDFS_USER_DATA_SEG_TYPE:
		seg_type_str = "SSDFS_USER_DATA_SEG_TYPE";
		break;
	default:
		BUG();
	}

	SSDFS_DUMPFS_DUMP(env, "SEG_TYPE: %s\n", seg_type_str);

	flags = le32_to_cpu(pl_hdr->flags);

	SSDFS_DUMPFS_DUMP(env, "VOLUME STATE FLAGS: ");

	if (flags & SSDFS_HAS_INLINE_INODES_TREE)
		SSDFS_DUMPFS_DUMP(env, "SSDFS_HAS_INLINE_INODES_TREE ");

	if (flags == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "LOG BYTES: %u bytes\n",
				le32_to_cpu(pl_hdr->log_bytes));

	flags = le32_to_cpu(pl_hdr->pl_flags);

	SSDFS_DUMPFS_DUMP(env, "PARTIAL HEADER FLAGS: ");

	if (flags & SSDFS_SEG_HDR_HAS_BLK_BMAP)
		SSDFS_DUMPFS_DUMP(env, "SEG_HDR_HAS_BLK_BMAP ");

	if (flags & SSDFS_SEG_HDR_HAS_OFFSET_TABLE)
		SSDFS_DUMPFS_DUMP(env, "SEG_HDR_HAS_OFFSET_TABLE ");

	if (flags & SSDFS_LOG_HAS_COLD_PAYLOAD)
		SSDFS_DUMPFS_DUMP(env, "LOG_HAS_COLD_PAYLOAD ");

	if (flags & SSDFS_LOG_HAS_WARM_PAYLOAD)
		SSDFS_DUMPFS_DUMP(env, "LOG_HAS_WARM_PAYLOAD ");

	if (flags & SSDFS_LOG_HAS_HOT_PAYLOAD)
		SSDFS_DUMPFS_DUMP(env, "LOG_HAS_HOT_PAYLOAD ");

	if (flags & SSDFS_LOG_HAS_BLK_DESC_CHAIN)
		SSDFS_DUMPFS_DUMP(env, "LOG_HAS_BLK_DESC_CHAIN ");

	if (flags & SSDFS_LOG_HAS_MAPTBL_CACHE)
		SSDFS_DUMPFS_DUMP(env, "LOG_HAS_MAPTBL_CACHE ");

	if (flags & SSDFS_LOG_HAS_FOOTER)
		SSDFS_DUMPFS_DUMP(env, "LOG_HAS_FOOTER ");

	if (flags & SSDFS_LOG_IS_PARTIAL)
		SSDFS_DUMPFS_DUMP(env, "LOG_IS_PARTIAL ");

	if (flags & SSDFS_LOG_HAS_PARTIAL_HEADER)
		SSDFS_DUMPFS_DUMP(env, "LOG_HAS_PARTIAL_HEADER ");

	if (flags & SSDFS_PARTIAL_HEADER_INSTEAD_FOOTER)
		SSDFS_DUMPFS_DUMP(env, "PARTIAL_HEADER_INSTEAD_FOOTER ");

	if (flags == 0)
		SSDFS_DUMPFS_DUMP(env, "NONE");

	SSDFS_DUMPFS_DUMP(env, "\n");

	desc = &pl_hdr->desc_array[SSDFS_BLK_BMAP_INDEX];
	SSDFS_DUMPFS_DUMP(env, "BLOCK_BITMAP: offset %u, size %u\n",
			  le32_to_cpu(desc->offset),
			  le32_to_cpu(desc->size));

	desc = &pl_hdr->desc_array[SSDFS_OFF_TABLE_INDEX];
	SSDFS_DUMPFS_DUMP(env, "OFFSETS_TABLE: offset %u, size %u\n",
			  le32_to_cpu(desc->offset),
			  le32_to_cpu(desc->size));

	desc = &pl_hdr->desc_array[SSDFS_COLD_PAYLOAD_AREA_INDEX];
	SSDFS_DUMPFS_DUMP(env, "COLD_PAYLOAD_AREA: offset %u, size %u\n",
			  le32_to_cpu(desc->offset),
			  le32_to_cpu(desc->size));

	desc = &pl_hdr->desc_array[SSDFS_WARM_PAYLOAD_AREA_INDEX];
	SSDFS_DUMPFS_DUMP(env, "WARM_PAYLOAD_AREA: offset %u, size %u\n",
			  le32_to_cpu(desc->offset),
			  le32_to_cpu(desc->size));

	desc = &pl_hdr->desc_array[SSDFS_HOT_PAYLOAD_AREA_INDEX];
	SSDFS_DUMPFS_DUMP(env, "HOT_PAYLOAD_AREA: offset %u, size %u\n",
			  le32_to_cpu(desc->offset),
			  le32_to_cpu(desc->size));

	desc = &pl_hdr->desc_array[SSDFS_BLK_DESC_AREA_INDEX];
	SSDFS_DUMPFS_DUMP(env, "BLOCK_DESCRIPTOR_AREA: offset %u, size %u\n",
			  le32_to_cpu(desc->offset),
			  le32_to_cpu(desc->size));

	desc = &pl_hdr->desc_array[SSDFS_MAPTBL_CACHE_INDEX];
	SSDFS_DUMPFS_DUMP(env, "MAPTBL_CACHE_AREA: offset %u, size %u\n",
			  le32_to_cpu(desc->offset),
			  le32_to_cpu(desc->size));

	desc = &pl_hdr->desc_array[SSDFS_LOG_FOOTER_INDEX];
	SSDFS_DUMPFS_DUMP(env, "LOG_FOOTER: offset %u, size %u\n",
			  le32_to_cpu(desc->offset),
			  le32_to_cpu(desc->size));

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "ROOT FOLDER:\n");
	ssdfs_dumpfs_parse_raw_inode(env, &pl_hdr->root_folder);

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "INODES B-TREE HEADER:\n");
	ssdfs_dumpfs_parse_btree_descriptor(env, &pl_hdr->inodes_btree.desc);
	SSDFS_DUMPFS_DUMP(env, "ALLOCATED INODES: %llu\n",
		   le64_to_cpu(pl_hdr->inodes_btree.allocated_inodes));
	SSDFS_DUMPFS_DUMP(env, "FREE INODES: %llu\n",
		   le64_to_cpu(pl_hdr->inodes_btree.free_inodes));
	SSDFS_DUMPFS_DUMP(env, "INODES CAPACITY: %llu\n",
		   le64_to_cpu(pl_hdr->inodes_btree.inodes_capacity));
	SSDFS_DUMPFS_DUMP(env, "LEAF NODES: %u\n",
		   le32_to_cpu(pl_hdr->inodes_btree.leaf_nodes));
	SSDFS_DUMPFS_DUMP(env, "NODES COUNT: %u\n",
		   le32_to_cpu(pl_hdr->inodes_btree.nodes_count));
	SSDFS_DUMPFS_DUMP(env, "UPPER_ALLOCATED_INO: %llu\n",
		   le64_to_cpu(pl_hdr->inodes_btree.upper_allocated_ino));
	ssdfs_dumpfs_parse_inline_root_node(env, &pl_hdr->inodes_btree.root_node);

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "SHARED EXTENTS B-TREE HEADER:\n");
	ssdfs_dumpfs_parse_btree_descriptor(env,
				&pl_hdr->shared_extents_btree.desc);
	ssdfs_dumpfs_parse_inline_root_node(env,
				&pl_hdr->shared_extents_btree.root_node);

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "SHARED DICTIONARY B-TREE HEADER:\n");
	ssdfs_dumpfs_parse_btree_descriptor(env,
				&pl_hdr->shared_dict_btree.desc);
	ssdfs_dumpfs_parse_inline_root_node(env,
				&pl_hdr->shared_dict_btree.root_node);

	SSDFS_DUMPFS_DUMP(env, "\n");

	SSDFS_DUMPFS_DUMP(env, "\n");

	if (env->is_raw_dump_requested) {
		offset = env->peb.id * env->peb.peb_size;
		env->raw_dump.offset = offset + area_offset;
		env->raw_dump.size = area_size;

		err = ssdfs_dumpfs_show_raw_dump(env);
		if (err) {
			SSDFS_ERR("fail to make partial log header "
				  "raw dump: "
				  "peb_id %llu, err %d\n",
				  env->peb.id, err);
			return err;
		}

		SSDFS_DUMPFS_DUMP(env, "\n");
	}

	return 0;
}

static
int ssdfs_dumpfs_parse_log_footer(struct ssdfs_dumpfs_environment *env,
				  union ssdfs_log_header *buf)
{
	struct ssdfs_metadata_descriptor *desc;
	void *area_buf = NULL;
	u32 area_offset;
	u32 area_size;
	int is_log_partial = SSDFS_FALSE;
	int err = 0;

	SSDFS_DBG(env->base.show_debug,
		  "parse log footer\n");

	is_log_partial =
		le32_to_cpu(buf->seg_hdr.seg_flags) & SSDFS_LOG_IS_PARTIAL;

	desc = &buf->seg_hdr.desc_array[SSDFS_LOG_FOOTER_INDEX];
	area_offset = le32_to_cpu(desc->offset);
	area_size = le32_to_cpu(desc->size);

	if (is_ssdfs_dumpfs_area_valid(desc)) {
		area_buf = malloc(area_size);
		if (!area_buf) {
			err = -ENOMEM;
			SSDFS_ERR("fail to allocate memory: "
				  "size %u\n", area_size);
			goto fail_parse_log_footer;
		}

		memset(area_buf, 0, area_size);

		if (is_log_partial) {
			err = ssdfs_dumpfs_read_partial_log_footer(env,
							   env->peb.id,
							   env->peb.peb_size,
							   env->peb.log_offset,
							   env->peb.log_size,
							   area_offset,
							   area_size,
							   area_buf);
			if (err) {
				SSDFS_ERR("fail to read partial log footer: "
					  "peb_id %llu, peb_size %u, "
					  "log_index %u, log_offset %u, "
					  "err %d\n",
					  env->peb.id, env->peb.peb_size,
					  env->peb.log_index,
					  env->peb.log_offset, err);
				goto finish_parse_log_footer;
			}

			err = __ssdfs_dumpfs_parse_partial_log_header(env,
								area_offset,
								area_buf,
								area_size);
			if (err) {
				SSDFS_ERR("fail to parse partial log footer: "
					  "peb_id %llu, log_index %u, "
					  "log_offset %u, err %d\n",
					  env->peb.id, env->peb.log_index,
					  env->peb.log_offset, err);
				goto finish_parse_log_footer;
			}
		} else {
			err = ssdfs_dumpfs_read_log_footer(env,
							   env->peb.id,
							   env->peb.peb_size,
							   env->peb.log_offset,
							   env->peb.log_size,
							   area_offset,
							   area_size,
							   area_buf);
			if (err) {
				SSDFS_ERR("fail to read log footer: "
					  "peb_id %llu, peb_size %u, "
					  "log_index %u, log_offset %u, "
					  "err %d\n",
					  env->peb.id, env->peb.peb_size,
					  env->peb.log_index,
					  env->peb.log_offset, err);
				goto finish_parse_log_footer;
			}

			err = __ssdfs_dumpfs_parse_log_footer(env,
							      area_offset,
							      area_buf,
							      area_size);
			if (err) {
				SSDFS_ERR("fail to parse log footer: "
					  "peb_id %llu, log_index %u, "
					  "log_offset %u, err %d\n",
					  env->peb.id, env->peb.log_index,
					  env->peb.log_offset, err);
				goto finish_parse_log_footer;
			}
		}

finish_parse_log_footer:
		free(area_buf);
		area_buf = NULL;

		if (err)
			goto fail_parse_log_footer;
	}

fail_parse_log_footer:
	return err;
}

static
int ssdfs_dumpfs_parse_full_log(struct ssdfs_dumpfs_environment *env,
				union ssdfs_log_header *buf)
{
	struct ssdfs_metadata_descriptor *desc;
	void *area_buf = NULL;
	u64 offset;
	u32 area_offset;
	u32 area_size;
	int err = 0;

	SSDFS_DBG(env->base.show_debug,
		  "parse full log\n");

	offset = env->peb.id * env->peb.peb_size;

	err = ssdfs_dumpfs_open_file(env);
	if (err) {
		SSDFS_ERR("unable to open output file: "
			  "PEB %llu, log_index %u, "
			  "log_offset %u, err %d\n",
			  env->peb.id, env->peb.log_index,
			  env->peb.log_offset, err);
		goto finish_parse_full_log;
	}

	SSDFS_DUMPFS_DUMP(env, "PEB_ID %llu, LOG_INDEX %u, LOG_OFFSET %u\n\n",
			  env->peb.id, env->peb.log_index,
			  env->peb.log_offset);

	if (!(env->peb.parse_flags & SSDFS_PARSE_HEADER))
		goto parse_block_bitmap;

	ssdfs_dumpfs_parse_segment_header(env, &buf->seg_hdr);

	SSDFS_DUMPFS_DUMP(env, "\n");

	if (env->is_raw_dump_requested) {
		if (env->peb.id == SSDFS_INITIAL_SNAPSHOT_SEG &&
		    env->peb.log_index == 0) {
			env->raw_dump.offset = SSDFS_RESERVED_VBR_SIZE;
		} else {
			env->raw_dump.offset = offset;
			env->raw_dump.offset += env->peb.log_offset;
		}

		env->raw_dump.size =
			sizeof(struct ssdfs_segment_header);

		err = ssdfs_dumpfs_show_raw_dump(env);
		if (err) {
			SSDFS_ERR("fail to make segment header dump: "
				  "peb_id %llu, err %d\n",
				  env->peb.id, err);
			goto close_opened_file;
		}

		SSDFS_DUMPFS_DUMP(env, "\n");
	}

parse_block_bitmap:
	if (!(env->peb.parse_flags & SSDFS_PARSE_BLOCK_BITMAP))
		goto parse_blk2off_table;

	err = ssdfs_dumpfs_parse_block_bitmap_area(env,
			&buf->seg_hdr.desc_array[SSDFS_BLK_BMAP_INDEX]);
	if (err) {
		SSDFS_ERR("fail to parse block bitmap: err %d\n", err);
		goto close_opened_file;
	}

parse_blk2off_table:
	if (!(env->peb.parse_flags & SSDFS_PARSE_BLK2OFF_TABLE))
		goto parse_block_state_area;

	err = ssdfs_dumpfs_parse_blk2off_area(env,
			&buf->seg_hdr.desc_array[SSDFS_OFF_TABLE_INDEX]);
	if (err) {
		SSDFS_ERR("fail to parse blk2 off table: err %d\n",
			  err);
		goto close_opened_file;
	}

parse_block_state_area:
	if (!(env->peb.parse_flags & SSDFS_PARSE_BLOCK_STATE_AREA))
		goto parse_log_footer;

	desc = &buf->seg_hdr.desc_array[SSDFS_BLK_DESC_AREA_INDEX];
	area_offset = le32_to_cpu(desc->offset);
	area_size = le32_to_cpu(desc->size);

	if (is_ssdfs_dumpfs_area_valid(desc)) {
		area_buf = malloc(area_size);
		if (!area_buf) {
			err = -ENOMEM;
			SSDFS_ERR("fail to allocate memory: "
				  "size %u\n", area_size);
			goto close_opened_file;
		}

		memset(area_buf, 0, area_size);

		err = ssdfs_dumpfs_read_blk_desc_array(env,
						     env->peb.id,
						     env->peb.peb_size,
						     env->peb.log_offset,
						     env->peb.log_size,
						     area_offset,
						     area_size,
						     area_buf);
		if (err) {
			SSDFS_ERR("fail to read block descriptors: "
				  "peb_id %llu, peb_size %u, "
				  "log_index %u, log_offset %u, err %d\n",
				  env->peb.id, env->peb.peb_size,
				  env->peb.log_index,
				  env->peb.log_offset, err);
			goto finish_parse_blk_desc_array;
		}

		err = ssdfs_dumpfs_parse_blk_desc_array(env, area_buf,
							area_size);
		if (err) {
			err = 0;
			SSDFS_ERR("fail to parse block descriptors: "
				  "peb_id %llu, log_index %u, "
				  "log_offset %u, err %d\n",
				  env->peb.id, env->peb.log_index,
				  env->peb.log_offset, err);
			goto finish_parse_blk_desc_array;
		}

finish_parse_blk_desc_array:
		free(area_buf);
		area_buf = NULL;

		if (err)
			goto close_opened_file;

		SSDFS_DUMPFS_DUMP(env, "\n");

		if (env->is_raw_dump_requested) {
			env->raw_dump.offset = offset + area_offset;
			env->raw_dump.size = area_size;

			err = ssdfs_dumpfs_show_raw_dump(env);
			if (err) {
				SSDFS_ERR("fail to make blk desc array "
					  "raw dump: "
					  "peb_id %llu, err %d\n",
					  env->peb.id, err);
				goto close_opened_file;
			}

			SSDFS_DUMPFS_DUMP(env, "\n");
		}
	}

parse_log_footer:
	if (!(env->peb.parse_flags & SSDFS_PARSE_LOG_FOOTER))
		goto close_opened_file;

	err = ssdfs_dumpfs_parse_log_footer(env, buf);
	if (err) {
		SSDFS_ERR("fail to parse log footer: err %d\n", err);
		goto close_opened_file;
	}

close_opened_file:
	ssdfs_dumpfs_close_file(env);

finish_parse_full_log:
	return err;
}

static
int ssdfs_dumpfs_parse_partial_log(struct ssdfs_dumpfs_environment *env,
				   union ssdfs_log_header *buf)
{
	struct ssdfs_metadata_descriptor *desc;
	size_t hdr_size = sizeof(struct ssdfs_partial_log_header);
	void *area_buf = NULL;
	u64 offset;
	u32 area_offset;
	u32 area_size;
	int has_footer = SSDFS_FALSE;
	int err = 0;

	SSDFS_DBG(env->base.show_debug,
		  "parse partial log\n");

	offset = env->peb.id * env->peb.peb_size;

	has_footer =
		le32_to_cpu(buf->pl_hdr.pl_flags) & SSDFS_LOG_HAS_FOOTER;

	err = ssdfs_dumpfs_open_file(env);
	if (err) {
		SSDFS_ERR("unable to open output file: "
			  "PEB %llu, log_index %u, "
			  "log_offset %u, err %d\n",
			  env->peb.id, env->peb.log_index,
			  env->peb.log_offset, err);
		goto finish_parse_partial_log;
	}

	SSDFS_DUMPFS_DUMP(env, "PEB_ID %llu, LOG_INDEX %u, LOG_OFFSET %u\n\n",
			  env->peb.id, env->peb.log_index,
			  env->peb.log_offset);

	if (!(env->peb.parse_flags & SSDFS_PARSE_HEADER))
		goto parse_block_bitmap;

	err = __ssdfs_dumpfs_parse_partial_log_header(env,
						      env->peb.log_offset,
						      &buf->pl_hdr,
						      hdr_size);
	if (err) {
		SSDFS_ERR("fail to parse partial log footer: "
			  "peb_id %llu, log_index %u, "
			  "log_offset %u, err %d\n",
			  env->peb.id, env->peb.log_index,
			  env->peb.log_offset, err);
		goto close_opened_file;
	}

parse_block_bitmap:
	if (!(env->peb.parse_flags & SSDFS_PARSE_BLOCK_BITMAP))
		goto parse_blk2off_table;

	err = ssdfs_dumpfs_parse_block_bitmap_area(env,
			&buf->pl_hdr.desc_array[SSDFS_BLK_BMAP_INDEX]);
	if (err) {
		SSDFS_ERR("fail to parse block bitmap: err %d\n", err);
		goto close_opened_file;
	}

parse_blk2off_table:
	if (!(env->peb.parse_flags & SSDFS_PARSE_BLK2OFF_TABLE))
		goto parse_block_state_area;

	err = ssdfs_dumpfs_parse_blk2off_area(env,
			&buf->pl_hdr.desc_array[SSDFS_OFF_TABLE_INDEX]);
	if (err) {
		SSDFS_ERR("fail to parse blk2 off table: err %d\n",
			  err);
		goto close_opened_file;
	}

parse_block_state_area:
	if (!(env->peb.parse_flags & SSDFS_PARSE_BLOCK_STATE_AREA))
		goto parse_log_footer;

	desc = &buf->pl_hdr.desc_array[SSDFS_BLK_DESC_AREA_INDEX];
	area_offset = le32_to_cpu(desc->offset);
	area_size = le32_to_cpu(desc->size);

	if (is_ssdfs_dumpfs_area_valid(desc)) {
		area_buf = malloc(area_size);
		if (!area_buf) {
			err = -ENOMEM;
			SSDFS_ERR("fail to allocate memory: "
				  "size %u\n", area_size);
			goto close_opened_file;
		}

		memset(area_buf, 0, area_size);

		err = ssdfs_dumpfs_read_blk_desc_array(env,
						     env->peb.id,
						     env->peb.peb_size,
						     env->peb.log_offset,
						     env->peb.log_size,
						     area_offset,
						     area_size,
						     area_buf);
		if (err) {
			SSDFS_ERR("fail to read block descriptors: "
				  "peb_id %llu, peb_size %u, "
				  "log_index %u, log_offset %u, err %d\n",
				  env->peb.id, env->peb.peb_size,
				  env->peb.log_index,
				  env->peb.log_offset, err);
			goto finish_parse_blk_desc_array;
		}

		err = ssdfs_dumpfs_parse_blk_desc_array(env, area_buf,
							area_size);
		if (err) {
			err = 0;
			SSDFS_ERR("fail to parse block descriptors: "
				  "peb_id %llu, log_index %u, "
				  "log_offset %u, err %d\n",
				  env->peb.id, env->peb.log_index,
				  env->peb.log_offset, err);
			goto finish_parse_blk_desc_array;
		}

finish_parse_blk_desc_array:
		free(area_buf);
		area_buf = NULL;

		if (err)
			goto close_opened_file;

		SSDFS_DUMPFS_DUMP(env, "\n");

		if (env->is_raw_dump_requested) {
			env->raw_dump.offset = offset + area_offset;
			env->raw_dump.size = area_size;

			err = ssdfs_dumpfs_show_raw_dump(env);
			if (err) {
				SSDFS_ERR("fail to make blk desc array "
					  "raw dump: "
					  "peb_id %llu, err %d\n",
					  env->peb.id, err);
				goto close_opened_file;
			}

			SSDFS_DUMPFS_DUMP(env, "\n");
		}
	}

parse_log_footer:
	if (!(env->peb.parse_flags & SSDFS_PARSE_LOG_FOOTER))
		goto close_opened_file;

	if (!has_footer)
		goto close_opened_file;

	desc = &buf->pl_hdr.desc_array[SSDFS_LOG_FOOTER_INDEX];
	area_offset = le32_to_cpu(desc->offset);
	area_size = le32_to_cpu(desc->size);

	if (is_ssdfs_dumpfs_area_valid(desc)) {
		area_buf = malloc(area_size);
		if (!area_buf) {
			err = -ENOMEM;
			SSDFS_ERR("fail to allocate memory: "
				  "size %u\n", area_size);
			goto close_opened_file;
		}

		memset(area_buf, 0, area_size);

		err = ssdfs_dumpfs_read_log_footer(env,
						   env->peb.id,
						   env->peb.peb_size,
						   env->peb.log_offset,
						   env->peb.log_size,
						   area_offset,
						   area_size,
						   area_buf);
		if (err) {
			SSDFS_ERR("fail to read log footer: "
				  "peb_id %llu, peb_size %u, "
				  "log_index %u, "
				  "log_offset %u err %d\n",
				  env->peb.id, env->peb.peb_size,
				  env->peb.log_index,
				  env->peb.log_offset, err);
			goto finish_parse_log_footer;
		}

		err = __ssdfs_dumpfs_parse_log_footer(env,
						      area_offset,
						      area_buf,
						      area_size);
		if (err) {
			SSDFS_ERR("fail to parse log footer: "
				  "peb_id %llu, log_index %u, "
				  "log_offset %u, err %d\n",
				  env->peb.id, env->peb.log_index,
				  env->peb.log_offset, err);
			goto finish_parse_log_footer;
		}

finish_parse_log_footer:
		free(area_buf);
		area_buf = NULL;

		if (err)
			goto close_opened_file;
	}

close_opened_file:
	ssdfs_dumpfs_close_file(env);

finish_parse_partial_log:
	return err;
}

int ssdfs_dumpfs_show_peb_dump(struct ssdfs_dumpfs_environment *env)
{
	union ssdfs_log_header buf;
	u64 peb_id;
	u64 pebs_count;
	int log_index;
	u16 logs_count;
	int step = 2;
	int i;
	int err = 0;

	SSDFS_DBG(env->base.show_debug,
		  "command: %#x\n",
		  env->command);

	if (env->peb.id == U64_MAX) {
		err = -EINVAL;
		SSDFS_INFO("PLEASE, DEFINE PEB ID\n");
		print_usage();
		goto finish_peb_dump;
	}

	if (env->peb.peb_size == U32_MAX) {
		SSDFS_DUMPFS_INFO(env->base.show_info,
				  "[00%d]\tFIND FIRST VALID PEB...\n",
				  step);

		err = ssdfs_dumpfs_find_any_valid_peb(env, &buf.seg_hdr);
		if (err) {
			SSDFS_INFO("PLEASE, DEFINE PEB SIZE\n");
			print_usage();
			goto finish_peb_dump;
		}

		SSDFS_DUMPFS_INFO(env->base.show_info,
				  "[00%d]\t[SUCCESS]\n",
				  step);

		step++;
		env->peb.peb_size = 1 << buf.seg_hdr.volume_hdr.log_erasesize;
	}

	SSDFS_DUMPFS_INFO(env->base.show_info,
			  "[00%d]\tDUMP PEB...\n",
			  step);

	err = ssdfs_dumpfs_read_segment_header(env, env->peb.id,
						env->peb.peb_size,
						0, env->peb.peb_size,
						&buf.magic);
	if (err) {
		SSDFS_ERR("fail to read PEB's header: "
			  "peb_id %llu, peb_size %u, err %d\n",
			  env->peb.id, env->peb.peb_size, err);
		goto finish_peb_dump;
	}

	if (env->peb.log_index == U16_MAX) {
		err = -EINVAL;
		SSDFS_INFO("PLEASE, DEFINE LOG INDEX\n");
		print_usage();
		goto finish_peb_dump;
	}

	peb_id = env->peb.id;
	pebs_count = env->peb.pebs_count;
	log_index = env->peb.log_index;
	logs_count = env->peb.logs_count;

try_find_start_log:
	for (i = 0; i < (env->peb.log_index - 1); i++) {
		err = ssdfs_dumpfs_read_log_bytes(env, &buf);
		if (err) {
			SSDFS_ERR("fail to read log's size in bytes: "
				  "peb_id %llu, peb_size %u, "
				  "log_offset %u, err %d\n",
				  env->peb.id, env->peb.peb_size,
				  env->peb.log_offset, err);
			goto finish_peb_dump;
		}

		env->peb.log_offset += env->peb.log_size;
	}

try_read_segment_header:
	err = ssdfs_dumpfs_read_log_bytes(env, &buf);
	if (err) {
		SSDFS_ERR("fail to read log's size in bytes: "
			  "peb_id %llu, peb_size %u, "
			  "log_offset %u, err %d\n",
			  env->peb.id, env->peb.peb_size,
			  env->peb.log_offset, err);
		goto finish_peb_dump;
	}

	if (le32_to_cpu(buf.magic.common) == SSDFS_SUPER_MAGIC &&
	    le16_to_cpu(buf.magic.key) == SSDFS_SEGMENT_HDR_MAGIC) {
		err = ssdfs_dumpfs_parse_full_log(env, &buf);
		if (err) {
			SSDFS_ERR("fail to parse the full log: "
				  "err %d\n", err);
			goto finish_peb_dump;
		}
	} else if (le32_to_cpu(buf.magic.common) == SSDFS_SUPER_MAGIC &&
		   le16_to_cpu(buf.magic.key) == SSDFS_PARTIAL_LOG_HDR_MAGIC) {
		err = ssdfs_dumpfs_parse_partial_log(env, &buf);
		if (err) {
			SSDFS_ERR("fail to parse the partial log: "
				  "err %d\n", err);
			goto finish_peb_dump;
		}
	} else {
		SSDFS_DBG(env->base.show_debug,
			  "LOG ABSENT: peb_id: %llu, log_index %u, "
			  "log_offset %u\n",
			  env->peb.id, env->peb.log_index,
			  env->peb.log_offset);
			goto try_next_peb;
	}

	if (env->peb.show_all_logs) {
		env->peb.log_index++;
		env->peb.logs_count--;
		env->peb.log_offset += env->peb.log_size;

		if (env->peb.log_offset < env->peb.peb_size) {
			if (env->peb.logs_count > 0)
				goto try_read_segment_header;
		}
	}

try_next_peb:
	if (env->peb.pebs_count > 0) {
		env->peb.id++;
		env->peb.pebs_count--;

		if (env->peb.id < (env->base.fs_size / env->peb.peb_size)) {
			env->peb.log_index = log_index;
			env->peb.logs_count = logs_count;
			env->peb.log_offset = 0;

			if (env->peb.pebs_count > 0)
				goto try_find_start_log;
		}
	}

	env->peb.id = peb_id;
	env->peb.pebs_count = pebs_count;
	env->peb.log_index = log_index;
	env->peb.logs_count = logs_count;
	env->peb.log_offset = 0;

	SSDFS_DUMPFS_INFO(env->base.show_info,
			  "[00%d]\t[SUCCESS]\n",
			  step);
	step++;

finish_peb_dump:
	return err;
}
