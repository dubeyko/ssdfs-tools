//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/dump.ssdfs/common.c - common dumpfs primitives.
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

#define SSDFS_DUMPFS_PEB_SEARCH_SHIFT	(1)

int ssdfs_dumpfs_read_block_bitmap(struct ssdfs_dumpfs_environment *env,
				   u64 peb_id, u32 peb_size,
				   u32 area_offset, u32 size,
				   void *buf)
{
	u64 offset = SSDFS_RESERVED_VBR_SIZE;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "peb_id: %llu, peb_size %u\n",
		  peb_id, peb_size);

	if (peb_id != SSDFS_INITIAL_SNAPSHOT_SEG)
		offset = peb_id * peb_size;

	offset += area_offset;

	err = env->base.dev_ops->read(env->base.fd, offset, size,
				      buf);
	if (err) {
		SSDFS_ERR("fail to read block bitmap: "
			  "offset %llu, err %d\n",
			  offset, err);
		return err;
	}

	return 0;
}

int ssdfs_dumpfs_read_segment_header(struct ssdfs_dumpfs_environment *env,
				     u64 peb_id, u32 peb_size,
				     struct ssdfs_segment_header *hdr)
{
	size_t sg_size = sizeof(struct ssdfs_segment_header);
	u64 offset = SSDFS_RESERVED_VBR_SIZE;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "peb_id: %llu, peb_size %u\n",
		  peb_id, peb_size);

	if (peb_id != SSDFS_INITIAL_SNAPSHOT_SEG)
		offset = peb_id * peb_size;

	err = env->base.dev_ops->read(env->base.fd, offset, sg_size,
				      hdr);
	if (err) {
		SSDFS_ERR("fail to read segment header: "
			  "offset %llu, err %d\n",
			  offset, err);
		return err;
	}

	return 0;
}

int ssdfs_dumpfs_find_any_valid_peb(struct ssdfs_dumpfs_environment *env,
				    struct ssdfs_segment_header *hdr)
{
	size_t sg_size = sizeof(struct ssdfs_segment_header);
	u64 offset = SSDFS_RESERVED_VBR_SIZE;
	u32 peb_size = env->base.erase_size;
	u64 factor = 1;
	int err = -ENODATA;

	SSDFS_DBG(env->base.show_debug,
		  "command: %#x\n",
		  env->command);

	do {
		struct ssdfs_signature *magic = NULL;
		struct ssdfs_metadata_check *check = NULL;

		SSDFS_DBG(env->base.show_debug,
			  "try to read the offset %llu\n",
			  offset);

		err = ssdfs_dumpfs_read_segment_header(env,
							offset / peb_size,
							peb_size,
							hdr);
		if (err) {
			SSDFS_ERR("fail to read segment header: "
				  "offset %llu, err %d\n",
				  offset, err);
			return err;
		}

		magic = &hdr->volume_hdr.magic;
		check = &hdr->volume_hdr.check;

		if (le32_to_cpu(magic->common) == SSDFS_SUPER_MAGIC &&
		    le16_to_cpu(magic->key) == SSDFS_SEGMENT_HDR_MAGIC) {
			if (!is_csum_valid(check, hdr, sg_size))
				err = -ENODATA;
		} else
			err = -ENODATA;

		if (!err)
			break;

		if (offset == SSDFS_RESERVED_VBR_SIZE)
			offset = env->base.erase_size;
		else {
			factor <<= SSDFS_DUMPFS_PEB_SEARCH_SHIFT;
			offset += factor * env->base.erase_size;
		}
	} while (offset < env->base.fs_size);

	if (err) {
		SSDFS_ERR("SSDFS isn't found on the device %s\n",
			  env->base.dev_name);
		return err;
	}

	return 0;
}

void ssdfs_dumpfs_show_key_volume_details(struct ssdfs_dumpfs_environment *env,
					  struct ssdfs_segment_header *hdr)
{
	u32 page_size;
	u32 erase_size;
	u32 seg_size;
	u64 create_time;

	page_size = 1 << hdr->volume_hdr.log_pagesize;
	erase_size = 1 << hdr->volume_hdr.log_erasesize;
	seg_size = 1 << hdr->volume_hdr.log_segsize;
	create_time = le64_to_cpu(hdr->volume_hdr.create_time);

	SSDFS_INFO("\n");
	SSDFS_INFO("SSDFS v.%u.%u\n",
		    hdr->volume_hdr.magic.version.major,
		    hdr->volume_hdr.magic.version.minor);
	SSDFS_INFO("PAGE: %u bytes\n", page_size);
	SSDFS_INFO("PEB: %u bytes\n", erase_size);
	SSDFS_INFO("PEBS_PER_SEGMENT: %u\n",
		   1 << hdr->volume_hdr.log_pebs_per_seg);
	SSDFS_INFO("SEGMENT: %u bytes\n", seg_size);
	SSDFS_INFO("VOLUME_SIZE: %llu bytes\n", env->base.fs_size);
	SSDFS_INFO("PAGES_PER_VOLUME: %llu\n",
		   env->base.fs_size / page_size);
	SSDFS_INFO("PEBS_PER_VOLUME: %llu\n",
		   env->base.fs_size / erase_size);
	SSDFS_INFO("SEGMENTS_PER_VOLUME: %llu\n",
		   env->base.fs_size / seg_size);
	SSDFS_INFO("CREATION_TIME: %s\n",
		   ssdfs_nanoseconds_to_time(create_time));
	SSDFS_INFO("\n");
}
