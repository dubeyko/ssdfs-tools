//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/recoverfs.c - implementation of recoverfs.ssdfs
 *                    (volume recovering) utility.
 *
 * Copyright (c) 2020-2023 Viacheslav Dubeyko <slava@dubeyko.com>
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

#include "recoverfs.h"

static
int ssdfs_recoverfs_find_valid_log(struct ssdfs_thread_state *state,
				   u64 peb_id)
{
	struct ssdfs_segment_header *seg_hdr = NULL;
	struct ssdfs_signature *magic;
	u32 offset;
	int is_log_valid;
	u32 i;
	int err;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu\n",
		  state->id, peb_id);

	is_log_valid = SSDFS_FALSE;

	for (i = state->peb.log_index; i < state->peb.logs_count; i++) {
		offset = i * SSDFS_4KB;

		err = ssdfs_read_segment_header(&state->base,
					peb_id,
					state->peb.peb_size,
					offset,
					state->raw_dump.seg_hdr.buffer.size,
					state->raw_dump.seg_hdr.buffer.ptr);
		if (err) {
			SSDFS_ERR("fail to read PEB's header: "
				  "peb_id %llu, peb_size %u, err %d\n",
				  peb_id, state->peb.peb_size, err);
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
			  peb_id);
		return -ENODATA;
	}

	return 0;
}






/*

static
int ssdfs_recoverfs_prefetch_blk_desc_table(struct ssdfs_thread_state *state,
					    u64 peb_id,
					    struct ssdfs_segment_header *seg_hdr)
{
	struct ssdfs_volume_header *vh = &seg_hdr->volume_hdr;
	struct ssdfs_metadata_descriptor *desc;
	u32 offset;
	u32 size;
	u32 pagesize;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u\n",
		  state->id, peb_id, state->peb.log_offset);

	desc = &hdr->desc_array[SSDFS_LOG_FOOTER_INDEX];

	offset = le32_to_cpu(desc->offset);
	pagesize = 1 << vh->log_pagesize;

	if (offset <= state->peb.log_offset) {
		SSDFS_DBG(state->base.show_debug,
			  "unable pre-fetch block descriptors table\n");
		state->peb.log_offset += pagesize;
		state->peb.log_index++;
		return -ENODATA;
	}

	state->peb.log_size = (offset + pagesize) - state->peb.log_offset;

	desc = &seg_hdr->desc_array[SSDFS_BLK_DESC_AREA_INDEX];

	offset = le32_to_cpu(desc->offset);
	size = le32_to_cpu(desc->size);

	if (state->raw_dump.blk_desc_table == NULL) {
		state->raw_dump->blk_desc_table_size = size;
		state->raw_dump->blk_desc_table = calloc(1,
					state->raw_dump->blk_desc_table_size);
		if (!state->raw_dump->blk_desc_table) {
			SSDFS_ERR("fail to allocate buffer: "
				  "size %u, err: %s\n",
				  state->raw_dump->blk_desc_table_size,
				  strerror(errno));
			return errno;
		}
	}

	if (size > state->raw_dump.blk_desc_table_size) {
		state->raw_dump.blk_desc_table_size = size;
		state->raw_dump.blk_desc_table =
			realloc(state->raw_dump.blk_desc_table,
				state->raw_dump.blk_desc_table_size);
		if (!state->raw_dump->blk_desc_table) {
			SSDFS_ERR("fail to re-allocate buffer: "
				  "size %u, err: %s\n",
				  state->raw_dump->blk_desc_table_size,
				  strerror(errno));
			return errno;
		}

		memset(state->raw_dump->blk_desc_table, 0,
			state->raw_dump->blk_desc_table_size);
	}

	err = ssdfs_read_blk_desc_array(&state->base,
					peb_id,
					state->peb.peb_size,
					state->peb.log_offset,
					state->peb.log_size,
					offset,
					size,
					state->raw_dump->blk_desc_table);
	if (err) {
		SSDFS_ERR("fail to read block descriptors: "
			  "peb_id %llu, peb_size %u, "
			  "log_index %u, log_offset %u, err %d\n",
			  peb_id, state->base.erase_size,
			  state->peb.log_index,
			  state->peb.log_offset,
			  err);
		return err;
	}

	return 0;
}

*/



/*
static
int ssdfs_recoverfs_parse_blk_desc_table(struct ssdfs_thread_state *state,
					 u64 peb_id,
					 struct ssdfs_segment_header *seg_hdr)
{
	struct ssdfs_area_block_table *area_hdr = NULL;
	size_t area_hdr_size = sizeof(struct ssdfs_area_block_table);
	struct ssdfs_fragment_desc *frag;
	void *area_buf;
	u16 fragments_count;
	u32 parsed_bytes = 0;
	int i;
	int err;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u\n",
		  state->id, peb_id, state->peb.log_offset);

	area_buf = state->raw_dump->blk_desc_table;

	do {
		u32 compr_bytes;
		u32 uncompr_bytes;
		u16 fragments_count;
		u16 desc_size;
		u16 flags;

		area_hdr = (struct ssdfs_area_block_table *)((u8 *)area_buf +
								parsed_bytes);

		compr_bytes = le32_to_cpu(area_hdr->chain_hdr.compr_bytes);
		uncompr_bytes = le32_to_cpu(area_hdr->chain_hdr.uncompr_bytes);
		fragments_count =
			le16_to_cpu(area_hdr->chain_hdr.fragments_count);
		desc_size = le16_to_cpu(area_hdr->chain_hdr.desc_size);
		flags = le16_to_cpu(area_hdr->chain_hdr.flags);

		parsed_bytes += area_hdr_size;

		if (fragments_count > SSDFS_FRAGMENTS_CHAIN_MAX) {
			fragments_count = SSDFS_FRAGMENTS_CHAIN_MAX;
		}

		for (i = 0; i < fragments_count; i++) {
			u8 *data;

			frag = &area_hdr->blk[i];



			//ssdfs_dumpfs_parse_fragment_header(env, frag);

			if ((area_size - parsed_bytes) < le16_to_cpu(frag->compr_size)) {
				SSDFS_ERR("size %u is lesser than %u\n",
					  area_size - parsed_bytes,
					  le16_to_cpu(frag->compr_size));
				return -EINVAL;
			}

			data = (u8 *)area_buf + parsed_bytes;

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







	} while (parsed_bytes < state->raw_dump->blk_desc_table_size);


}

*/

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

	err = snprintf(state->name_buf, sizeof(state->name_buf),
			"%llu", timestamp);
	if (err < 0) {
		SSDFS_ERR("fail to convert %llu into string: %s\n",
			  timestamp, strerror(errno));
		return err;
	}

	err = mkdirat(state->output_fd, state->name_buf, 0777);
	if (errno == EEXIST) {
		/* ignore error */
		err = 0;
	} else if (errno) {
		SSDFS_ERR("fail to create folder %s, err %d, errno %d: %s\n",
			  state->name_buf, err, errno, strerror(errno));
		return errno;
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



// ssdfs_recoverfs_get_next_phys_offset(state);




static
int ssdfs_recoverfs_parse_full_log(struct ssdfs_thread_state *state,
				   u64 peb_id)
{
	struct ssdfs_segment_header *seg_hdr = NULL;
	struct ssdfs_log_footer *footer = NULL;
	struct ssdfs_signature *magic = NULL;
	struct ssdfs_metadata_descriptor *meta_desc = NULL;
	struct ssdfs_raw_area *area_desc = NULL;
	struct ssdfs_raw_buffer *raw_buf = NULL;
//	struct ssdfs_phys_offset_descriptor *phys_off;
	u64 offset;
	u32 size;
	u32 peb_size = state->peb.peb_size;
	u64 timestamp;
	u32 latest_area_offset = 0;
	u32 latest_area_size = 0;
	u32 next_log_index = state->peb.log_index + 1;
	u32 i;
	int err = 0;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u\n",
		  state->id, peb_id, state->peb.log_offset);

	seg_hdr = SSDFS_SEG_HDR(state->raw_dump.seg_hdr.buffer.ptr);

	timestamp = le64_to_cpu(seg_hdr->timestamp);

	err = ssdfs_recoverfs_create_folder(state, timestamp);
	if (err) {
		SSDFS_ERR("fail to parse full log: "
			  "thread %d, PEB %llu, log_offset %u, err %d\n",
			  state->id, peb_id, state->peb.log_offset, err);
		return err;
	}

	for (i = 0; i < SSDFS_SEG_HDR_DESC_MAX; i++) {
		meta_desc = &seg_hdr->desc_array[i];
		area_desc = &state->raw_dump.desc[i].area;

		offset = le32_to_cpu(meta_desc->offset);
		size = le32_to_cpu(meta_desc->size);

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
						  state->id, peb_id,
						  state->peb.log_offset,
						  i, offset, size);
				}
			}
		}
	}

	i = SSDFS_LOG_FOOTER_INDEX;
	area_desc = &state->raw_dump.desc[i].area;

	if (area_desc->offset >= U64_MAX) {
		next_log_index =
			ssdfs_recoverfs_define_next_log_index(state,
							latest_area_offset,
							latest_area_size);
	} else {
		raw_buf = &state->raw_dump.desc[i].buffer;
		size = min_t(u32, area_desc->size, raw_buf->size);

		err = ssdfs_read_log_footer(&state->base,
					    peb_id,
					    state->peb.peb_size,
					    area_desc->offset,
					    size,
					    raw_buf->ptr);
		if (err) {
			SSDFS_ERR("fail to read PEB's footer: "
				  "peb_id %llu, peb_size %u, "
				  "area_offset %llu, err %d\n",
				  peb_id, state->peb.peb_size,
				  area_desc->offset, err);
			return err;
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

	area_desc = &state->raw_dump.desc[SSDFS_OFF_TABLE_INDEX].area;

	if (area_desc->offset >= peb_size || area_desc->size == 0) {
		SSDFS_DBG(state->base.show_debug,
			  "block2offset area descriptor is corrupted: "
			  "thread %d, PEB %llu, log_offset %u\n",
			  state->id, peb_id, state->peb.log_offset);
		goto finish_log_parsing;
	}

/* TODO: pre-fetch blk2off table headers */


	area_desc = &state->raw_dump.desc[SSDFS_BLK_DESC_AREA_INDEX].area;

	if (area_desc->offset >= peb_size || area_desc->size == 0) {
		SSDFS_DBG(state->base.show_debug,
			  "block descriptors area descriptor is corrupted: "
			  "thread %d, PEB %llu, log_offset %u\n",
			  state->id, peb_id, state->peb.log_offset);
		goto finish_log_parsing;
	}

/* TODO: pre-fetch block descriptor table headers */

/* TODO: log parsing */

/*
	while (phys_off = ssdfs_recoverfs_get_next_phys_offset(state)) {
		if (PTR_ERR(phys_off) < 0) {
			err = PTR_ERR(phys_off);
			SSDFS_ERR("fail to get next physical offset: "
				  "err %d\n", err);
			goto finish_log_parsing;
		}


	};
*/


finish_log_parsing:
	state->peb.log_index = next_log_index;

	return err;



/* TODO: implement */






/*
	err = ssdfs_recoverfs_prefetch_blk_desc_table(state, peb_id, seg_hdr);
	if (err == -ENODATA) {
		SSDFS_DBG(state->base.show_debug,
			  "unable pre-fetch block descriptors table\n");
		return err;
	} else if (err) {
		SSDFS_ERR("fail to pre-fetch block descriptors table: "
			  "err %d\n", err);
		return err;
	}
*/

}




static
int ssdfs_recoverfs_parse_partial_log(struct ssdfs_thread_state *state,
				      u64 peb_id)
{
	struct ssdfs_partial_log_header *pl_hdr = NULL;
	struct ssdfs_metadata_descriptor *meta_desc = NULL;
	struct ssdfs_raw_area *area_desc = NULL;
	u64 offset;
	u32 size;
	u32 peb_size = state->peb.peb_size;
	u64 timestamp;
	u32 latest_area_offset = 0;
	u32 next_log_index = state->peb.log_index + 1;
	u32 i;
	int err;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu, log_offset %u\n",
		  state->id, peb_id, state->peb.log_offset);

	pl_hdr = SSDFS_PL_HDR(state->raw_dump.seg_hdr.buffer.ptr);

	timestamp = le64_to_cpu(pl_hdr->timestamp);

	err = ssdfs_recoverfs_create_folder(state, timestamp);
	if (err) {
		SSDFS_ERR("fail to parse full log: "
			  "thread %d, PEB %llu, log_offset %u, err %d\n",
			  state->id, peb_id, state->peb.log_offset, err);
		return err;
	}

	for (i = 0; i < SSDFS_SEG_HDR_DESC_MAX; i++) {
		meta_desc = &pl_hdr->desc_array[i];
		area_desc = &state->raw_dump.desc[i].area;

		offset = le32_to_cpu(meta_desc->offset);
		size = le32_to_cpu(meta_desc->size);

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
						  state->id, peb_id,
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





/* TODO: log parsing */


	state->peb.log_index = next_log_index;

	return 0;


/* TODO: implement */
}






static
int ssdfs_recoverfs_process_peb(struct ssdfs_thread_state *state,
				u64 peb_id)
{
	struct ssdfs_segment_header *seg_hdr = NULL;
	struct ssdfs_signature *magic;
	u32 logs_count = state->base.erase_size / SSDFS_4KB;
	int err;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu\n",
		  state->id, peb_id);

	state->peb.log_index = 0;

	do {
		err = ssdfs_recoverfs_find_valid_log(state, peb_id);
		if (err == -ENODATA) {
			/* PEB %llu has none valid log */
			return 0;
		} else if (err) {
			SSDFS_ERR("fail to find valid PEB: "
				  "PEB %llu, err %d\n",
				  peb_id, err);
			return err;
		}

		seg_hdr = SSDFS_SEG_HDR(state->raw_dump.seg_hdr.buffer.ptr);
		magic = &seg_hdr->volume_hdr.magic;

		if (le32_to_cpu(magic->common) == SSDFS_SUPER_MAGIC &&
		    le16_to_cpu(magic->key) == SSDFS_SEGMENT_HDR_MAGIC) {
			/* parse full log */
			err = ssdfs_recoverfs_parse_full_log(state, peb_id);
			if (err) {
				SSDFS_DBG(state->base.show_debug,
					  "unable to parse full log: "
					  "peb_id %llu, log_offset %u, "
					  "err %d\n",
					  peb_id,
					  state->peb.log_offset,
					  err);
			}
		} else if (le32_to_cpu(magic->common) == SSDFS_SUPER_MAGIC &&
			   le16_to_cpu(magic->key) ==
						SSDFS_PARTIAL_LOG_HDR_MAGIC) {
			/* parse partial log */
			err = ssdfs_recoverfs_parse_partial_log(state, peb_id);
			if (err) {
				SSDFS_DBG(state->base.show_debug,
					  "unable to parse partial log: "
					  "peb_id %llu, log_offset %u, "
					  "err %d\n",
					  peb_id,
					  state->peb.log_offset,
					  err);
			}
		} else if (le32_to_cpu(magic->common) == SSDFS_SUPER_MAGIC &&
			   le16_to_cpu(magic->key) == SSDFS_LOG_FOOTER_MAGIC) {
			SSDFS_DBG(state->base.show_debug,
				  "found orphaned footer: "
				  "PEB %llu, offset %u\n",
				  peb_id, state->peb.log_offset);
			state->peb.log_index++;
			continue;
		} else {
			SSDFS_ERR("unexpected state: "
				  "PEB %llu, offset %u\n",
				  peb_id, state->peb.log_offset);
			return -ERANGE;
		}

//		state->peb.log_index++;
	} while (state->peb.log_index < logs_count);

	return 0;
}

void *ssdfs_recoverfs_process_peb_range(void *arg)
{
	struct ssdfs_thread_state *state = (struct ssdfs_thread_state *)arg;
	u64 per_1_percent = 0;
	u64 message_threshold = 0;
	u64 i;
	int err;

	if (!state)
		pthread_exit((void *)1);

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu\n",
		  state->id, state->peb.id);

	state->err = 0;

	per_1_percent = state->peb.pebs_count / 100;
	if (per_1_percent == 0)
		per_1_percent = 1;

	message_threshold = per_1_percent;

	SSDFS_RECOVERFS_INFO(state->base.show_info,
			     "thread %d, PEB %llu, percentage %u\n",
			     state->id, state->peb.id, 0);

	for (i = 0; i < state->peb.pebs_count; i++) {
		u64 peb_id = state->peb.id + i;

		if (i >= message_threshold) {
			SSDFS_RECOVERFS_INFO(state->base.show_info,
				     "thread %d, PEB %llu, percentage %llu\n",
				     state->id, peb_id,
				     (i / per_1_percent));

			message_threshold += per_1_percent;
		}

		err = ssdfs_recoverfs_process_peb(state, peb_id);
		if (err) {
			SSDFS_ERR("fail to process PEB: "
				  "peb_id %llu, err %d\n",
				  peb_id, err);
		}
	}

	SSDFS_RECOVERFS_INFO(state->base.show_info,
			     "FINISHED: thread %d\n",
			     state->id);

	pthread_exit((void *)0);
}

int main(int argc, char *argv[])
{
	struct ssdfs_recoverfs_environment env = {
		.base.show_debug = SSDFS_FALSE,
		.base.show_info = SSDFS_TRUE,
		.base.erase_size = SSDFS_128KB,
		.base.fs_size = 0,
		.base.device_type = SSDFS_DEVICE_TYPE_MAX,
		.threads = SSDFS_RECOVERFS_DEFAULT_THREADS,
		.output_fd = -1,
	};
	union ssdfs_log_header buf;
	struct ssdfs_thread_state *jobs = NULL;
	u64 pebs_count;
	u64 pebs_per_thread;
	u32 logs_count;
	int i;
	void *res;
	int err = 0;

	parse_options(argc, argv, &env);

	SSDFS_DBG(env.base.show_debug,
		  "options have been parsed\n");

	env.base.dev_name = argv[optind];
	env.output_folder = argv[optind + 1];

	SSDFS_RECOVERFS_INFO(env.base.show_info,
			     "[001]\tOPEN DEVICE...\n");

	err = open_device(&env.base, 0);
	if (err)
		exit(EXIT_FAILURE);

	env.output_fd = open(env.output_folder, O_DIRECTORY);
	if (env.output_fd == -1) {
		close(env.base.fd);
		SSDFS_ERR("unable to open %s: %s\n",
			  env.output_folder, strerror(errno));
		exit(EXIT_FAILURE);
	}

	SSDFS_RECOVERFS_INFO(env.base.show_info,
			     "[001]\t[SUCCESS]\n");

	SSDFS_RECOVERFS_INFO(env.base.show_info,
			     "[002]\tFIND FIRST VALID PEB...\n");

	err = ssdfs_find_any_valid_peb(&env.base, &buf.seg_hdr);
	if (err) {
		SSDFS_ERR("unable to find any valid PEB\n");
		goto close_device;
	}

	SSDFS_RECOVERFS_INFO(env.base.show_info,
			     "[002]\t[SUCCESS]\n");

	env.base.erase_size = 1 << buf.seg_hdr.volume_hdr.log_erasesize;

	pebs_count = env.base.fs_size / env.base.erase_size;
	pebs_per_thread = pebs_count / env.threads;

	SSDFS_RECOVERFS_INFO(env.base.show_info,
			     "[003]\tCREATE THREADS...\n");

	jobs = calloc(env.threads,
			sizeof(struct ssdfs_thread_state));
	if (!jobs) {
		err = -ENOMEM;
		SSDFS_ERR("fail to allocate threads pool: %s\n",
			  strerror(errno));
		goto close_device;
	}

	logs_count = env.base.erase_size / SSDFS_4KB;

	for (i = 0; i < env.threads; i++) {
		jobs[i].id = i;
		jobs[i].err = 0;

		memcpy(&jobs[i].base, &env.base,
			sizeof(struct ssdfs_environment));

		jobs[i].peb.id = (u64)i * pebs_per_thread;
		jobs[i].peb.pebs_count = min_t(u64,
						pebs_count - jobs[i].peb.id,
						pebs_per_thread);
		jobs[i].peb.peb_size = env.base.erase_size;
		jobs[i].peb.log_offset = U32_MAX;
		jobs[i].peb.log_size = U32_MAX;
		jobs[i].peb.log_index = 0;
		jobs[i].peb.logs_count = logs_count;

		jobs[i].raw_dump.peb_offset = U64_MAX;

		err = ssdfs_create_raw_buffers(&jobs[i].base,
						&jobs[i].raw_dump);
		if (err) {
			SSDFS_ERR("fail to allocate raw buffers: "
				  "erase_size %u, index %d, err %d\n",
				  env.base.erase_size, i, err);
			goto free_threads_pool;
		}

		jobs[i].output_folder = env.output_folder;
		jobs[i].output_fd = env.output_fd;

		err = pthread_create(&jobs[i].thread, NULL,
				     ssdfs_recoverfs_process_peb_range,
				     (void *)&jobs[i]);
		if (err) {
			SSDFS_ERR("fail to create thread %d: %s\n",
				  i, strerror(errno));
			for (i--; i >= 0; i--) {
				pthread_join(jobs[i].thread, &res);
			}
			goto free_threads_pool;
		}
	}

	SSDFS_RECOVERFS_INFO(env.base.show_info,
			     "[003]\t[SUCCESS]\n");

	SSDFS_RECOVERFS_INFO(env.base.show_info,
			     "[004]\tWAITING THREADS...\n");

	for (i = 0; i < env.threads; i++) {
		err = pthread_join(jobs[i].thread, &res);
		if (err) {
			SSDFS_ERR("fail to finish thread %d: %s\n",
				  i, strerror(errno));
			continue;
		}

		if ((long)res != 0) {
			SSDFS_ERR("thread %d has failed: res %lu\n",
				  i, (long)res);
			continue;
		}

		if (jobs[i].err != 0) {
			SSDFS_ERR("thread %d has failed: err %d\n",
				  i, jobs[i].err);
		}
	}

	SSDFS_RECOVERFS_INFO(env.base.show_info,
			     "[004]\t[SUCCESS]\n");

free_threads_pool:
	if (jobs) {
		for (i = 0; i < env.threads; i++) {
			ssdfs_destroy_raw_buffers(&jobs[i].raw_dump);
		}
		free(jobs);
	}

close_device:
	close(env.base.fd);
	close(env.output_fd);
	exit(err ? EXIT_FAILURE : EXIT_SUCCESS);
}
