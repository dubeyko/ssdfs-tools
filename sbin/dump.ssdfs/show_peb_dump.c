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
 * Authors: Vyacheslav Dubeyko <Vyacheslav.Dubeyko@wdc.com>
 *
 * Acknowledgement: Cyril Guyot <Cyril.Guyot@wdc.com>
 *                  Zvonimir Bandic <Zvonimir.Bandic@wdc.com>
 */

#include "dumpfs.h"

/************************************************************************
 *                     Show PEB dump command                            *
 ************************************************************************/

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
}

int ssdfs_dumpfs_show_peb_dump(struct ssdfs_dumpfs_environment *env)
{
	struct ssdfs_segment_header sg_buf;
	struct ssdfs_signature *magic;
	u64 offset;
	u32 size;
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
		size = env->peb.size;
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
		size = env->peb.log_size;
	}

	if (env->is_raw_dump_requested) {
		env->raw_dump.offset = offset;
		env->raw_dump.size = size;

		err = ssdfs_dumpfs_show_raw_dump(env);
		if (err) {
			SSDFS_ERR("fail to make raw dump of PEB: "
				  "id %llu, err %d\n",
				  env->peb.id, err);
			goto finish_peb_dump;
		}
	}

	SSDFS_DUMPFS_INFO(env->base.show_info,
			  "[00%d]\t[SUCCESS]\n",
			  step);
	step++;


	SSDFS_ERR("TODO: implement operation...\n");

finish_peb_dump:
	return err;
}
