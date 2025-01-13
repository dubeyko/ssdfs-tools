/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/recoverfs.ssdfs/peb_processing.c - implementation of erase block
 *                                         processing logic.
 *
 * Copyright (c) 2023-2025 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
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
#include <errno.h>
#include <pthread.h>
#include <dirent.h>

#include "recoverfs.h"

static inline
int ssdfs_recoverfs_create_folder(struct ssdfs_thread_state *state,
				  u64 timestamp)
{
	int err;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, "
		  "log_offset %u, timestamp %llu\n",
		  state->id, state->peb.id, state->peb.log_offset,
		  timestamp);

	memset(state->name_buf, 0, sizeof(state->name_buf));

	err = snprintf(state->name_buf, sizeof(state->name_buf) - 1,
			"%llu", timestamp);
	if (err < 0) {
		SSDFS_ERR("fail to convert %llu into string: %s\n",
			  timestamp, strerror(errno));
		return err;
	}

	err = mkdirat(state->output_folder.fd, state->name_buf, 0777);
	if (err < 0) {
		if (errno == EEXIST) {
			/* ignore error */
			err = 0;
		} else if (errno) {
			SSDFS_ERR("fail to create folder %s, "
				  "err %d, errno %d: %s\n",
				  state->name_buf, err,
				  errno, strerror(errno));
			return errno;
		}
	}

	state->checkpoint_folder.fd = openat(state->output_folder.fd,
					     state->name_buf,
					     O_DIRECTORY, 0777);
	if (state->checkpoint_folder.fd < 1) {
		SSDFS_ERR("unable to open %s: %s\n",
			  state->name_buf, strerror(errno));
		return errno;
	}

	return 0;
}

static
int ssdfs_recoverfs_find_valid_log(struct ssdfs_thread_state *state)
{
	struct ssdfs_segment_header *seg_hdr = NULL;
	struct ssdfs_signature *magic;
	u32 offset;
	int is_log_valid;
	u32 i;
	int err;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu\n",
		  state->id, state->peb.id);

	is_log_valid = SSDFS_FALSE;

	for (i = state->peb.log_index; i < state->peb.logs_count; i++) {
		offset = i * SSDFS_4KB;

		err = ssdfs_read_segment_header(&state->base,
					state->peb.id,
					state->peb.peb_size,
					offset,
					state->raw_dump.seg_hdr.buffer.size,
					state->raw_dump.seg_hdr.buffer.ptr);
		if (err) {
			SSDFS_ERR("fail to read PEB's header: "
				  "peb_id %llu, peb_size %u, err %d\n",
				  state->peb.id, state->peb.peb_size, err);
			return err;
		}

		seg_hdr = SSDFS_SEG_HDR(state->raw_dump.seg_hdr.buffer.ptr);
		magic = &seg_hdr->volume_hdr.magic;

		if (le32_to_cpu(magic->common) == SSDFS_SUPER_MAGIC) {
			is_log_valid = SSDFS_TRUE;

			state->raw_dump.seg_hdr.area.offset = offset;

			state->peb.log_offset = offset;
			state->peb.log_size = SSDFS_4KB;
			state->peb.log_index = i;
			break;
		}
	}

	if (!is_log_valid) {
		SSDFS_DBG(state->base.show_debug,
			  "PEB %llu has none valid log\n",
			  state->peb.id);
		return -ENODATA;
	}

	return 0;
}

static
int ssdfs_recoverfs_prepare_raw_buffer(struct ssdfs_raw_buffer *buf,
					u32 buf_size)
{
	if (buf->ptr == NULL) {
		buf->ptr = calloc(1, buf_size);
		if (!buf->ptr) {
			SSDFS_ERR("fail to allocate buffer: "
				  "size %u, err: %s\n",
				  buf_size,
				  strerror(errno));
			return errno;
		}
		buf->size = buf_size;
	} else if (buf_size > buf->size) {
		buf->ptr = realloc(buf->ptr, buf_size);
		if (!buf->ptr) {
			SSDFS_ERR("fail to re-allocate buffer: "
				  "size %u, err: %s\n",
				  buf_size,
				  strerror(errno));
			return errno;
		}
		buf->size = buf_size;
	}

	memset(buf->ptr, 0, buf->size);

	return 0;
}

static
int ssdfs_recoverfs_prefetch_blk_desc_table(struct ssdfs_thread_state *state)
{
	struct ssdfs_raw_dump_environment *dump_env;
	struct ssdfs_signature *magic;
	struct ssdfs_segment_header *seg_hdr = NULL;
	struct ssdfs_volume_header *vh;
	struct ssdfs_partial_log_header *pl_hdr = NULL;
	struct ssdfs_metadata_descriptor *desc;
	struct ssdfs_raw_buffer *area_buf;
	u32 offset = U32_MAX;
	u32 size = 0;
	u32 pagesize;
	int err;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u\n",
		  state->id, state->peb.id, state->peb.log_offset);

	dump_env = &state->raw_dump;

	seg_hdr = SSDFS_SEG_HDR(SSDFS_RAW_SEG_HDR(dump_env)->ptr);
	magic = &seg_hdr->volume_hdr.magic;

	if (le32_to_cpu(magic->common) == SSDFS_SUPER_MAGIC &&
	    le16_to_cpu(magic->key) == SSDFS_SEGMENT_HDR_MAGIC) {
		/* full log */

		vh = &seg_hdr->volume_hdr;
		pagesize = 1 << vh->log_pagesize;

		state->peb.log_size =
			(u32)le16_to_cpu(seg_hdr->log_pages) * pagesize;

		desc = &seg_hdr->desc_array[SSDFS_BLK_DESC_AREA_INDEX];

		offset = le32_to_cpu(desc->offset);
		size = le32_to_cpu(desc->size);
	} else if (le32_to_cpu(magic->common) == SSDFS_SUPER_MAGIC &&
		   le16_to_cpu(magic->key) == SSDFS_PARTIAL_LOG_HDR_MAGIC) {
		/* partial log */

		pl_hdr = SSDFS_PL_HDR(SSDFS_RAW_SEG_HDR(dump_env)->ptr);

		state->peb.log_size = le32_to_cpu(pl_hdr->log_bytes);

		desc = &pl_hdr->desc_array[SSDFS_BLK_DESC_AREA_INDEX];

		offset = le32_to_cpu(desc->offset);
		size = le32_to_cpu(desc->size);
	} else {
		SSDFS_ERR("unexpected state: "
			  "PEB %llu, offset %u\n",
			  state->peb.id, state->peb.log_offset);
		return -ERANGE;
	}

	area_buf = SSDFS_COMPR_CONTENT(dump_env, SSDFS_BLK_DESC_AREA_INDEX);

	err = ssdfs_recoverfs_prepare_raw_buffer(area_buf, size);
	if (err) {
		SSDFS_ERR("fail to prepare raw buffer: "
			  "size %u, err %d\n",
			  size, err);
		return err;
	}

	err = ssdfs_read_blk_desc_array(&state->base,
					state->peb.id,
					state->peb.peb_size,
					offset, size,
					area_buf->ptr);
	if (err) {
		SSDFS_ERR("fail to read block descriptors: "
			  "peb_id %llu, peb_size %u, "
			  "log_index %u, log_offset %u, "
			  "offset %u, err %d\n",
			  state->peb.id, state->base.erase_size,
			  state->peb.log_index,
			  state->peb.log_offset,
			  offset, err);
		return err;
	}

	return 0;
}

static inline
u32 ssdfs_recoverfs_define_next_log_index(struct ssdfs_thread_state *state,
					  u32 latest_area_offset,
					  u32 latest_area_size)
{
	u32 next_log_index = state->peb.log_index + 1;

	if (latest_area_offset > 0 && latest_area_size > 0) {
		state->peb.log_size = latest_area_offset + latest_area_size;

		if (state->peb.log_size > state->peb.log_offset)
			state->peb.log_size -= state->peb.log_offset;
		else
			state->peb.log_size = SSDFS_4KB;

		next_log_index = state->peb.log_size + SSDFS_4KB - 1;
		next_log_index /= SSDFS_4KB;
		next_log_index += state->peb.log_index;
	}

	return next_log_index;
}

static inline
void ssdfs_read_portion_header(struct ssdfs_thread_state *state,
				struct ssdfs_raw_area_environment *env,
				u32 offset)
{
	u8 *src_ptr;
	u8 *dst_ptr;
	size_t tbl_hdr_size = sizeof(struct ssdfs_area_block_table);

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u\n",
		  state->id, state->peb.id, state->peb.log_offset);

	src_ptr = env->buffer.ptr;
	src_ptr += offset;
	dst_ptr = &env->area.content.metadata.raw_buffer;

	memcpy(dst_ptr, src_ptr, tbl_hdr_size);
}

static
int ssdfs_copy_blk_desc_fragment(struct ssdfs_thread_state *state,
				 struct ssdfs_raw_area_environment *env,
				 int fragment_index)
{
	struct ssdfs_area_block_table *tbl;
	struct ssdfs_fragments_chain_header *chain_hdr;
	struct ssdfs_fragment_desc *frag_desc = NULL;
	struct ssdfs_raw_area *area;
	u8 *src_ptr;
	u8 *dst_ptr;
	size_t tbl_hdr_size = sizeof(struct ssdfs_area_block_table);
	size_t item_size = sizeof(struct ssdfs_block_descriptor);
	u32 offset;
	u32 compr_size;
	u32 uncompr_size;
	__le32 csum;
	int err;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u\n",
		  state->id, state->peb.id, state->peb.log_offset);

	area = &env->area;
	tbl = SSDFS_CONTENT_BLK_DESC_HDR(area);
	chain_hdr = &tbl->chain_hdr;

	if (chain_hdr->magic != SSDFS_CHAIN_HDR_MAGIC) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted chain header: invalid magic\n");
		return -EIO;
	}

	switch (chain_hdr->type) {
	case SSDFS_BLK_DESC_CHAIN_HDR:
	case SSDFS_BLK_DESC_ZLIB_CHAIN_HDR:
		/* expected chain header type */
		break;

	case SSDFS_BLK_DESC_LZO_CHAIN_HDR:
		SSDFS_ERR("unexpected chain header type %#x\n",
			  chain_hdr->type);
		return -EINVAL;

	default:
		SSDFS_ERR("unknown chain header type %#x\n",
			  chain_hdr->type);
		return -ERANGE;
	}

	if (fragment_index >= SSDFS_BLK_TABLE_MAX) {
		SSDFS_ERR("out of range: "
			  "fragment_index %d, threshold %u\n",
			  fragment_index, SSDFS_BLK_TABLE_MAX);
		return -ERANGE;
	}

	frag_desc = &tbl->blk[fragment_index];

	if (frag_desc == NULL) {
		SSDFS_ERR("empty fragment decriptor pointer\n");
		return -ENODATA;
	}

	if (frag_desc->magic != SSDFS_FRAGMENT_DESC_MAGIC) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted fragment descriptor: "
			  "invalid magic\n");
		return -ENODATA;
	}

	offset = le32_to_cpu(frag_desc->offset);
	compr_size = le16_to_cpu(frag_desc->compr_size);
	uncompr_size = le16_to_cpu(frag_desc->uncompr_size);

	if ((offset + compr_size) > env->buffer.size) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted fragment descriptor: "
			  "fragment (offset %u, size %u), "
			  "buffer (size %u)\n",
			  offset, compr_size,
			  env->buffer.size);
		return -EIO;
	}

	err = ssdfs_recoverfs_prepare_raw_buffer(SSDFS_CONTENT_BUFFER(area),
						 uncompr_size);
	if (err) {
		SSDFS_ERR("fail to prepare content buffer: "
			  "size %u, err %d\n",
			  uncompr_size, err);
		return err;
	}

	switch (frag_desc->type) {
	case SSDFS_DATA_BLK_DESC:
		if (compr_size != uncompr_size) {
			SSDFS_DBG(state->base.show_debug,
				  "corrupted fragment descriptor: "
				  "compr_size %u != uncompr_size %u\n",
				  compr_size, uncompr_size);
		}

		src_ptr = env->buffer.ptr + offset;
		dst_ptr = SSDFS_CONTENT_BUFFER(area)->ptr;
		memcpy(dst_ptr, src_ptr, compr_size);
		break;

	case SSDFS_DATA_BLK_DESC_ZLIB:
		src_ptr = env->buffer.ptr + offset;
		dst_ptr = SSDFS_CONTENT_BUFFER(area)->ptr;

		err = ssdfs_zlib_decompress(src_ptr, dst_ptr,
					    compr_size, uncompr_size,
					    state->base.show_debug);
		if (err) {
			SSDFS_ERR("fail to decompress: err %d\n", err);
			return err;
		}
		break;

	case SSDFS_DATA_BLK_DESC_LZO:
		SSDFS_ERR("TODO: implement LZO support\n");
		return -EOPNOTSUPP;
		break;

	default:
		SSDFS_ERR("unknown fragment type %#x\n",
			  frag_desc->type);
		return -ERANGE;
	}

	if (frag_desc->flags & SSDFS_FRAGMENT_HAS_CSUM) {
		csum = ssdfs_crc32_le(dst_ptr, uncompr_size);
		if (csum != frag_desc->checksum) {
			SSDFS_DBG(state->base.show_debug,
				  "corrupted fragment: "
				  "checksum1 %#x != checksum2 %#x\n",
				  le32_to_cpu(csum),
				  le32_to_cpu(frag_desc->checksum));
			return -EIO;
		}
	}

	SSDFS_INIT_CONTENT_ITERATOR(SSDFS_CONTENT_ITER(area),
				    tbl_hdr_size,
				    le32_to_cpu(chain_hdr->uncompr_bytes),
				    fragment_index,
				    uncompr_size,
				    0,
				    item_size);

	return 0;
}

static
int ssdfs_extract_valid_blk_desc_fragment(struct ssdfs_thread_state *state,
					  struct ssdfs_raw_area_environment *env,
					  int start_fragment)
{
	struct ssdfs_area_block_table *tbl;
	struct ssdfs_fragments_chain_header *chain_hdr;
	struct ssdfs_raw_area *area;
	u16 fragments_count;
	int i = 0;
	int err = -ENODATA;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u, "
		  "start_fragment %d\n",
		  state->id, state->peb.id,
		  state->peb.log_offset,
		  start_fragment);

	area = &env->area;
	tbl = SSDFS_CONTENT_BLK_DESC_HDR(area);
	chain_hdr = &tbl->chain_hdr;

	if (chain_hdr->magic != SSDFS_CHAIN_HDR_MAGIC) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted chain header: invalid magic\n");
		return -EIO;
	}

	fragments_count = le16_to_cpu(chain_hdr->fragments_count);

	if (fragments_count == 0) {
		fragments_count = SSDFS_FRAGMENTS_CHAIN_MAX;
	} else {
		fragments_count = min_t(u16, fragments_count,
					SSDFS_FRAGMENTS_CHAIN_MAX);
	}

	for (i = start_fragment; i < fragments_count; i++) {
		err = ssdfs_copy_blk_desc_fragment(state, env, i);
		if (err == -ENODATA) {
			SSDFS_DBG(state->base.show_debug,
				  "corrupted fragment descriptor: "
				  "fragment_index %d\n", i);
		} else if (err == -EOPNOTSUPP) {
			SSDFS_ERR("TODO: implement support\n");
			return err;
		} else if (err) {
			SSDFS_ERR("fail to process fragment: "
				  "fragment_index %d, err %d\n",
				  i, err);
			return err;
		} else {
			/* valid fragment has been prepared */
			break;
		}
	}

	if (err == -ENODATA) {
		SSDFS_DBG(state->base.show_debug,
			  "valid fragment has not been found: "
			  "fragment_index %d\n", i);
	}

	return err;
}

static
int is_ssdfs_recoverfs_content_buffer_ready(struct ssdfs_thread_state *state,
					    int area_index)
{
	struct ssdfs_raw_area *area_desc = NULL;

	area_desc = &state->raw_dump.desc[area_index].area;

	if (SSDFS_CONTENT_ITER(area_desc)->state >=
			SSDFS_RAW_AREA_CONTENT_ITERATOR_INITIALIZED &&
	    SSDFS_CONTENT_ITER(area_desc)->state <
			SSDFS_RAW_AREA_CONTENT_STATE_MAX) {
		return SSDFS_TRUE;
	}

	return SSDFS_FALSE;
}

static
int ssdfs_recoverfs_prepare_first_content_buffer(struct ssdfs_thread_state *state,
						 int area_index)
{
	struct ssdfs_raw_area_environment *env;
	int err;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u\n",
		  state->id, state->peb.id, state->peb.log_offset);

	env = &state->raw_dump.desc[area_index];

	if (!env->buffer.ptr) {
		SSDFS_ERR("buffer is not allocated\n");
		return -ERANGE;
	}

	if (env->buffer.size == 0 || env->buffer.size < env->area.size) {
		SSDFS_ERR("invalid buffer size %u\n",
			  env->buffer.size);
	}

	switch (area_index) {
	case SSDFS_BLK_DESC_AREA_INDEX:
		ssdfs_read_portion_header(state, env, 0);

		err = ssdfs_extract_valid_blk_desc_fragment(state, env, 0);
		if (err == -ENODATA) {
			SSDFS_DBG(state->base.show_debug,
				  "unable to extract fragment\n");
			return err;
		} else if (err) {
			SSDFS_ERR("fail to process fragment: "
				  "err %d\n", err);
			return err;
		}
		break;

	default:
		SSDFS_ERR("unknown area index %d\n",
			  area_index);
		return -EOPNOTSUPP;
	}

	return 0;
}

static
int is_ssdfs_recoverfs_content_buffer_processed(struct ssdfs_thread_state *state,
						int area_index)
{
	struct ssdfs_raw_area *area_desc = NULL;

	area_desc = &state->raw_dump.desc[area_index].area;

	if (SSDFS_CONTENT_ITER(area_desc)->state >=
			SSDFS_RAW_AREA_CONTENT_PROCESSED &&
	    SSDFS_CONTENT_ITER(area_desc)->state <
			SSDFS_RAW_AREA_CONTENT_STATE_MAX) {
		return SSDFS_TRUE;
	}

	return SSDFS_FALSE;
}

static
int ssdfs_recoverfs_prepare_next_content_buffer(struct ssdfs_thread_state *state,
						int area_index)
{
	struct ssdfs_raw_area_environment *env;
	struct ssdfs_raw_area *area;
	struct ssdfs_area_block_table *tbl;
	struct ssdfs_fragments_chain_header *chain_hdr;
	u16 start_fragment;
	u16 fragments_count;
	u32 offset;
	int err;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u\n",
		  state->id, state->peb.id, state->peb.log_offset);

	env = &state->raw_dump.desc[area_index];
	area = &env->area;

	if (!env->buffer.ptr) {
		SSDFS_ERR("buffer is not allocated\n");
		return -ERANGE;
	}

	if (env->buffer.size == 0 || env->buffer.size < area->size) {
		SSDFS_ERR("invalid buffer size %u\n",
			  env->buffer.size);
	}

	tbl = SSDFS_CONTENT_BLK_DESC_HDR(area);
	chain_hdr = &tbl->chain_hdr;

	if (chain_hdr->magic != SSDFS_CHAIN_HDR_MAGIC) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted chain header: invalid magic\n");
		return -EIO;
	}

	fragments_count = le16_to_cpu(chain_hdr->fragments_count);

	switch (area_index) {
	case SSDFS_BLK_DESC_AREA_INDEX:
		start_fragment = SSDFS_CONTENT_ITER(area)->fragment_index;

		if (start_fragment >= fragments_count) {
			SSDFS_ERR("invalid fragment_index: "
				  "start_fragment %u >= fragments_count %u\n",
				  start_fragment, fragments_count);
			return -ERANGE;
		}

		start_fragment++;

		if (start_fragment == fragments_count) {
			err = -ENODATA;
			SSDFS_DBG(state->base.show_debug,
				  "no more fragments in portion\n");
			return err;
		}

		err = ssdfs_extract_valid_blk_desc_fragment(state, env,
							    start_fragment);
		if (err == -ENODATA) {
			SSDFS_DBG(state->base.show_debug,
				  "unable to extract fragment\n");

			offset = SSDFS_CONTENT_ITER(area)->portion_offset;
			offset += SSDFS_CONTENT_ITER(area)->portion_size;

			ssdfs_read_portion_header(state, env, offset);

			err = ssdfs_extract_valid_blk_desc_fragment(state,
								    env, 0);
			if (err == -ENODATA) {
				SSDFS_DBG(state->base.show_debug,
					  "unable to extract fragment\n");
				return err;
			} else if (err) {
				SSDFS_ERR("fail to process fragment: "
					  "err %d\n", err);
				return err;
			}
		} else if (err) {
			SSDFS_ERR("fail to process fragment: "
				  "err %d\n", err);
			return err;
		}
		break;

	default:
		SSDFS_ERR("unknown area index %d\n",
			  area_index);
		return -EOPNOTSUPP;
	}

	return 0;
}

static
int ssdfs_recoverfs_get_next_blk_desc(struct ssdfs_thread_state *state,
				      struct ssdfs_block_descriptor *blk_desc)
{
	struct ssdfs_raw_area *area_desc = NULL;
	struct ssdfs_raw_content_iterator *iter;
	struct ssdfs_raw_buffer *uncompressed;
	struct ssdfs_block_descriptor *found = NULL;
	int index = SSDFS_BLK_DESC_AREA_INDEX;
	int err;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u\n",
		  state->id, state->peb.id, state->peb.log_offset);

	if (!is_ssdfs_recoverfs_content_buffer_ready(state, index)) {
		err = ssdfs_recoverfs_prepare_first_content_buffer(state,
								   index);
		if (err) {
			SSDFS_ERR("fail to prepare content buffer: err %d\n",
				  err);
			return err;
		}
	} else if (is_ssdfs_recoverfs_content_buffer_processed(state, index)) {
		err = ssdfs_recoverfs_prepare_next_content_buffer(state,
								  index);
		if (err == -ENODATA) {
			SSDFS_DBG(state->base.show_debug,
				  "no more block descriptors in buffer\n");
			return err;
		} else if (err) {
			SSDFS_ERR("fail to prepare content buffer: err %d\n",
				  err);
			return err;
		}
	}

	area_desc = &state->raw_dump.desc[index].area;

	switch (SSDFS_CONTENT_ITER(area_desc)->state) {
	case SSDFS_RAW_AREA_CONTENT_ITERATOR_INITIALIZED:
		/* continue logic */
		break;

	default:
		SSDFS_ERR("unexpected content state %#x\n",
			  SSDFS_CONTENT_ITER(area_desc)->state);
		return -ERANGE;
	}

	iter = SSDFS_CONTENT_ITER(area_desc);
	uncompressed = SSDFS_CONTENT_BUFFER(area_desc);

	if (uncompressed->ptr == NULL) {
		SSDFS_ERR("uncompressed buffer is absent\n");
		return -ERANGE;
	}

	if (iter->item_size != sizeof(struct ssdfs_block_descriptor)) {
		SSDFS_ERR("invalid item size %u\n",
			  iter->item_size);
		return -ERANGE;
	}

	if ((iter->item_offset + iter->item_size) > uncompressed->size) {
		SSDFS_ERR("inconsistent iterator: "
			  "iter (item_offset %u, item_size %u), "
			  "buffer size %u\n",
			  iter->item_offset, iter->item_size,
			  uncompressed->size);
		return -ERANGE;
	}

	found = (struct ssdfs_block_descriptor *)((u8 *)uncompressed->ptr +
							iter->item_offset);
	memcpy(blk_desc, found, iter->item_size);

	err = SSDFS_CONTENT_ITERATOR_INCREMENT(iter);
	if (err == -ENODATA) {
		err = 0;
		SSDFS_DBG(state->base.show_debug,
			  "current fragment %d is processed: "
			  "item_offset %u, fragment_size %u\n",
			  iter->fragment_index,
			  iter->item_offset,
			  iter->fragment_size);
	}

	return 0;
}

static inline
int IS_BLK_STATE_INVALID(struct ssdfs_blk_state_offset *blk_state)
{
	return le16_to_cpu(blk_state->log_start_page) >= U16_MAX ||
		blk_state->log_area >= U8_MAX ||
		blk_state->peb_migration_id >= U8_MAX ||
		le32_to_cpu(blk_state->byte_offset) >= U32_MAX;
}

static
int ssdfs_recoverfs_read_raw_block_state(struct ssdfs_thread_state *state,
					 struct ssdfs_blk_state_offset *blk_state)
{
	struct ssdfs_raw_dump_environment *dump_env;
	struct ssdfs_raw_area_environment *area_env;
	struct ssdfs_raw_buffer *area_buf;
	u64 peb_id;
	u32 peb_size;
	u64 offset;
	u32 block_size;
	int area_index;
	int err;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u, "
		  "BLK_STATE: (log_start_page %u, log_area %u, "
		  "peb_migration_id %u, byte_offset %u)\n",
		  state->id, state->peb.id, state->peb.log_offset,
		  le16_to_cpu(blk_state->log_start_page),
		  blk_state->log_area,
		  blk_state->peb_migration_id,
		  le32_to_cpu(blk_state->byte_offset));

	area_index = SSDFS_AREA_TYPE2INDEX(blk_state->log_area);

	dump_env = &state->raw_dump;
	area_env = SSDFS_RAW_AREA_ENV(dump_env, area_index);

	peb_id = state->peb.id;
	peb_size = state->base.erase_size;

	if (area_env->area.offset >= U64_MAX) {
		SSDFS_ERR("invalid area offset %llu\n",
			  area_env->area.offset);
		return -ERANGE;
	}

	offset = area_env->area.offset;
	offset += le32_to_cpu(blk_state->byte_offset);

	if (offset >= (area_env->area.offset + area_env->area.size)) {
		SSDFS_ERR("invalid block state offset: "
			  "offset %llu, area_offset %llu, area_size %u\n",
			  offset, area_env->area.offset, area_env->area.size);
		return -ERANGE;
	}

	BUG_ON(offset >= U32_MAX);

	if (!is_pagesize_valid(state->base.page_size)) {
		SSDFS_ERR("invalid logical block size %u\n",
			  state->base.page_size);
		return -ERANGE;
	}

	area_buf = SSDFS_COMPR_CONTENT(dump_env, area_index);
	block_size = state->base.page_size;

	err = ssdfs_recoverfs_prepare_raw_buffer(area_buf, block_size);
	if (err) {
		SSDFS_ERR("fail to prepare raw buffer: "
			  "size %u, err %d\n",
			  block_size, err);
		return err;
	}

	err = ssdfs_read_area_content(&state->base,
				      peb_id, peb_size,
				      offset, block_size,
				      area_buf->ptr);
	if (err) {
		SSDFS_ERR("fail to read block state: "
			  "peb_id %llu, peb_size %u, "
			  "offset %llu, block_size %u, err %d\n",
			  peb_id, peb_size,
			  offset, block_size,
			  err);
		return err;
	}

	return 0;
}

static
int ssdfs_recoverfs_parse_block_fragment(struct ssdfs_thread_state *state,
					 struct ssdfs_blk_state_offset *blk_state,
					 int fragment_index)
{
	struct ssdfs_raw_dump_environment *dump_env;
	struct ssdfs_raw_area_environment *area_env;
	struct ssdfs_raw_buffer *compr_content;
	struct ssdfs_raw_buffer *uncompr_content;
	struct ssdfs_block_state_descriptor *blk_state_desc;
	struct ssdfs_fragments_chain_header *chain_hdr;
	struct ssdfs_fragment_desc *frag_desc;
	u8 *src_ptr = NULL, *dst_ptr = NULL;
	u32 block_size;
	u32 fragments_per_block;
	u16 fragments_count;
	u32 blk_state_offset;
	u32 src_offset;
	u64 dst_offset;
	u32 compr_size;
	u32 uncompr_size;
	int area_index;
	int err;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u, "
		  "BLK_STATE: (log_start_page %u, log_area %u, "
		  "peb_migration_id %u, byte_offset %u)\n",
		  state->id, state->peb.id, state->peb.log_offset,
		  le16_to_cpu(blk_state->log_start_page),
		  blk_state->log_area,
		  blk_state->peb_migration_id,
		  le32_to_cpu(blk_state->byte_offset));

	if (!is_pagesize_valid(state->base.page_size)) {
		SSDFS_ERR("invalid logical block size %u\n",
			  state->base.page_size);
		return -ERANGE;
	}

	block_size = state->base.page_size;
	fragments_per_block = block_size / SSDFS_4KB;

	area_index = SSDFS_AREA_TYPE2INDEX(blk_state->log_area);

	dump_env = &state->raw_dump;
	area_env = SSDFS_RAW_AREA_ENV(dump_env, area_index);
	compr_content = SSDFS_COMPR_CONTENT(dump_env, area_index);

	blk_state_desc =
		(struct ssdfs_block_state_descriptor *)compr_content->ptr;
	chain_hdr = &blk_state_desc->chain_hdr;

	if (chain_hdr->magic != SSDFS_CHAIN_HDR_MAGIC) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted chain header: invalid magic\n");
		return -EIO;
	}

	fragments_count = le16_to_cpu(chain_hdr->fragments_count);

	if (fragments_count > fragments_per_block) {
		SSDFS_ERR("invalid fragments count: "
			  "fragments_count %u, fragments_per_block %u\n",
			  fragments_count, fragments_per_block);
	}

	if (fragment_index >= fragments_count) {
		SSDFS_DBG(state->base.show_debug,
			  "fragment_index %d >= fragments_count %u\n",
			  fragment_index, fragments_count);
		return 0;
	}

	frag_desc = (struct ssdfs_fragment_desc *)((u8 *)compr_content->ptr +
				sizeof(struct ssdfs_block_state_descriptor));

	if (frag_desc[fragment_index].magic != SSDFS_FRAGMENT_DESC_MAGIC) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted fragment descriptor: "
			  "invalid magic\n");
		return -ENODATA;
	}

	blk_state_offset = le32_to_cpu(blk_state->byte_offset);
	src_offset = le32_to_cpu(frag_desc[fragment_index].offset);

	if (src_offset < blk_state_offset) {
		SSDFS_ERR("corrupted fragment descriptor: "
			  "blk_state_offset %u, src_offset %u\n",
			  blk_state_offset, src_offset);
		return -EIO;
	}

	src_offset -= blk_state_offset;

	compr_size = le16_to_cpu(frag_desc[fragment_index].compr_size);
	uncompr_size = le16_to_cpu(frag_desc[fragment_index].uncompr_size);

	if (compr_size > SSDFS_4KB) {
		SSDFS_ERR("corrupted block fragment: "
			  "compr_size %u\n", compr_size);
		return -EIO;
	}

	if (uncompr_size > SSDFS_4KB) {
		SSDFS_ERR("corrupted block fragment: "
			  "compr_size %u\n", compr_size);
		return -EIO;
	}

	if ((src_offset + compr_size) > area_env->buffer.size) {
		SSDFS_DBG(state->base.show_debug,
			  "corrupted fragment descriptor: "
			  "fragment (offset %u, size %u), "
			  "buffer (size %u)\n",
			  src_offset, compr_size,
			  area_env->buffer.size);
		return -EIO;
	}

	uncompr_content = SSDFS_UNCOMPR_BUFFER(dump_env, area_index);

	err = ssdfs_recoverfs_prepare_raw_buffer(uncompr_content, SSDFS_4KB);
	if (err) {
		SSDFS_ERR("fail to prepare raw buffer: "
			  "size %u, err %d\n",
			  SSDFS_4KB, err);
		return err;
	}

	switch (frag_desc[fragment_index].type) {
	case SSDFS_FRAGMENT_UNCOMPR_BLOB:
		src_ptr = (u8 *)compr_content->ptr + src_offset;
		dst_ptr = (u8 *)uncompr_content->ptr;
		memcpy(dst_ptr, src_ptr, uncompr_size);
		break;

	case SSDFS_FRAGMENT_ZLIB_BLOB:
		src_ptr = (u8 *)compr_content->ptr + src_offset;
		dst_ptr = (u8 *)uncompr_content->ptr;
		err = ssdfs_zlib_decompress(src_ptr, dst_ptr,
					    compr_size, uncompr_size,
					    state->base.show_debug);
		if (err) {
			SSDFS_ERR("fail to decompress: err %d\n", err);
			return err;
		}
		break;

	case SSDFS_FRAGMENT_LZO_BLOB:
		SSDFS_ERR("TODO: implement LZO support\n");
		return -EOPNOTSUPP;
		break;

	default:
		SSDFS_ERR("unexpected fragment type %#x\n",
			  frag_desc[fragment_index].type);
		return -EIO;
	}

	if (frag_desc->flags & SSDFS_FRAGMENT_HAS_CSUM) {
		__le32 csum;

		BUG_ON(!dst_ptr);

		csum = ssdfs_crc32_le(dst_ptr, uncompr_size);
		if (csum != frag_desc->checksum) {
			SSDFS_DBG(state->base.show_debug,
				  "corrupted fragment: "
				  "checksum1 %#x != checksum2 %#x\n",
				  le32_to_cpu(csum),
				  le32_to_cpu(frag_desc->checksum));
			return -EIO;
		}
	}

	dst_offset = (u64)fragment_index * SSDFS_4KB;
	BUG_ON(dst_offset >= U32_MAX);

	BUG_ON(!SSDFS_DUMP_DATA(dump_env)->ptr);
	memcpy(SSDFS_DUMP_DATA(dump_env)->ptr + dst_offset,
		uncompr_content->ptr, SSDFS_4KB);

	return 0;
}

static int
ssdfs_recoverfs_parse_and_decompress_block_state(struct ssdfs_thread_state *state,
					 struct ssdfs_blk_state_offset *blk_state)
{
	struct ssdfs_raw_dump_environment *dump_env;
	u32 block_size;
	u32 fragments_per_block;
	int area_index;
	int i;
	int err;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u, "
		  "BLK_STATE: (log_start_page %u, log_area %u, "
		  "peb_migration_id %u, byte_offset %u)\n",
		  state->id, state->peb.id, state->peb.log_offset,
		  le16_to_cpu(blk_state->log_start_page),
		  blk_state->log_area,
		  blk_state->peb_migration_id,
		  le32_to_cpu(blk_state->byte_offset));

	area_index = SSDFS_AREA_TYPE2INDEX(blk_state->log_area);

	dump_env = &state->raw_dump;

	if (!is_pagesize_valid(state->base.page_size)) {
		SSDFS_ERR("invalid logical block size %u\n",
			  state->base.page_size);
		return -ERANGE;
	}

	block_size = state->base.page_size;

	err = ssdfs_recoverfs_prepare_raw_buffer(SSDFS_DUMP_DATA(dump_env),
						 block_size);
	if (err) {
		SSDFS_ERR("fail to prepare raw buffer: "
			  "size %u, err %d\n",
			  block_size, err);
		return err;
	}

	switch (blk_state->log_area) {
	case SSDFS_LOG_MAIN_AREA:
		BUG_ON(!SSDFS_COMPR_CONTENT(dump_env, area_index)->ptr);
		memcpy(SSDFS_DUMP_DATA(dump_env)->ptr,
			SSDFS_COMPR_CONTENT(dump_env, area_index)->ptr,
			block_size);
		break;

	case SSDFS_LOG_DIFFS_AREA:
	case SSDFS_LOG_JOURNAL_AREA:
		fragments_per_block = block_size / SSDFS_4KB;

		if (fragments_per_block == 0) {
			SSDFS_ERR("invalid logical block size %u\n",
				  block_size);
			return -ERANGE;
		}

		for (i = 0; i < fragments_per_block; i++) {
			err = ssdfs_recoverfs_parse_block_fragment(state,
								   blk_state,
								   i);
			if (err) {
				SSDFS_ERR("fail to parse block's fragment: "
					  "fragment_index %d, err %d\n",
					  i, err);
				return err;
			}
		}
		break;

	default:
		SSDFS_ERR("unexpected area type %#x\n",
			  blk_state->log_area);
		return -ERANGE;
	}

	return 0;
}

static
int ssdfs_recoverfs_read_raw_block_delta(struct ssdfs_thread_state *state,
					struct ssdfs_blk_state_offset *blk_state)
{
	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u, "
		  "BLK_STATE: (log_start_page %u, log_area %u, "
		  "peb_migration_id %u, byte_offset %u)\n",
		  state->id, state->peb.id, state->peb.log_offset,
		  le16_to_cpu(blk_state->log_start_page),
		  blk_state->log_area,
		  blk_state->peb_migration_id,
		  le32_to_cpu(blk_state->byte_offset));

	SSDFS_ERR("TODO: implement\n");
	return -EOPNOTSUPP;
}

static
int ssdfs_recoverfs_parse_and_decompress_delta(struct ssdfs_thread_state *state,
					struct ssdfs_blk_state_offset *blk_state)
{
	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u, "
		  "BLK_STATE: (log_start_page %u, log_area %u, "
		  "peb_migration_id %u, byte_offset %u)\n",
		  state->id, state->peb.id, state->peb.log_offset,
		  le16_to_cpu(blk_state->log_start_page),
		  blk_state->log_area,
		  blk_state->peb_migration_id,
		  le32_to_cpu(blk_state->byte_offset));

	SSDFS_ERR("TODO: implement\n");
	return -EOPNOTSUPP;
}

static
int ssdfs_recoverfs_apply_delta(struct ssdfs_thread_state *state,
				struct ssdfs_blk_state_offset *blk_state)
{
	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u, "
		  "BLK_STATE: (log_start_page %u, log_area %u, "
		  "peb_migration_id %u, byte_offset %u)\n",
		  state->id, state->peb.id, state->peb.log_offset,
		  le16_to_cpu(blk_state->log_start_page),
		  blk_state->log_area,
		  blk_state->peb_migration_id,
		  le32_to_cpu(blk_state->byte_offset));

	SSDFS_ERR("TODO: implement\n");
	return -EOPNOTSUPP;
}

static
int ssdfs_recoverfs_read_block_state(struct ssdfs_thread_state *state,
				     int index,
				     struct ssdfs_blk_state_offset *blk_state)
{
	int err;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u, "
		  "BLK_STATE: (log_start_page %u, log_area %u, "
		  "peb_migration_id %u, byte_offset %u)\n",
		  state->id, state->peb.id, state->peb.log_offset,
		  le16_to_cpu(blk_state->log_start_page),
		  blk_state->log_area,
		  blk_state->peb_migration_id,
		  le32_to_cpu(blk_state->byte_offset));

	if (index == 0) {
		err = ssdfs_recoverfs_read_raw_block_state(state, blk_state);
		if (err) {
			SSDFS_ERR("fail to read raw block state: err %d\n",
				  err);
			return err;
		}

		err = ssdfs_recoverfs_parse_and_decompress_block_state(state,
									blk_state);
		if (err) {
			SSDFS_ERR("fail to parse and decompress block state: err %d\n",
				  err);
			return err;
		}
	} else {
		err = ssdfs_recoverfs_read_raw_block_delta(state, blk_state);
		if (err) {
			SSDFS_ERR("fail to read raw block's delta: err %d\n",
				  err);
			return err;
		}

		err = ssdfs_recoverfs_parse_and_decompress_delta(state,
								 blk_state);
		if (err) {
			SSDFS_ERR("fail to parse and decompress delta: err %d\n",
				  err);
			return err;
		}

		err = ssdfs_recoverfs_apply_delta(state, blk_state);
		if (err) {
			SSDFS_ERR("fail to apply delta to block state: err %d\n",
				  err);
			return err;
		}
	}

	return 0;
}

static
int ssdfs_recoverfs_extract_block_state(struct ssdfs_thread_state *state,
					struct ssdfs_block_descriptor *blk_desc)
{
	struct ssdfs_blk_state_offset *blk_state;
	int i;
	int err = -ENODATA;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u, "
		  "BLK_DESC: (ino %llu, logical_offset %u, "
		  "peb_index %u, peb_page %u)\n",
		  state->id, state->peb.id, state->peb.log_offset,
		  le64_to_cpu(blk_desc->ino),
		  le32_to_cpu(blk_desc->logical_offset),
		  le16_to_cpu(blk_desc->peb_index),
		  le16_to_cpu(blk_desc->peb_page));

	for (i = 0; i < SSDFS_BLK_STATE_OFF_MAX; i++) {
		blk_state = &blk_desc->state[i];

		if (IS_BLK_STATE_INVALID(blk_state))
			break;

		err = ssdfs_recoverfs_read_block_state(state, i, blk_state);
		if (err) {
			SSDFS_ERR("fail to read block state: "
				  "ino %llu, logical_offset %u, err %d\n",
				  le64_to_cpu(blk_desc->ino),
				  le32_to_cpu(blk_desc->logical_offset),
				  err);
			return err;
		}
	}

	return err;
}

static
int ssdfs_recoverfs_copy_block_state(struct ssdfs_thread_state *state,
				     struct ssdfs_block_descriptor *blk_desc)
{
	struct ssdfs_raw_dump_environment *env;
	int fd = -1;
	ssize_t written_bytes;
	int err = 0;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u, "
		  "BLK_DESC: (ino %llu, logical_offset %u)\n",
		  state->id, state->peb.id, state->peb.log_offset,
		  le64_to_cpu(blk_desc->ino),
		  le32_to_cpu(blk_desc->logical_offset));

	env = &state->raw_dump;

	memset(state->name_buf, 0, sizeof(state->name_buf));

	snprintf(state->name_buf, sizeof(state->name_buf) - 1,
		 "%llu-%u",
		 le64_to_cpu(blk_desc->ino),
		 le32_to_cpu(blk_desc->logical_offset));

	/*
	 * Check that file is absent
	 */
	fd = openat(state->checkpoint_folder.fd,
		    state->name_buf,
		    O_RDWR | O_LARGEFILE,
		    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd < 0) {
		/*
		 * File absence is expected.
		 * Continue logic.
		 */
	} else {
		SSDFS_ERR("file %d exists already: "
			  "name %s, thread %d, err %d: %s\n",
			  fd, state->name_buf,
			  state->id, errno, strerror(errno));
		SSDFS_ERR("Rewrite existing file: "
			  "name %s, thread %d, PEB %llu, log_offset %u, "
			  "BLK_DESC: (ino %llu, logical_offset %u)\n",
			  state->name_buf, state->id, state->peb.id,
			  state->peb.log_offset,
			  le64_to_cpu(blk_desc->ino),
			  le32_to_cpu(blk_desc->logical_offset));
		goto write_to_file;
	}

	fd = openat(state->checkpoint_folder.fd,
		    state->name_buf,
		    O_CREAT | O_RDWR | O_LARGEFILE,
		    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd < 0) {
		err = errno;
		SSDFS_ERR("unable to create %s: %s\n",
			  state->name_buf, strerror(errno));
		goto fail_copy_block_state;
	}

write_to_file:
	BUG_ON(!SSDFS_DUMP_DATA(env)->ptr);

	written_bytes = write(fd, SSDFS_DUMP_DATA(env)->ptr,
				SSDFS_DUMP_DATA(env)->size);
	if (written_bytes < 0) {
		err = errno;
		SSDFS_ERR("fail to write: %s\n",
			  strerror(errno));
		goto close_file;
	}

	if (fsync(fd) < 0) {
		err = errno;
		SSDFS_ERR("fail to sync: %s\n",
			  strerror(errno));
		goto close_file;
	}

close_file:
	close(fd);

fail_copy_block_state:
	return err;
}

static
int ssdfs_recoverfs_parse_full_log(struct ssdfs_thread_state *state)
{
	struct ssdfs_raw_dump_environment *dump_env;
	struct ssdfs_segment_header *seg_hdr = NULL;
	struct ssdfs_log_footer *footer = NULL;
	struct ssdfs_signature *magic = NULL;
	struct ssdfs_metadata_descriptor *meta_desc = NULL;
	struct ssdfs_raw_area_environment *area_env;
	struct ssdfs_raw_area *area_desc = NULL;
	struct ssdfs_raw_buffer *raw_buf = NULL;
	struct ssdfs_block_descriptor blk_desc;
	u64 offset;
	u32 size;
	u32 peb_size = state->peb.peb_size;
	u16 seg_type;
	u64 timestamp;
	u32 latest_area_offset = 0;
	u32 latest_area_size = 0;
	u32 next_log_index = state->peb.log_index + 1;
	u32 i;
	int err = 0;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u\n",
		  state->id, state->peb.id, state->peb.log_offset);

	dump_env = &state->raw_dump;

	BUG_ON(!SSDFS_RAW_SEG_HDR(dump_env)->ptr);

	seg_hdr = SSDFS_SEG_HDR(SSDFS_RAW_SEG_HDR(dump_env)->ptr);

	seg_type = le16_to_cpu(seg_hdr->seg_type);
	timestamp = le64_to_cpu(seg_hdr->timestamp);

	switch (seg_type) {
	case SSDFS_UNKNOWN_SEG_TYPE:
	case SSDFS_SB_SEG_TYPE:
		/* don't create folder */
		break;

	default:
		err = ssdfs_recoverfs_create_folder(state, timestamp);
		break;
	}

	if (err) {
		SSDFS_ERR("fail to parse full log: "
			  "thread %d, PEB %llu, log_offset %u, err %d\n",
			  state->id, state->peb.id,
			  state->peb.log_offset, err);
		goto finish_log_parsing;
	}

	for (i = 0; i < SSDFS_SEG_HDR_DESC_MAX; i++) {
		meta_desc = &seg_hdr->desc_array[i];
		area_env = SSDFS_RAW_AREA_ENV(dump_env, i);
		area_desc = &area_env->area;

		offset = le32_to_cpu(meta_desc->offset);
		size = le32_to_cpu(meta_desc->size);

		SSDFS_DBG(state->base.show_debug,
			  "thread %d, PEB %llu, log_offset %u, "
			  "area_index %d, offset %llu, size %u\n",
			  state->id, state->peb.id, state->peb.log_offset,
			  i, offset, size);

		err = ssdfs_create_raw_area_environment(area_env,
							offset, size,
							SSDFS_AREA2BUFFER_SIZE(i));
		if (err) {
			SSDFS_ERR("fail to create area %d: "
				  "area_offset %llu, area_size %u, "
				  "raw_buffer_size %u, err %d\n",
				  i,
				  offset, size,
				  SSDFS_AREA2BUFFER_SIZE(i),
				  err);
			goto finish_log_parsing;
		}

		if (size != 0) {
			area_desc->offset = offset;
			area_desc->size = size;

			if (latest_area_offset < offset) {
				if ((offset + size) < peb_size) {
					latest_area_offset = offset;
					latest_area_size = size;
				} else {
					SSDFS_DBG(state->base.show_debug,
						  "corrupted area descriptor: "
						  "thread %d, PEB %llu, "
						  "log_offset %u, "
						  "area_index %u, "
						  "offset %llu, size %u\n",
						  state->id, state->peb.id,
						  state->peb.log_offset,
						  i, offset, size);
				}
			}
		}
	}

	i = SSDFS_LOG_FOOTER_INDEX;
	area_desc = &SSDFS_RAW_AREA_ENV(dump_env, i)->area;

	if (area_desc->offset >= U64_MAX) {
		next_log_index =
			ssdfs_recoverfs_define_next_log_index(state,
							latest_area_offset,
							latest_area_size);
	} else {
		raw_buf = SSDFS_COMPR_CONTENT(dump_env, i);
		size = min_t(u32, area_desc->size, raw_buf->size);

		err = ssdfs_recoverfs_prepare_raw_buffer(raw_buf, SSDFS_4KB);
		if (err) {
			SSDFS_ERR("fail to prepare raw buffer: "
				  "size %u, err %d\n",
				  SSDFS_4KB, err);
			goto close_checkpoint_folder;
		}

		err = ssdfs_read_log_footer(&state->base,
					    state->peb.id,
					    state->peb.peb_size,
					    area_desc->offset,
					    size,
					    raw_buf->ptr);
		if (err) {
			SSDFS_ERR("fail to read PEB's footer: "
				  "peb_id %llu, peb_size %u, "
				  "area_offset %llu, err %d\n",
				  state->peb.id, state->peb.peb_size,
				  area_desc->offset, err);
			goto close_checkpoint_folder;
		}

		footer = SSDFS_LOG_FOOTER(raw_buf->ptr);
		magic = &footer->volume_state.magic;

		if (le32_to_cpu(magic->common) == SSDFS_SUPER_MAGIC &&
		    le16_to_cpu(magic->key) == SSDFS_LOG_FOOTER_MAGIC) {
			state->peb.log_size = le32_to_cpu(footer->log_bytes);
			next_log_index = state->peb.log_size + SSDFS_4KB - 1;
			next_log_index /= SSDFS_4KB;
			next_log_index += state->peb.log_index;
		} else {
			next_log_index =
				ssdfs_recoverfs_define_next_log_index(state,
							latest_area_offset,
							latest_area_size);
		}
	}

	i = SSDFS_BLK_DESC_AREA_INDEX;
	area_desc = &SSDFS_RAW_AREA_ENV(dump_env, i)->area;

	if (area_desc->offset >= peb_size || area_desc->size == 0) {
		err = -ERANGE;
		SSDFS_DBG(state->base.show_debug,
			  "block descriptors area descriptor is corrupted: "
			  "thread %d, PEB %llu, log_offset %u\n",
			  state->id, state->peb.id, state->peb.log_offset);
		goto close_checkpoint_folder;
	}

	err = ssdfs_recoverfs_prefetch_blk_desc_table(state);
	if (err == -ENODATA) {
		SSDFS_DBG(state->base.show_debug,
			  "unable pre-fetch block descriptors table\n");
		goto close_checkpoint_folder;
	} else if (err) {
		SSDFS_ERR("fail to pre-fetch block descriptors table: "
			  "err %d\n", err);
		goto close_checkpoint_folder;
	}

	while (ssdfs_recoverfs_get_next_blk_desc(state, &blk_desc) == 0) {
		err = ssdfs_recoverfs_extract_block_state(state, &blk_desc);
		if (err) {
			SSDFS_DBG(state->base.show_debug,
				  "unable to extract block state: "
				  "thread %d, PEB %llu, log_offset %u, "
				  "ino %llu, logical_offset %u\n",
				  state->id, state->peb.id,
				  state->peb.log_offset,
				  le64_to_cpu(blk_desc.ino),
				  le32_to_cpu(blk_desc.logical_offset));
			continue;
		}

		err = ssdfs_recoverfs_copy_block_state(state, &blk_desc);
		if (err) {
			SSDFS_DBG(state->base.show_debug,
				  "unable to copy block state: "
				  "thread %d, PEB %llu, log_offset %u, "
				  "ino %llu, logical_offset %u\n",
				  state->id, state->peb.id,
				  state->peb.log_offset,
				  le64_to_cpu(blk_desc.ino),
				  le32_to_cpu(blk_desc.logical_offset));
			continue;
		}
	}

close_checkpoint_folder:
	close(state->checkpoint_folder.fd);

finish_log_parsing:
	state->peb.log_index = next_log_index;

	return err;
}

static
int ssdfs_recoverfs_parse_partial_log(struct ssdfs_thread_state *state)
{
	struct ssdfs_raw_dump_environment *dump_env;
	struct ssdfs_partial_log_header *pl_hdr = NULL;
	struct ssdfs_metadata_descriptor *meta_desc = NULL;
	struct ssdfs_raw_area_environment *area_env;
	struct ssdfs_raw_area *area_desc = NULL;
	struct ssdfs_block_descriptor blk_desc;
	u64 offset;
	u32 size;
	u32 peb_size = state->peb.peb_size;
	u16 seg_type;
	u64 timestamp;
	u32 latest_area_offset = 0;
	u32 next_log_index = state->peb.log_index + 1;
	int area_index;
	u32 i;
	int err = 0;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u\n",
		  state->id, state->peb.id, state->peb.log_offset);

	dump_env = &state->raw_dump;

	BUG_ON(!SSDFS_RAW_SEG_HDR(dump_env)->ptr);

	pl_hdr = SSDFS_PL_HDR(SSDFS_RAW_SEG_HDR(dump_env)->ptr);

	seg_type = le16_to_cpu(pl_hdr->seg_type);
	timestamp = le64_to_cpu(pl_hdr->timestamp);

	switch (seg_type) {
	case SSDFS_UNKNOWN_SEG_TYPE:
	case SSDFS_SB_SEG_TYPE:
		/* don't create folder */
		break;

	default:
		err = ssdfs_recoverfs_create_folder(state, timestamp);
		break;
	}

	if (err) {
		SSDFS_ERR("fail to parse full log: "
			  "thread %d, PEB %llu, log_offset %u, err %d\n",
			  state->id, state->peb.id,
			  state->peb.log_offset, err);
		goto finish_log_parsing;
	}

	for (i = 0; i < SSDFS_SEG_HDR_DESC_MAX; i++) {
		meta_desc = &pl_hdr->desc_array[i];
		area_env = SSDFS_RAW_AREA_ENV(dump_env, i);
		area_desc = &area_env->area;

		offset = le32_to_cpu(meta_desc->offset);
		size = le32_to_cpu(meta_desc->size);

		SSDFS_DBG(state->base.show_debug,
			  "thread %d, PEB %llu, log_offset %u, "
			  "area_index %d, offset %llu, size %u\n",
			  state->id, state->peb.id, state->peb.log_offset,
			  i, offset, size);

		err = ssdfs_create_raw_area_environment(area_env,
							offset, size,
							SSDFS_AREA2BUFFER_SIZE(i));
		if (err) {
			SSDFS_ERR("fail to create area %d: "
				  "area_offset %llu, area_size %u, "
				  "raw_buffer_size %u, err %d\n",
				  i,
				  offset, size,
				  SSDFS_AREA2BUFFER_SIZE(i),
				  err);
			goto finish_log_parsing;
		}

		if (size != 0) {
			area_desc->offset = offset;
			area_desc->size = size;

			if (latest_area_offset < offset) {
				if ((offset + size) < peb_size) {
					latest_area_offset = offset;
				} else {
					SSDFS_DBG(state->base.show_debug,
						  "corrupted area descriptor: "
						  "thread %d, PEB %llu, "
						  "log_offset %u, "
						  "area_index %u, "
						  "offset %llu, size %u\n",
						  state->id, state->peb.id,
						  state->peb.log_offset,
						  i, offset, size);
				}
			}
		}
	}

	state->peb.log_size = le32_to_cpu(pl_hdr->log_bytes);
	next_log_index = state->peb.log_size + SSDFS_4KB - 1;
	next_log_index /= SSDFS_4KB;
	next_log_index += state->peb.log_index;

	area_index = SSDFS_BLK_DESC_AREA_INDEX;
	area_desc = &SSDFS_RAW_AREA_ENV(dump_env, area_index)->area;

	if (area_desc->offset >= peb_size || area_desc->size == 0) {
		err = -ERANGE;
		SSDFS_DBG(state->base.show_debug,
			  "block descriptors area descriptor is corrupted: "
			  "thread %d, PEB %llu, log_offset %u\n",
			  state->id, state->peb.id, state->peb.log_offset);
		goto close_checkpoint_folder;
	}

	err = ssdfs_recoverfs_prefetch_blk_desc_table(state);
	if (err == -ENODATA) {
		SSDFS_DBG(state->base.show_debug,
			  "unable pre-fetch block descriptors table\n");
		goto close_checkpoint_folder;
	} else if (err) {
		SSDFS_ERR("fail to pre-fetch block descriptors table: "
			  "err %d\n", err);
		goto close_checkpoint_folder;
	}

	while (ssdfs_recoverfs_get_next_blk_desc(state, &blk_desc) == 0) {
		err = ssdfs_recoverfs_extract_block_state(state, &blk_desc);
		if (err) {
			SSDFS_DBG(state->base.show_debug,
				  "unable to extract block state: "
				  "thread %d, PEB %llu, log_offset %u, "
				  "ino %llu, logical_offset %u\n",
				  state->id, state->peb.id,
				  state->peb.log_offset,
				  le64_to_cpu(blk_desc.ino),
				  le32_to_cpu(blk_desc.logical_offset));
			continue;
		}

		err = ssdfs_recoverfs_copy_block_state(state, &blk_desc);
		if (err) {
			SSDFS_DBG(state->base.show_debug,
				  "unable to copy block state: "
				  "thread %d, PEB %llu, log_offset %u, "
				  "ino %llu, logical_offset %u\n",
				  state->id, state->peb.id,
				  state->peb.log_offset,
				  le64_to_cpu(blk_desc.ino),
				  le32_to_cpu(blk_desc.logical_offset));
			continue;
		}
	}

close_checkpoint_folder:
	close(state->checkpoint_folder.fd);

finish_log_parsing:
	state->peb.log_index = next_log_index;

	return err;
}

int ssdfs_recoverfs_process_peb(struct ssdfs_thread_state *state)
{
	struct ssdfs_raw_dump_environment *dump_env;
	struct ssdfs_segment_header *seg_hdr = NULL;
	struct ssdfs_signature *magic;
	u32 logs_count = state->base.erase_size / SSDFS_4KB;
	int err;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu\n",
		  state->id, state->peb.id);

	dump_env = &state->raw_dump;
	state->peb.log_index = 0;

	BUG_ON(!SSDFS_RAW_SEG_HDR(dump_env)->ptr);

	do {
		err = ssdfs_create_raw_area_environment(&dump_env->seg_hdr,
						state->peb.log_offset,
						sizeof(struct ssdfs_segment_header),
						SSDFS_4KB);
		if (err) {
			SSDFS_ERR("fail to create segment header area: "
				  "log_offset %u, err %d\n",
				  state->peb.log_offset, err);
			return err;
		}

		err = ssdfs_recoverfs_find_valid_log(state);
		if (err == -ENODATA) {
			/* PEB has none valid log */
			return 0;
		} else if (err) {
			SSDFS_ERR("fail to find valid PEB: "
				  "PEB %llu, err %d\n",
				  state->peb.id, err);
			return err;
		}

		seg_hdr = SSDFS_SEG_HDR(SSDFS_RAW_SEG_HDR(dump_env)->ptr);
		magic = &seg_hdr->volume_hdr.magic;

		if (le32_to_cpu(magic->common) == SSDFS_SUPER_MAGIC &&
		    le16_to_cpu(magic->key) == SSDFS_SEGMENT_HDR_MAGIC) {
			/* parse full log */
			err = ssdfs_recoverfs_parse_full_log(state);
			if (err) {
				SSDFS_DBG(state->base.show_debug,
					  "unable to parse full log: "
					  "peb_id %llu, log_offset %u, "
					  "err %d\n",
					  state->peb.id,
					  state->peb.log_offset,
					  err);
			}
		} else if (le32_to_cpu(magic->common) == SSDFS_SUPER_MAGIC &&
			   le16_to_cpu(magic->key) ==
						SSDFS_PARTIAL_LOG_HDR_MAGIC) {
			/* parse partial log */
			err = ssdfs_recoverfs_parse_partial_log(state);
			if (err) {
				SSDFS_DBG(state->base.show_debug,
					  "unable to parse partial log: "
					  "peb_id %llu, log_offset %u, "
					  "err %d\n",
					  state->peb.id,
					  state->peb.log_offset,
					  err);
			}
		} else if (le32_to_cpu(magic->common) == SSDFS_SUPER_MAGIC &&
			   le16_to_cpu(magic->key) == SSDFS_LOG_FOOTER_MAGIC) {
			SSDFS_DBG(state->base.show_debug,
				  "found orphaned footer: "
				  "PEB %llu, offset %u\n",
				  state->peb.id, state->peb.log_offset);
			state->peb.log_index++;
			continue;
		} else {
			SSDFS_ERR("unexpected state: "
				  "PEB %llu, offset %u\n",
				  state->peb.id, state->peb.log_offset);
			return -ERANGE;
		}
	} while (state->peb.log_index < logs_count);

	return 0;
}
