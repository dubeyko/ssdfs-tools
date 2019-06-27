//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/dump.ssdfs/show_peb_dump.c - show PEB dump command.
 *
 * Copyright (c) 2014-2018 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 *
 * HGST Confidential
 * (C) Copyright 2009-2018, HGST, Inc., All rights reserved.
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








static
int ssdfs_dumpfs_parse_blk2off_table(void *area_buf, u32 area_size)
{
	struct ssdfs_blk2off_table_header *hdr =
			(struct ssdfs_blk2off_table_header *)area_buf;
	size_t hdr_size = sizeof(struct ssdfs_blk2off_table_header);
	struct ssdfs_translation_extent *extents = NULL;
	size_t extent_desc_size = sizeof(struct ssdfs_translation_extent);
	struct ssdfs_phys_offset_table_header *pot_table = NULL;
	size_t pot_desc_size = sizeof(struct ssdfs_phys_offset_table_header);
	struct ssdfs_signature *magic = &hdr->magic;
	u8 *magic_common = (u8 *)&magic->common;
	u8 *magic_key = (u8 *)&magic->key;
	u16 flags;
	u16 extents_count;
	u16 offset_table_off;
	u32 pot_magic;
	size_t size;
	int i;
	int err;

	if (area_size < hdr_size) {
		SSDFS_ERR("area_size %u < hdr_size %zu\n",
			  area_size, hdr_size);
		return -EINVAL;
	}

	SSDFS_INFO("BLK2OFF TABLE:\n");

	SSDFS_INFO("MAGIC: %c%c%c%c %c%c\n",
		   *magic_common, *(magic_common + 1),
		   *(magic_common + 2), *(magic_common + 3),
		   *magic_key, *(magic_key + 1));
	SSDFS_INFO("VERSION: v.%u.%u\n",
		   magic->version.major,
		   magic->version.minor);

	SSDFS_INFO("METADATA CHECK:\n");
	SSDFS_INFO("BYTES: %u\n", le16_to_cpu(hdr->check.bytes));

	flags = le16_to_cpu(hdr->check.flags);

	SSDFS_INFO("METADATA CHECK FLAGS: ");

	if (flags & SSDFS_CRC32)
		SSDFS_INFO("SSDFS_CRC32 ");

	if (flags & SSDFS_BLK2OFF_TBL_ZLIB_COMPR)
		SSDFS_INFO("SSDFS_BLK2OFF_TBL_ZLIB_COMPR ");

	if (flags & SSDFS_BLK2OFF_TBL_LZO_COMPR)
		SSDFS_INFO("SSDFS_BLK2OFF_TBL_LZO_COMPR ");

	if (flags == 0)
		SSDFS_INFO("NONE");

	SSDFS_INFO("\n");

	SSDFS_INFO("CHECKSUM: %#x\n", le32_to_cpu(hdr->check.csum));

	SSDFS_INFO("EXTENTS OFFSET: %u bytes\n",
			le16_to_cpu(hdr->extents_off));
	extents_count = le16_to_cpu(hdr->extents_count);
	SSDFS_INFO("EXTENTS COUNT: %u\n", extents_count);
	offset_table_off = le16_to_cpu(hdr->offset_table_off);
	SSDFS_INFO("OFFSETS TABLE OFFSET: %u bytes\n", offset_table_off);
	SSDFS_INFO("FRAGMENTS COUNT: %u\n",
			le16_to_cpu(hdr->fragments_count));

	if (extents_count > 1) {
		size = hdr_size + ((extents_count - 1) * extent_desc_size);

		if (area_size < size) {
			SSDFS_ERR("area_size %u < size %zu\n",
				  area_size, size);
			return -EINVAL;
		}
	}

	SSDFS_INFO("\n");

	extents = &hdr->sequence[0];

	for (i = 0; i < extents_count; i++) {
		SSDFS_INFO("EXTENT#%d:\n", i);
		SSDFS_INFO("LOGICAL BLOCK: %u\n",
			    le16_to_cpu(extents[i].logical_blk));
		SSDFS_INFO("OFFSET_ID: %u\n",
			    le16_to_cpu(extents[i].offset_id));
		SSDFS_INFO("LENGTH: %u\n",
			    le16_to_cpu(extents[i].len));
		SSDFS_INFO("SEQUENCE_ID: %u\n",
			    extents[i].sequence_id);

		switch (extents[i].state) {
		case SSDFS_LOGICAL_BLK_FREE:
			SSDFS_INFO("EXTENT STATE: SSDFS_LOGICAL_BLK_FREE\n");
			break;

		case SSDFS_LOGICAL_BLK_USED:
			SSDFS_INFO("EXTENT STATE: SSDFS_LOGICAL_BLK_USED\n");
			break;

		default:
			SSDFS_INFO("EXTENT STATE: UNKNOWN\n");
			break;
		}

		SSDFS_INFO("\n");
	}

	if (area_size < (offset_table_off + pot_desc_size)) {
		SSDFS_ERR("area_size %u, offset_table_off %u, "
			  "pot_desc_size %zu\n",
			  area_size, offset_table_off,
			  pot_desc_size);
		return -EINVAL;
	}

	pot_table = (struct ssdfs_phys_offset_table_header *)((u8*)area_buf +
							    offset_table_off);

	SSDFS_INFO("PHYSICAL OFFSETS TABLE HEADER:\n");
	SSDFS_INFO("START_ID: %u\n", le16_to_cpu(pot_table->start_id));
	SSDFS_INFO("ID_COUNT: %u\n", le16_to_cpu(pot_table->id_count));
	SSDFS_INFO("BYTE_SIZE: %u bytes\n", le32_to_cpu(pot_table->byte_size));
	SSDFS_INFO("PEB INDEX: %u\n", le16_to_cpu(pot_table->peb_index));
	SSDFS_INFO("SEQUENCE_ID: %u\n", le16_to_cpu(pot_table->sequence_id));

	switch (le16_to_cpu(pot_table->type)) {
	case SSDFS_SEG_OFF_TABLE:
		SSDFS_INFO("OFFSET TABLE TYPE: SSDFS_SEG_OFF_TABLE\n");
		break;

	default:
		SSDFS_INFO("OFFSET TABLE TYPE: UNKNOWN\n");
		break;
	}

	flags = le16_to_cpu(pot_table->flags);

	SSDFS_INFO("OFFSET TABLE FLAGS: ");

	if (flags & SSDFS_OFF_TABLE_HAS_CSUM)
		SSDFS_INFO("SSDFS_OFF_TABLE_HAS_CSUM ");

	if (flags & SSDFS_OFF_TABLE_HAS_NEXT_FRAGMENT)
		SSDFS_INFO("SSDFS_OFF_TABLE_HAS_NEXT_FRAGMENT ");

	if (flags == 0)
		SSDFS_INFO("NONE");

	SSDFS_INFO("\n");

	pot_magic = le32_to_cpu(pot_table->magic);
	magic_common = (u8 *)&pot_magic;

	SSDFS_INFO("OFFSET TABLE MAGIC: %c%c%c%c\n",
		   *magic_common, *(magic_common + 1),
		   *(magic_common + 2), *(magic_common + 3));

	SSDFS_INFO("CHECKSUM: %#x\n", le32_to_cpu(pot_table->checksum));
	SSDFS_INFO("USED LOGICAL BLOCKS: %u\n",
			le16_to_cpu(pot_table->used_logical_blks));
	SSDFS_INFO("FREE LOGICAL BLOCKS: %u\n",
			le16_to_cpu(pot_table->free_logical_blks));
	SSDFS_INFO("LAST ALLOCATED BLOCK: %u\n",
			le16_to_cpu(pot_table->last_allocated_blk));
	SSDFS_INFO("NEXT FRAGMENT OFFSET: %u bytes\n",
			le16_to_cpu(pot_table->next_fragment_off));




	return 0;
}







static
int ssdfs_dumpfs_parse_block_bitmap_fragment(void *area_buf,
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
	u32 compr_bytes;
	u32 uncompr_bytes;
	u16 fragments_count;
	u16 desc_size;
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

	SSDFS_INFO("PEB_INDEX: %u\n", peb_index);
	SSDFS_INFO("SEQUENCE_ID: %u\n", sequence_id);

	SSDFS_INFO("FRAGMENT FLAGS: ");

	if (flags & SSDFS_MIGRATING_BLK_BMAP)
		SSDFS_INFO("SSDFS_MIGRATING_BLK_BMAP ");

	if (flags & SSDFS_PEB_HAS_EXT_PTR)
		SSDFS_INFO("SSDFS_PEB_HAS_EXT_PTR ");

	if (flags & SSDFS_PEB_HAS_RELATION)
		SSDFS_INFO("SSDFS_PEB_HAS_RELATION ");

	if (flags == 0)
		SSDFS_INFO("NONE");

	SSDFS_INFO("\n");

	switch (type) {
	case SSDFS_SRC_BLK_BMAP:
		SSDFS_INFO("FRAGMENT TYPE: SSDFS_SRC_BLK_BMAP\n");
		break;

	case SSDFS_DST_BLK_BMAP:
		SSDFS_INFO("FRAGMENT TYPE: SSDFS_DST_BLK_BMAP\n");
		break;

	default:
		SSDFS_INFO("FRAGMENT TYPE: UNKNOWN\n");
		break;
	}

	SSDFS_INFO("LAST_FREE_BLK: %u\n", last_free_blk);
	SSDFS_INFO("METADATA_BLKS: %u\n", metadata_blks);
	SSDFS_INFO("INVALID_BLKS: %u\n", invalid_blks);

	compr_bytes = le32_to_cpu(hdr->chain_hdr.compr_bytes);
	uncompr_bytes = le32_to_cpu(hdr->chain_hdr.uncompr_bytes);
	fragments_count = le16_to_cpu(hdr->chain_hdr.fragments_count);
	desc_size = le16_to_cpu(hdr->chain_hdr.desc_size);
	flags = le16_to_cpu(hdr->chain_hdr.flags);

	SSDFS_INFO("CHAIN HEADER:\n");
	SSDFS_INFO("COMPRESSED BYTES: %u bytes\n", compr_bytes);
	SSDFS_INFO("UNCOMPRESSED BYTES: %u bytes\n", uncompr_bytes);
	SSDFS_INFO("FRAGMENTS COUNT: %u\n", fragments_count);
	SSDFS_INFO("DESC_SIZE: %u bytes\n", desc_size);
	SSDFS_INFO("MAGIC: %c\n", hdr->chain_hdr.magic);

	switch (hdr->chain_hdr.type) {
	case SSDFS_LOG_AREA_CHAIN_HDR:
		SSDFS_INFO("CHAIN TYPE: SSDFS_LOG_AREA_CHAIN_HDR\n");
		break;

	case SSDFS_BLK_STATE_CHAIN_HDR:
		SSDFS_INFO("CHAIN TYPE: SSDFS_BLK_STATE_CHAIN_HDR\n");
		break;

	case SSDFS_BLK_DESC_CHAIN_HDR:
		SSDFS_INFO("CHAIN TYPE: SSDFS_BLK_DESC_CHAIN_HDR\n");
		break;

	case SSDFS_BLK_BMAP_CHAIN_HDR:
		SSDFS_INFO("CHAIN TYPE: SSDFS_BLK_BMAP_CHAIN_HDR\n");
		break;

	default:
		SSDFS_INFO("CHAIN TYPE: UNKNOWN\n");
		break;
	}

	SSDFS_INFO("CHAIN FLAGS: ");

	if (flags & SSDFS_MULTIPLE_HDR_CHAIN)
		SSDFS_INFO("SSDFS_MULTIPLE_HDR_CHAIN ");

	if (flags == 0)
		SSDFS_INFO("NONE");

	SSDFS_INFO("\n");

	*parsed_bytes += sizeof(struct ssdfs_block_bitmap_fragment);

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

		SSDFS_INFO("\n");
		SSDFS_INFO("FRAGMENT_INDEX: #%d\n", i);
		SSDFS_INFO("OFFSET: %u bytes\n",
			   le32_to_cpu(frag->offset));
		SSDFS_INFO("COMPRESSED_SIZE: %u bytes\n",
			   le16_to_cpu(frag->compr_size));
		SSDFS_INFO("UNCOMPRESSED_SIZE: %u bytes\n",
			   le16_to_cpu(frag->uncompr_size));
		SSDFS_INFO("CHECKSUM: %#x\n",
			   le32_to_cpu(frag->checksum));
		SSDFS_INFO("SEQUENCE_ID: %u\n",
			   frag->sequence_id);
		SSDFS_INFO("MAGIC: %c\n",
			   frag->magic);

		switch (frag->type) {
		case SSDFS_FRAGMENT_UNCOMPR_BLOB:
			SSDFS_INFO("FRAGMENT TYPE: SSDFS_FRAGMENT_UNCOMPR_BLOB\n");
			break;

		case SSDFS_FRAGMENT_ZLIB_BLOB:
			SSDFS_INFO("FRAGMENT TYPE: SSDFS_FRAGMENT_ZLIB_BLOB\n");
			break;

		case SSDFS_FRAGMENT_LZO_BLOB:
			SSDFS_INFO("FRAGMENT TYPE: SSDFS_FRAGMENT_LZO_BLOB\n");
			break;

		case SSDFS_DATA_BLK_STATE_DESC:
			SSDFS_INFO("FRAGMENT TYPE: SSDFS_DATA_BLK_STATE_DESC\n");
			break;

		case SSDFS_DATA_BLK_DESC:
			SSDFS_INFO("FRAGMENT TYPE: SSDFS_DATA_BLK_DESC\n");
			break;

		case SSDFS_NEXT_TABLE_DESC:
			SSDFS_INFO("FRAGMENT TYPE: SSDFS_NEXT_TABLE_DESC\n");
			break;

		default:
			SSDFS_INFO("FRAGMENT TYPE: UNKNOWN\n");
			break;
		}

		SSDFS_INFO("FRAGMENT FLAGS: ");

		if (flags & SSDFS_FRAGMENT_HAS_CSUM)
			SSDFS_INFO("SSDFS_FRAGMENT_HAS_CSUM ");

		if (flags == 0)
			SSDFS_INFO("NONE");

		SSDFS_INFO("\n");

		*parsed_bytes += frag_desc_size;

		if ((size - *parsed_bytes) < le16_to_cpu(frag->compr_size)) {
			SSDFS_ERR("size %u is lesser than %u\n",
				  size - *parsed_bytes,
				  le16_to_cpu(frag->compr_size));
			return -EINVAL;
		}

		data = (u8 *)area_buf + offset + *parsed_bytes;

		SSDFS_INFO("RAW DATA:\n");

		res = ssdfs_dumpfs_show_raw_string(0, data,
						le16_to_cpu(frag->compr_size));
		if (res < 0) {
			SSDFS_ERR("fail to dump raw data: size %u, err %d\n",
				  le16_to_cpu(frag->compr_size), res);
			return res;
		}

		*parsed_bytes += le16_to_cpu(frag->compr_size);
	}

	SSDFS_INFO("\n");


	return 0;
}

static
int ssdfs_dumpfs_parse_block_bitmap(void *area_buf, u32 area_size)
{
	struct ssdfs_block_bitmap_header *hdr =
			(struct ssdfs_block_bitmap_header *)area_buf;
	struct ssdfs_signature *magic = &hdr->magic;
	u8 *magic_common = (u8 *)&magic->common;
	u8 *magic_key = (u8 *)&magic->key;
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

	SSDFS_INFO("BLOCK BITMAP:\n");

	SSDFS_INFO("MAGIC: %c%c%c%c %c%c\n",
		   *magic_common, *(magic_common + 1),
		   *(magic_common + 2), *(magic_common + 3),
		   *magic_key, *(magic_key + 1));
	SSDFS_INFO("VERSION: v.%u.%u\n",
		   magic->version.major,
		   magic->version.minor);

	SSDFS_INFO("FRAGMENTS_COUNT: %u\n", fragments_count);
	SSDFS_INFO("BYTES_COUNT: %u bytes\n", bytes_count);

	SSDFS_INFO("BLOCK BITMAP FLAGS: ");

	if (flags & SSDFS_BLK_BMAP_BACKUP)
		SSDFS_INFO("SSDFS_BLK_BMAP_BACKUP ");

	if (flags & SSDFS_BLK_BMAP_COMPRESSED)
		SSDFS_INFO("SSDFS_BLK_BMAP_COMPRESSED ");

	if (flags == 0)
		SSDFS_INFO("NONE");

	SSDFS_INFO("\n");

	switch (type) {
	case SSDFS_BLK_BMAP_UNCOMPRESSED_BLOB:
		SSDFS_INFO("BLOCK BITMAP TYPE: SSDFS_BLK_BMAP_UNCOMPRESSED_BLOB\n");
		break;

	case SSDFS_BLK_BMAP_ZLIB_BLOB:
		SSDFS_INFO("BLOCK BITMAP TYPE: SSDFS_BLK_BMAP_ZLIB_BLOB\n");
		break;

	case SSDFS_BLK_BMAP_LZO_BLOB:
		SSDFS_INFO("BLOCK BITMAP TYPE: SSDFS_BLK_BMAP_LZO_BLOB\n");
		break;

	default:
		SSDFS_INFO("BLOCK BITMAP TYPE: UNKNOWN\n");
		break;
	}

	SSDFS_INFO("\n");

	offset = sizeof(struct ssdfs_block_bitmap_header);
	size = area_size - offset;

	for (i = 0; i < fragments_count; i++) {
		u32 parsed_bytes = 0;

		SSDFS_INFO("BLOCK BITMAP FRAGMENT: #%d\n", i);

		err = ssdfs_dumpfs_parse_block_bitmap_fragment(area_buf,
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
void ssdfs_dumpfs_parse_btree_descriptor(struct ssdfs_btree_descriptor *desc)
{
	u8 *magic = (u8 *)&desc->magic;
	u16 flags = le16_to_cpu(desc->flags);
	u32 node_size = 1 << desc->log_node_size;
	u16 index_size = le16_to_cpu(desc->index_size);
	u16 item_size = le16_to_cpu(desc->item_size);
	u16 index_area_min_size = le16_to_cpu(desc->index_area_min_size);

	SSDFS_INFO("B-TREE HEADER:\n");

	SSDFS_INFO("MAGIC: %c%c%c%c\n",
		   *magic, *(magic + 1),
		   *(magic + 2), *(magic + 3));

	SSDFS_INFO("B-TREE FLAGS: ");

	if (flags & SSDFS_BTREE_DESC_INDEX_AREA_RESIZABLE)
		SSDFS_INFO("SSDFS_BTREE_DESC_INDEX_AREA_RESIZABLE ");

	if (flags == 0)
		SSDFS_INFO("NONE");

	SSDFS_INFO("\n");

	switch (desc->type) {
	case SSDFS_INODES_BTREE:
		SSDFS_INFO("B-TREE TYPE: SSDFS_INODES_BTREE\n");
		break;

	case SSDFS_DENTRIES_BTREE:
		SSDFS_INFO("B-TREE TYPE: SSDFS_DENTRIES_BTREE\n");
		break;

	case SSDFS_EXTENTS_BTREE:
		SSDFS_INFO("B-TREE TYPE: SSDFS_EXTENTS_BTREE\n");
		break;

	case SSDFS_SHARED_EXTENTS_BTREE:
		SSDFS_INFO("B-TREE TYPE: SSDFS_SHARED_EXTENTS_BTREE\n");
		break;

	case SSDFS_XATTR_BTREE:
		SSDFS_INFO("B-TREE TYPE: SSDFS_XATTR_BTREE\n");
		break;

	case SSDFS_SHARED_XATTR_BTREE:
		SSDFS_INFO("B-TREE TYPE: SSDFS_SHARED_XATTR_BTREE\n");
		break;

	case SSDFS_SHARED_DICTIONARY_BTREE:
		SSDFS_INFO("B-TREE TYPE: SSDFS_SHARED_DICTIONARY_BTREE\n");
		break;

	default:
		SSDFS_INFO("B-TREE TYPE: UNKNOWN_BTREE_TYPE\n");
		break;
	}

	SSDFS_INFO("NODE_SIZE: %u bytes\n", node_size);
	SSDFS_INFO("PAGES_PER_NODE: %u\n", desc->pages_per_node);
	SSDFS_INFO("NODE_PTR_SIZE: %u bytes\n", desc->node_ptr_size);
	SSDFS_INFO("INDEX_SIZE: %u bytes\n", index_size);
	SSDFS_INFO("ITEM_SIZE: %u bytes\n", item_size);
	SSDFS_INFO("INDEX_AREA_MIN_SIZE: %u bytes\n", index_area_min_size);
}

static
void ssdfs_dumpfs_parse_dentries_btree_descriptor(struct ssdfs_dentries_btree_descriptor *tree)
{
	SSDFS_INFO("DENTRIES B-TREE HEADER:\n");

	ssdfs_dumpfs_parse_btree_descriptor(&tree->desc);
}

static
void ssdfs_dumpfs_parse_extents_btree_descriptor(struct ssdfs_extents_btree_descriptor *tree)
{
	SSDFS_INFO("EXTENTS B-TREE HEADER:\n");

	ssdfs_dumpfs_parse_btree_descriptor(&tree->desc);
}

static
void ssdfs_dumpfs_parse_xattr_btree_descriptor(struct ssdfs_xattr_btree_descriptor *tree)
{
	SSDFS_INFO("XATTRS B-TREE HEADER:\n");

	ssdfs_dumpfs_parse_btree_descriptor(&tree->desc);
}

static
void ssdfs_dumpfs_parse_maptbl_sb_header(struct ssdfs_segment_header *hdr)
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

	SSDFS_INFO("MAPPING TABLE HEADER:\n");

	SSDFS_INFO("FRAGMENTS_COUNT: %u\n", fragments_count);
	SSDFS_INFO("FRAGMENT_BYTES: %u\n", fragment_bytes);
	SSDFS_INFO("LAST_PEB_RECOVER_CNO: %llu\n", last_peb_recover_cno);
	SSDFS_INFO("LEBS_COUNT: %llu\n", lebs_count);
	SSDFS_INFO("PEBS_COUNT: %llu\n", pebs_count);
	SSDFS_INFO("FRAGMENTS_PER_SEGMENT: %u\n", fragments_per_seg);
	SSDFS_INFO("FRAGMENTS_PER_PEB: %u\n", fragments_per_peb);
	SSDFS_INFO("PRE_ERASE_PEBS: %u\n", pre_erase_pebs);
	SSDFS_INFO("LEBS_PER_FRAGMENT: %u\n", lebs_per_fragment);
	SSDFS_INFO("PEBS_PER_FRAGMENT: %u\n", pebs_per_fragment);
	SSDFS_INFO("PEBS_PER_STRIPE: %u\n", pebs_per_stripe);
	SSDFS_INFO("STRIPES_PER_FRAGMENT: %u\n", stripes_per_fragment);

	SSDFS_INFO("MAPPING TABLE FLAGS: ");

	if (flags & SSDFS_MAPTBL_HAS_COPY)
		SSDFS_INFO("SSDFS_MAPTBL_HAS_COPY ");

	if (flags & SSDFS_MAPTBL_ERROR)
		SSDFS_INFO("SSDFS_MAPTBL_ERROR ");

	if (flags & SSDFS_MAPTBL_MAKE_ZLIB_COMPR)
		SSDFS_INFO("SSDFS_MAPTBL_MAKE_ZLIB_COMPR ");

	if (flags & SSDFS_MAPTBL_MAKE_LZO_COMPR)
		SSDFS_INFO("SSDFS_MAPTBL_MAKE_LZO_COMPR ");

	if (flags == 0)
		SSDFS_INFO("NONE");

	SSDFS_INFO("\n");

	SSDFS_INFO("MAPPING TABLE EXTENTS:\n");

	for (i = 0; i < SSDFS_MAPTBL_RESERVED_EXTENTS; i++) {
		struct ssdfs_meta_area_extent *extent;

		extent = &maptbl->extents[i][SSDFS_MAIN_MAPTBL_SEG];

		SSDFS_INFO("extent[%d][MAIN]: start_id %llu, len %u, ",
			   i,
			   le64_to_cpu(extent->start_id),
			   le32_to_cpu(extent->len));

		switch (le16_to_cpu(extent->type)) {
		case SSDFS_EMPTY_EXTENT_TYPE:
			SSDFS_INFO("SSDFS_EMPTY_EXTENT_TYPE, ");
			break;

		case SSDFS_SEG_EXTENT_TYPE:
			SSDFS_INFO("SSDFS_SEG_EXTENT_TYPE, ");
			break;

		case SSDFS_PEB_EXTENT_TYPE:
			SSDFS_INFO("SSDFS_PEB_EXTENT_TYPE, ");
			break;

		case SSDFS_BLK_EXTENT_TYPE:
			SSDFS_INFO("SSDFS_BLK_EXTENT_TYPE, ");
			break;

		default:
			SSDFS_INFO("UNKNOWN_EXTENT_TYPE, ");
			break;
		}

		SSDFS_INFO("flags %#x\n",
			   le16_to_cpu(extent->flags));

		extent = &maptbl->extents[i][SSDFS_COPY_MAPTBL_SEG];

		SSDFS_INFO("extent[%d][COPY]: start_id %llu, len %u, ",
			   i,
			   le64_to_cpu(extent->start_id),
			   le32_to_cpu(extent->len));

		switch (le16_to_cpu(extent->type)) {
		case SSDFS_EMPTY_EXTENT_TYPE:
			SSDFS_INFO("SSDFS_EMPTY_EXTENT_TYPE, ");
			break;

		case SSDFS_SEG_EXTENT_TYPE:
			SSDFS_INFO("SSDFS_SEG_EXTENT_TYPE, ");
			break;

		case SSDFS_PEB_EXTENT_TYPE:
			SSDFS_INFO("SSDFS_PEB_EXTENT_TYPE, ");
			break;

		case SSDFS_BLK_EXTENT_TYPE:
			SSDFS_INFO("SSDFS_BLK_EXTENT_TYPE, ");
			break;

		default:
			SSDFS_INFO("UNKNOWN_EXTENT_TYPE, ");
			break;
		}

		SSDFS_INFO("flags %#x\n",
			   le16_to_cpu(extent->flags));
	}
}

static
void ssdfs_dumpfs_parse_segbmap_sb_header(struct ssdfs_segment_header *hdr)
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

	SSDFS_INFO("SEGMENT BITMAP HEADER:\n");

	SSDFS_INFO("FRAGMENTS_COUNT: %u\n", fragments_count);
	SSDFS_INFO("FRAGMENTS_PER_SEGMENT: %u\n", fragments_per_seg);
	SSDFS_INFO("FRAGMENTS_PER_PEB: %u\n", fragments_per_peb);
	SSDFS_INFO("FRAGMENTS_SIZE: %u bytes\n", fragment_size);
	SSDFS_INFO("BYTES_COUNT: %u bytes\n", bytes_count);
	SSDFS_INFO("SEGMENTS_COUNT: %u\n", segs_count);

	SSDFS_INFO("SEGMENT BITMAP FLAGS: ");

	if (flags & SSDFS_SEGBMAP_HAS_COPY)
		SSDFS_INFO("SSDFS_SEGBMAP_HAS_COPY ");

	if (flags & SSDFS_SEGBMAP_ERROR)
		SSDFS_INFO("SSDFS_SEGBMAP_ERROR ");

	if (flags & SSDFS_SEGBMAP_MAKE_ZLIB_COMPR)
		SSDFS_INFO("SSDFS_SEGBMAP_MAKE_ZLIB_COMPR ");

	if (flags & SSDFS_SEGBMAP_MAKE_LZO_COMPR)
		SSDFS_INFO("SSDFS_SEGBMAP_MAKE_LZO_COMPR ");

	if (flags == 0)
		SSDFS_INFO("NONE");

	SSDFS_INFO("\n");

	SSDFS_INFO("SEGMENT BITMAP SEGMENTS:\n");

	for (i = 0; i < SSDFS_SEGBMAP_SEGS; i++) {
		SSDFS_INFO("SEG[%d][MAIN]: %llu; SEG[%d][COPY]: %llu\n",
			   i,
			   le64_to_cpu(segbmap->segs[i][SSDFS_MAIN_SEGBMAP_SEG]),
			   i,
			   le64_to_cpu(segbmap->segs[i][SSDFS_COPY_SEGBMAP_SEG]));
	}
}

static
void ssdfs_dumpfs_parse_segment_header(struct ssdfs_segment_header *hdr)
{
	struct ssdfs_volume_header *vh = &hdr->volume_hdr;
	u8 *magic_common = (u8 *)&vh->magic.common;
	u8 *magic_key = (u8 *)&vh->magic.key;
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

	page_size = 1 << vh->log_pagesize;
	erase_size = 1 << vh->log_erasesize;
	seg_size = 1 << vh->log_segsize;
	create_time = le64_to_cpu(vh->create_time);
	create_cno = le64_to_cpu(vh->create_cno);
	seg_type = le16_to_cpu(hdr->seg_type);
	seg_flags = le32_to_cpu(hdr->seg_flags);

	SSDFS_INFO("MAGIC: %c%c%c%c %c%c\n",
		   *magic_common, *(magic_common + 1),
		   *(magic_common + 2), *(magic_common + 3),
		   *magic_key, *(magic_key + 1));
	SSDFS_INFO("VERSION: v.%u.%u\n",
		   vh->magic.version.major,
		   vh->magic.version.minor);
	SSDFS_INFO("PAGE: %u bytes\n", page_size);
	SSDFS_INFO("PEB: %u bytes\n", erase_size);
	SSDFS_INFO("PEBS_PER_SEGMENT: %u\n",
		   1 << vh->log_pebs_per_seg);
	SSDFS_INFO("SEGMENT: %u bytes\n", seg_size);

	SSDFS_INFO("CREATION_TIME: %s\n",
		   ssdfs_nanoseconds_to_time(create_time));
	SSDFS_INFO("CREATION_CHECKPOINT: %llu\n",
		   create_cno);

	SSDFS_INFO("\n");

	pair1 = &vh->sb_pebs[SSDFS_CUR_SB_SEG][SSDFS_MAIN_SB_SEG];
	pair2 = &vh->sb_pebs[SSDFS_CUR_SB_SEG][SSDFS_COPY_SB_SEG];
	SSDFS_INFO("CURRENT_SUPERBLOCK_SEGMENT: "
		   "MAIN: LEB %llu, PEB %llu; "
		   "COPY: LEB %llu, PEB %llu\n",
		   le64_to_cpu(pair1->leb_id), le64_to_cpu(pair1->peb_id),
		   le64_to_cpu(pair2->leb_id), le64_to_cpu(pair2->peb_id));

	pair1 = &vh->sb_pebs[SSDFS_NEXT_SB_SEG][SSDFS_MAIN_SB_SEG];
	pair2 = &vh->sb_pebs[SSDFS_NEXT_SB_SEG][SSDFS_COPY_SB_SEG];
	SSDFS_INFO("NEXT_SUPERBLOCK_SEGMENT: "
		   "MAIN: LEB %llu, PEB %llu; "
		   "COPY: LEB %llu, PEB %llu\n",
		   le64_to_cpu(pair1->leb_id), le64_to_cpu(pair1->peb_id),
		   le64_to_cpu(pair2->leb_id), le64_to_cpu(pair2->peb_id));

	pair1 = &vh->sb_pebs[SSDFS_RESERVED_SB_SEG][SSDFS_MAIN_SB_SEG];
	pair2 = &vh->sb_pebs[SSDFS_RESERVED_SB_SEG][SSDFS_COPY_SB_SEG];
	SSDFS_INFO("RESERVED_SUPERBLOCK_SEGMENT: "
		   "MAIN: LEB %llu, PEB %llu; "
		   "COPY: LEB %llu, PEB %llu\n",
		   le64_to_cpu(pair1->leb_id), le64_to_cpu(pair1->peb_id),
		   le64_to_cpu(pair2->leb_id), le64_to_cpu(pair2->peb_id));

	pair1 = &vh->sb_pebs[SSDFS_PREV_SB_SEG][SSDFS_MAIN_SB_SEG];
	pair2 = &vh->sb_pebs[SSDFS_PREV_SB_SEG][SSDFS_COPY_SB_SEG];
	SSDFS_INFO("PREVIOUS_SUPERBLOCK_SEGMENT: "
		   "MAIN: LEB %llu, PEB %llu; "
		   "COPY: LEB %llu, PEB %llu\n",
		   le64_to_cpu(pair1->leb_id), le64_to_cpu(pair1->peb_id),
		   le64_to_cpu(pair2->leb_id), le64_to_cpu(pair2->peb_id));

	SSDFS_INFO("\n");

	SSDFS_INFO("SB_SEGMENT_LOG_PAGES: %u\n",
		   le16_to_cpu(vh->sb_seg_log_pages));
	SSDFS_INFO("SEGBMAP_LOG_PAGES: %u\n",
		   le16_to_cpu(vh->segbmap_log_pages));
	SSDFS_INFO("MAPTBL_LOG_PAGES: %u\n",
		   le16_to_cpu(vh->maptbl_log_pages));
	SSDFS_INFO("USER_DATA_LOG_PAGES: %u\n",
		   le16_to_cpu(vh->user_data_log_pages));

	SSDFS_INFO("\n");

	SSDFS_INFO("LOG_CREATION_TIME: %s\n",
		   ssdfs_nanoseconds_to_time(le64_to_cpu(hdr->timestamp)));
	SSDFS_INFO("LOG_CREATION_CHECKPOINT: %llu\n",
		   le64_to_cpu(hdr->cno));
	SSDFS_INFO("LOG_PAGES: %u\n",
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

	SSDFS_INFO("SEG_TYPE: %s\n", seg_type_str);

	SSDFS_INFO("SEG_FLAGS: ");

	if (seg_flags & SSDFS_SEG_HDR_HAS_BLK_BMAP)
		SSDFS_INFO("SEG_HDR_HAS_BLK_BMAP ");

	if (seg_flags & SSDFS_SEG_HDR_HAS_OFFSET_TABLE)
		SSDFS_INFO("SEG_HDR_HAS_OFFSET_TABLE ");

	if (seg_flags & SSDFS_LOG_HAS_COLD_PAYLOAD)
		SSDFS_INFO("LOG_HAS_COLD_PAYLOAD ");

	if (seg_flags & SSDFS_LOG_HAS_WARM_PAYLOAD)
		SSDFS_INFO("LOG_HAS_WARM_PAYLOAD ");

	if (seg_flags & SSDFS_LOG_HAS_HOT_PAYLOAD)
		SSDFS_INFO("LOG_HAS_HOT_PAYLOAD ");

	if (seg_flags & SSDFS_LOG_HAS_BLK_DESC_CHAIN)
		SSDFS_INFO("LOG_HAS_BLK_DESC_CHAIN ");

	if (seg_flags & SSDFS_LOG_HAS_MAPTBL_CACHE)
		SSDFS_INFO("LOG_HAS_MAPTBL_CACHE ");

	if (seg_flags & SSDFS_LOG_HAS_FOOTER)
		SSDFS_INFO("LOG_HAS_FOOTER ");

	if (seg_flags & SSDFS_LOG_IS_PARTIAL)
		SSDFS_INFO("LOG_IS_PARTIAL ");

	SSDFS_INFO("\n");

	desc = &hdr->desc_array[SSDFS_BLK_BMAP_INDEX];
	SSDFS_INFO("BLOCK_BITMAP: offset %u, size %u\n",
		   le32_to_cpu(desc->offset),
		   le32_to_cpu(desc->size));

	desc = &hdr->desc_array[SSDFS_OFF_TABLE_INDEX];
	SSDFS_INFO("OFFSETS_TABLE: offset %u, size %u\n",
		   le32_to_cpu(desc->offset),
		   le32_to_cpu(desc->size));

	desc = &hdr->desc_array[SSDFS_COLD_PAYLOAD_AREA_INDEX];
	SSDFS_INFO("COLD_PAYLOAD_AREA: offset %u, size %u\n",
		   le32_to_cpu(desc->offset),
		   le32_to_cpu(desc->size));

	desc = &hdr->desc_array[SSDFS_WARM_PAYLOAD_AREA_INDEX];
	SSDFS_INFO("WARM_PAYLOAD_AREA: offset %u, size %u\n",
		   le32_to_cpu(desc->offset),
		   le32_to_cpu(desc->size));

	desc = &hdr->desc_array[SSDFS_HOT_PAYLOAD_AREA_INDEX];
	SSDFS_INFO("HOT_PAYLOAD_AREA: offset %u, size %u\n",
		   le32_to_cpu(desc->offset),
		   le32_to_cpu(desc->size));

	desc = &hdr->desc_array[SSDFS_BLK_DESC_AREA_INDEX];
	SSDFS_INFO("BLOCK_DESCRIPTOR_AREA: offset %u, size %u\n",
		   le32_to_cpu(desc->offset),
		   le32_to_cpu(desc->size));

	desc = &hdr->desc_array[SSDFS_MAPTBL_CACHE_INDEX];
	SSDFS_INFO("MAPTBL_CACHE_AREA: offset %u, size %u\n",
		   le32_to_cpu(desc->offset),
		   le32_to_cpu(desc->size));

	desc = &hdr->desc_array[SSDFS_LOG_FOOTER_INDEX];
	SSDFS_INFO("LOG_FOOTER: offset %u, size %u\n",
		   le32_to_cpu(desc->offset),
		   le32_to_cpu(desc->size));

	SSDFS_INFO("\n");

	SSDFS_INFO("PREV_MIGRATING_PEB: migration_id %u\n",
		   hdr->peb_migration_id[SSDFS_PREV_MIGRATING_PEB]);
	SSDFS_INFO("CUR_MIGRATING_PEB: migration_id %u\n",
		   hdr->peb_migration_id[SSDFS_CUR_MIGRATING_PEB]);

	SSDFS_INFO("\n");

	ssdfs_dumpfs_parse_segbmap_sb_header(hdr);

	SSDFS_INFO("\n");

	ssdfs_dumpfs_parse_maptbl_sb_header(hdr);

	SSDFS_INFO("\n");

	ssdfs_dumpfs_parse_dentries_btree_descriptor(&vh->dentries_btree);

	SSDFS_INFO("\n");

	ssdfs_dumpfs_parse_extents_btree_descriptor(&vh->extents_btree);

	SSDFS_INFO("\n");

	ssdfs_dumpfs_parse_xattr_btree_descriptor(&vh->xattr_btree);
}

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

int ssdfs_dumpfs_show_peb_dump(struct ssdfs_dumpfs_environment *env)
{
	struct ssdfs_segment_header sg_buf;
	struct ssdfs_signature *magic;
	struct ssdfs_metadata_descriptor *desc;
	void *area_buf = NULL;
	u64 offset;
	u32 area_offset;
	u32 area_size;
	int step = 2;
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

	if (env->peb.size == U32_MAX) {
		SSDFS_DUMPFS_INFO(env->base.show_info,
				  "[00%d]\tFIND FIRST VALID PEB...\n",
				  step);

		err = ssdfs_dumpfs_find_any_valid_peb(env, &sg_buf);
		if (err) {
			SSDFS_INFO("PLEASE, DEFINE PEB SIZE\n");
			print_usage();
			goto finish_peb_dump;
		}

		SSDFS_DUMPFS_INFO(env->base.show_info,
				  "[00%d]\t[SUCCESS]\n",
				  step);

		step++;
		env->peb.size = 1 << sg_buf.volume_hdr.log_erasesize;
	}

	SSDFS_DUMPFS_INFO(env->base.show_info,
			  "[00%d]\tDUMP PEB...\n",
			  step);

	err = ssdfs_dumpfs_read_segment_header(env, env->peb.id,
						env->peb.size,
						0, env->peb.size,
						&sg_buf);
	if (err) {
		SSDFS_ERR("fail to read PEB's header: "
			  "peb_id %llu, peb_size %u, err %d\n",
			  env->peb.id, env->peb.size, err);
		goto finish_peb_dump;
	}

	magic = &sg_buf.volume_hdr.magic;

	if (le32_to_cpu(magic->common) != SSDFS_SUPER_MAGIC ||
	    le16_to_cpu(magic->key) != SSDFS_SEGMENT_HDR_MAGIC) {
		if (env->peb.log_size == U32_MAX) {
			err = -EINVAL;
			SSDFS_INFO("PLEASE, DEFINE LOG SIZE\n");
			print_usage();
			goto finish_peb_dump;
		}
	} else {
		env->peb.log_size = le16_to_cpu(sg_buf.log_pages);
		env->peb.log_size <<= sg_buf.volume_hdr.log_pagesize;
	}

	if (env->peb.show_all_logs) {
		offset = env->peb.id * env->peb.size;
	} else {
		offset = env->peb.id * env->peb.size;

		if (env->peb.log_index == U16_MAX) {
			err = -EINVAL;
			SSDFS_INFO("PLEASE, DEFINE LOG INDEX\n");
			print_usage();
			goto finish_peb_dump;
		}

		if ((env->peb.log_index * env->peb.log_size) >= env->peb.size) {
			err = -EINVAL;
			SSDFS_ERR("log_index %u is huge\n",
				  env->peb.log_index);
			goto finish_peb_dump;
		}

		offset += env->peb.log_index * env->peb.log_size;
	}

	err = ssdfs_dumpfs_read_segment_header(env, env->peb.id,
						env->peb.size,
						env->peb.log_index,
						env->peb.log_size,
						&sg_buf);
	if (err) {
		SSDFS_ERR("fail to read PEB's header: "
			  "peb_id %llu, peb_size %u, "
			  "log_index %u, err %d\n",
			  env->peb.id, env->peb.size,
			  env->peb.log_index, err);
		goto finish_peb_dump;
	}

	if (le32_to_cpu(magic->common) == SSDFS_SUPER_MAGIC ||
	    le16_to_cpu(magic->key) == SSDFS_SEGMENT_HDR_MAGIC) {
		SSDFS_INFO("PEB_ID %llu, LOG_INDEX %u\n\n",
			   env->peb.id, env->peb.log_index);

		ssdfs_dumpfs_parse_segment_header(&sg_buf);

		SSDFS_INFO("\n");

		if (env->is_raw_dump_requested) {
			env->raw_dump.offset = offset;
			env->raw_dump.size =
				sizeof(struct ssdfs_segment_header);

			err = ssdfs_dumpfs_show_raw_dump(env);
			if (err) {
				SSDFS_ERR("fail to make segment header dump: "
					  "peb_id %llu, err %d\n",
					  env->peb.id, err);
				goto finish_peb_dump;
			}

			SSDFS_INFO("\n");
		}

		desc = &sg_buf.desc_array[SSDFS_BLK_BMAP_INDEX];
		area_offset = le32_to_cpu(desc->offset);
		area_size = le32_to_cpu(desc->size);

		if (is_ssdfs_dumpfs_area_valid(desc)) {
			area_buf = malloc(area_size);
			if (!area_buf) {
				err = -ENOMEM;
				SSDFS_ERR("fail to allocate memory: "
					  "size %u\n", area_size);
				goto finish_peb_dump;
			}

			memset(area_buf, 0, area_size);

			err = ssdfs_dumpfs_read_block_bitmap(env,
							     env->peb.id,
							     env->peb.size,
							     env->peb.log_index,
							     env->peb.log_size,
							     area_offset,
							     area_size,
							     area_buf);
			if (err) {
				SSDFS_ERR("fail to read block bitmap: "
					  "peb_id %llu, peb_size %u, "
					  "log_index %u, err %d\n",
					  env->peb.id, env->peb.size,
					  env->peb.log_index, err);
				goto finish_parse_block_bitmap;
			}

			err = ssdfs_dumpfs_parse_block_bitmap(area_buf, area_size);
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
				goto finish_peb_dump;

			SSDFS_INFO("\n");

			if (env->is_raw_dump_requested) {
				env->raw_dump.offset = offset + area_offset;
				env->raw_dump.size = area_size;

				err = ssdfs_dumpfs_show_raw_dump(env);
				if (err) {
					SSDFS_ERR("fail to make block bitmap "
						  "raw dump: "
						  "peb_id %llu, err %d\n",
						  env->peb.id, err);
					goto finish_peb_dump;
				}

				SSDFS_INFO("\n");
			}
		}

		desc = &sg_buf.desc_array[SSDFS_OFF_TABLE_INDEX];
		area_offset = le32_to_cpu(desc->offset);
		area_size = le32_to_cpu(desc->size);

		if (is_ssdfs_dumpfs_area_valid(desc)) {
			area_buf = malloc(area_size);
			if (!area_buf) {
				err = -ENOMEM;
				SSDFS_ERR("fail to allocate memory: "
					  "size %u\n", area_size);
				goto finish_peb_dump;
			}

			memset(area_buf, 0, area_size);

			err = ssdfs_dumpfs_read_blk2off_table(env,
							     env->peb.id,
							     env->peb.size,
							     env->peb.log_index,
							     env->peb.log_size,
							     area_offset,
							     area_size,
							     area_buf);
			if (err) {
				SSDFS_ERR("fail to read blk2off table: "
					  "peb_id %llu, peb_size %u, "
					  "log_index %u, err %d\n",
					  env->peb.id, env->peb.size,
					  env->peb.log_index, err);
				goto finish_parse_blk2off_table;
			}

			err = ssdfs_dumpfs_parse_blk2off_table(area_buf,
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
				goto finish_peb_dump;

			SSDFS_INFO("\n");

			if (env->is_raw_dump_requested) {
				env->raw_dump.offset = offset + area_offset;
				env->raw_dump.size = area_size;

				err = ssdfs_dumpfs_show_raw_dump(env);
				if (err) {
					SSDFS_ERR("fail to make blk2off table "
						  "raw dump: "
						  "peb_id %llu, err %d\n",
						  env->peb.id, err);
					goto finish_peb_dump;
				}

				SSDFS_INFO("\n");
			}
		}


	} else {
		SSDFS_INFO("PEB IS EMPTY\n");
	}

	SSDFS_DUMPFS_INFO(env->base.show_info,
			  "[00%d]\t[SUCCESS]\n",
			  step);
	step++;

finish_peb_dump:
	return err;
}
