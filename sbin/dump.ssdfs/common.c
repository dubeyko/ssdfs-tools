/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/dump.ssdfs/common.c - common dumpfs primitives.
 *
 * Copyright (c) 2014-2019 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 * Copyright (c) 2014-2026 Viacheslav Dubeyko <slava@dubeyko.com>
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

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "dumpfs.h"

#define SSDFS_DUMPFS_PEB_SEARCH_SHIFT	(1)

int ssdfs_dumpfs_open_file(struct ssdfs_dumpfs_environment *env,
			   char *file_name)
{
#define SSDFS_DUMPFS_PATH_LEN		(256)
	char buf[SSDFS_DUMPFS_PATH_LEN];
	struct stat st = {0};

	if (!env->dump_into_files)
		return 0;

	if (stat(env->output_folder, &st) == -1) {
		mkdir(env->output_folder, S_IRWXU | S_IRWXG | S_IRWXO);
	}

	memset(buf, 0, SSDFS_DUMPFS_PATH_LEN);

	if (env->output_folder == NULL) {
		if (file_name == NULL) {
			snprintf(buf, SSDFS_DUMPFS_PATH_LEN - 1,
				 "peb-%llu-log-%u-dump.txt",
				 env->peb.id,
				 env->peb.log_index);
		} else {
			snprintf(buf, SSDFS_DUMPFS_PATH_LEN - 1,
				 "%s", file_name);
		}
	} else {
		if (file_name == NULL) {
			snprintf(buf, SSDFS_DUMPFS_PATH_LEN - 1,
				 "%s/peb-%llu-log-%u-dump.txt",
				 env->output_folder,
				 env->peb.id,
				 env->peb.log_index);
		} else {
			snprintf(buf, SSDFS_DUMPFS_PATH_LEN - 1,
				 "%s/%s",
				 env->output_folder,
				 file_name);
		}
	}

	env->fd = creat(buf, S_IRUSR | S_IWUSR |
			     S_IRGRP | S_IWGRP |
			     S_IROTH | S_IWOTH);
	if (env->fd == -1) {
		SSDFS_ERR("unable to create %s: %s\n",
			  buf, strerror(errno));
		return errno;
	}

	env->stream = fdopen(env->fd, "w");
	if (env->stream == NULL) {
		SSDFS_ERR("fail to open file's stream: %s\n",
			  strerror(errno));
		return errno;
	}

	return 0;
}

void ssdfs_dumpfs_close_file(struct ssdfs_dumpfs_environment *env)
{
	if (!env->dump_into_files)
		return;

	fclose(env->stream);
	close(env->fd);
}

int ssdfs_dumpfs_read_blk_desc_array(struct ssdfs_dumpfs_environment *env,
				   u64 peb_id, u32 peb_size,
				   u32 log_offset, u32 log_size,
				   u32 area_offset, u32 size,
				   void *buf)
{
	u64 offset;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "peb_id: %llu, peb_size %u\n",
		  peb_id, peb_size);

	offset = peb_id * peb_size;
	offset += area_offset;

	err = env->base.dev_ops->read(env->base.fd, offset, size,
				      buf, env->base.show_debug);
	if (err) {
		SSDFS_ERR("fail to read block descriptors array: "
			  "offset %llu, err %d\n",
			  offset, err);
		return err;
	}

	return 0;
}

int ssdfs_dumpfs_read_blk2off_table(struct ssdfs_dumpfs_environment *env,
				   u64 peb_id, u32 peb_size,
				   u32 log_offset, u32 log_size,
				   u32 area_offset, u32 size,
				   void *buf)
{
	u64 offset;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "peb_id: %llu, peb_size %u\n",
		  peb_id, peb_size);

	offset = peb_id * peb_size;
	offset += area_offset;

	err = env->base.dev_ops->read(env->base.fd, offset, size,
				      buf, env->base.show_debug);
	if (err) {
		SSDFS_ERR("fail to read blk2off table: "
			  "offset %llu, err %d\n",
			  offset, err);
		return err;
	}

	return 0;
}

int ssdfs_dumpfs_read_block_bitmap(struct ssdfs_dumpfs_environment *env,
				   u64 peb_id, u32 peb_size,
				   u32 log_offset, u32 log_size,
				   u32 area_offset, u32 size,
				   void *buf)
{
	u64 offset;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "peb_id: %llu, peb_size %u\n",
		  peb_id, peb_size);

	offset = peb_id * peb_size;
	offset += area_offset;

	err = env->base.dev_ops->read(env->base.fd, offset, size,
				      buf, env->base.show_debug);
	if (err) {
		SSDFS_ERR("fail to read block bitmap: "
			  "offset %llu, err %d\n",
			  offset, err);
		return err;
	}

	return 0;
}

int ssdfs_dumpfs_read_maptbl_cache(struct ssdfs_dumpfs_environment *env,
				   u64 peb_id, u32 peb_size,
				   u32 log_offset, u32 log_size,
				   u32 area_offset, u32 size,
				   void *buf)
{
	u64 offset;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "peb_id: %llu, peb_size %u\n",
		  peb_id, peb_size);

	offset = peb_id * peb_size;
	offset += area_offset;

	err = env->base.dev_ops->read(env->base.fd, offset, size,
				      buf, env->base.show_debug);
	if (err) {
		SSDFS_ERR("fail to read mapping table cache: "
			  "offset %llu, err %d\n",
			  offset, err);
		return err;
	}

	return 0;
}

int ssdfs_dumpfs_read_logical_block(struct ssdfs_dumpfs_environment *env,
				    u64 peb_id, u32 peb_size,
				    u32 log_offset, u32 log_size,
				    u32 block_offset, u32 size,
				    void *buf)
{
	u64 offset;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "peb_id: %llu, peb_size %u\n",
		  peb_id, peb_size);

	offset = peb_id * peb_size;
	offset += block_offset;

	err = env->base.dev_ops->read(env->base.fd, offset, size,
				      buf, env->base.show_debug);
	if (err) {
		SSDFS_ERR("fail to read logical block: "
			  "offset %llu, err %d\n",
			  offset, err);
		return err;
	}

	return 0;
}

int ssdfs_dumpfs_read_log_footer(struct ssdfs_dumpfs_environment *env,
				   u64 peb_id, u32 peb_size,
				   u32 log_offset, u32 log_size,
				   u32 area_offset, u32 size,
				   void *buf)
{
	u64 offset;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "peb_id: %llu, peb_size %u\n",
		  peb_id, peb_size);

	offset = peb_id * peb_size;
	offset += area_offset;

	err = env->base.dev_ops->read(env->base.fd, offset, size,
				      buf, env->base.show_debug);
	if (err) {
		SSDFS_ERR("fail to read log footer: "
			  "offset %llu, err %d\n",
			  offset, err);
		return err;
	}

	return 0;
}

int ssdfs_dumpfs_read_partial_log_footer(struct ssdfs_dumpfs_environment *env,
					 u64 peb_id, u32 peb_size,
					 u32 log_offset, u32 log_size,
					 u32 area_offset, u32 size,
					 void *buf)
{
	u64 offset;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "peb_id: %llu, peb_size %u\n",
		  peb_id, peb_size);

	offset = peb_id * peb_size;
	offset += area_offset;

	err = env->base.dev_ops->read(env->base.fd, offset, size,
				      buf, env->base.show_debug);
	if (err) {
		SSDFS_ERR("fail to read partial log footer: "
			  "offset %llu, err %d\n",
			  offset, err);
		return err;
	}

	return 0;
}

int ssdfs_dumpfs_read_partial_log_header(struct ssdfs_dumpfs_environment *env,
					 u64 peb_id, u32 peb_size,
					 u32 log_offset, u32 size,
					 void *buf)
{
	u64 offset;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "peb_id: %llu, peb_size %u, "
		  "log_offset %u, size %u\n",
		  peb_id, peb_size,
		  log_offset, size);

	offset = peb_id * peb_size;
	offset += log_offset;

	err = env->base.dev_ops->read(env->base.fd, offset, size,
				      buf, env->base.show_debug);
	if (err) {
		SSDFS_ERR("fail to read partial log header: "
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

		err = ssdfs_read_segment_header(&env->base,
						offset / peb_size,
						peb_size,
						0, peb_size,
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
