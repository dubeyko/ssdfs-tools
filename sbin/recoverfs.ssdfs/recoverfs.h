//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/recoverfs.ssdfs/recoverfs.h - declarations of recoverfs utility.
 *
 * Copyright (c) 2022-2023 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#ifndef _SSDFS_UTILS_RECOVERFS_H
#define _SSDFS_UTILS_RECOVERFS_H

#ifdef recoverfs_fmt
#undef recoverfs_fmt
#endif

#include "version.h"

#define recoverfs_fmt(fmt) "recoverfs.ssdfs: " SSDFS_UTILS_VERSION ": " fmt

#include <pthread.h>
#include <dirent.h>

#include "ssdfs_tools.h"

#define SSDFS_RECOVERFS_INFO(show, fmt, ...) \
	do { \
		if (show) { \
			fprintf(stdout, recoverfs_fmt(fmt), ##__VA_ARGS__); \
		} \
	} while (0)

#define SSDFS_RECOVERFS_DEFAULT_THREADS		(1)
#define SSDFS_FILE_NAME_DELIMITER		('-')
#define SSDFS_EMPTY_FOLDER_DEFAULT_ITEMS_COUNT	(2)

/*
 * struct ssdfs_recoverfs_environment - recoverfs environment
 * @base: basic environment
 * @threads: threads environment
 * @output_folder: output folder environment
 * @timestamp: timestamp defining the state of files
 */
struct ssdfs_recoverfs_environment {
	struct ssdfs_environment base;
	struct ssdfs_threads_environment threads;
	struct ssdfs_folder_environment output_folder;
	struct ssdfs_time_range timestamp;
};

#define SSDFS_DOT_FOLDER_NAME		(".")
#define SSDFS_DOTDOT_FOLDER_NAME	("..")

/* Inline functions */

static inline
int is_pagesize_valid(int pagesize)
{
	switch (pagesize) {
	case SSDFS_4KB:
	case SSDFS_8KB:
	case SSDFS_16KB:
	case SSDFS_32KB:
	case SSDFS_64KB:
	case SSDFS_128KB:
		/* do nothing: proper value */
		break;

	default:
		return SSDFS_FALSE;
	}

	return SSDFS_TRUE;
}

static inline
void ssdfs_init_thread_descriptor(struct ssdfs_thread_state *state,
				  int thread_id)
{
	state->id = thread_id;
	state->err = 0;
}

static inline
void ssdfs_init_base_environment(struct ssdfs_thread_state *state,
				 struct ssdfs_environment *base)
{
	memcpy(&state->base, base, sizeof(struct ssdfs_environment));
}

static inline
void ssdfs_init_peb_environment(struct ssdfs_thread_state *state,
				int thread_id, u64 pebs_per_thread,
				u64 pebs_count, u32 erase_size,
				u32 logs_count)
{
	state->peb.id = (u64)thread_id * pebs_per_thread;
	state->peb.pebs_count = min_t(u64,
					pebs_count - state->peb.id,
					pebs_per_thread);
	state->peb.peb_size = erase_size;
	state->peb.log_offset = U32_MAX;
	state->peb.log_size = U32_MAX;
	state->peb.log_index = 0;
	state->peb.logs_count = logs_count;
}

static inline
int ssdfs_init_raw_dump_environment(struct ssdfs_thread_state *state,
				    int thread_id)
{
	int err;

	state->raw_dump.peb_offset = U64_MAX;

	err = ssdfs_create_raw_dump_environment(&state->base, &state->raw_dump);
	if (err) {
		SSDFS_ERR("fail to allocate raw buffers: "
			  "erase_size %u, index %d, err %d\n",
			  state->base.erase_size, thread_id, err);
		return err;
	}

	return 0;
}

static inline
void ssdfs_init_folder_descriptors(struct ssdfs_thread_state *state,
				   const char *output_folder,
				   int output_fd)
{
	ssdfs_init_folder_environment(&state->output_folder);
	state->output_folder.name = output_folder;
	state->output_folder.fd = output_fd;

	ssdfs_init_folder_environment(&state->checkpoint_folder);
}

static inline
void ssdfs_init_file_descriptors(struct ssdfs_thread_state *state)
{
	ssdfs_init_file_environment(&state->data_file);
}

static inline
void ssdfs_init_timestamp(struct ssdfs_thread_state *state,
			  struct ssdfs_time_range *timestamp)
{
	memcpy(&state->timestamp, timestamp,
		sizeof(struct ssdfs_time_range));
}

static inline
void ssdfs_init_item_descriptors(struct ssdfs_thread_state *state)
{
	memset(state->name_buf, 0, sizeof(state->name_buf));
}

static inline
int ssdfs_init_thread_state(struct ssdfs_thread_state *state,
			    int thread_id,
			    struct ssdfs_environment *base,
			    u64 pebs_per_thread,
			    u64 pebs_count,
			    u32 erase_size,
			    u32 logs_count,
			    const char *output_folder,
			    int output_fd,
			    struct ssdfs_time_range *timestamp)
{
	int err;

	ssdfs_init_thread_descriptor(state, thread_id);
	ssdfs_init_base_environment(state, base);
	ssdfs_init_peb_environment(state, thread_id, pebs_per_thread,
				   pebs_count, erase_size, logs_count);

	err = ssdfs_init_raw_dump_environment(state, thread_id);
	if (err) {
		SSDFS_ERR("fail to init raw dump environment: "
			  "err %d\n", err);
		return err;
	}

	ssdfs_init_folder_descriptors(state, output_folder, output_fd);
	ssdfs_init_file_descriptors(state);
	ssdfs_init_timestamp(state, timestamp);
	ssdfs_init_item_descriptors(state);

	return 0;
}

static inline
int IS_CONTENT_VALID(struct ssdfs_folder_environment *folder, int index)
{
	if (folder->content.count <= 0)
		return SSDFS_FALSE;

	if (folder->content.namelist == NULL)
		return SSDFS_FALSE;

	if (index >= folder->content.count)
		return SSDFS_FALSE;

	return SSDFS_TRUE;
}


static inline
const char *FOLDER_NAME(struct ssdfs_folder_environment *folder,
			int index)
{
	BUG_ON(!IS_CONTENT_VALID(folder, index));

	return folder->content.namelist[index]->d_name;
}

static inline
const char *FILE_NAME(struct ssdfs_folder_environment *folder,
		      int index)
{
	BUG_ON(!IS_CONTENT_VALID(folder, index));

	return folder->content.namelist[index]->d_name;
}

static inline
int IS_FOLDER(struct ssdfs_folder_environment *folder, int index)
{
	BUG_ON(!IS_CONTENT_VALID(folder, index));

	return folder->content.namelist[index]->d_type == DT_DIR;
}

static inline
int IS_FILE(struct ssdfs_folder_environment *folder, int index)
{
	BUG_ON(!IS_CONTENT_VALID(folder, index));

	return folder->content.namelist[index]->d_type == DT_REG;
}

static inline
int IS_DOT_FOLDER(struct ssdfs_folder_environment *folder, int index)
{
	BUG_ON(!IS_CONTENT_VALID(folder, index));

	if (strcmp(FOLDER_NAME(folder, index), SSDFS_DOT_FOLDER_NAME) == 0)
		return SSDFS_TRUE;

	return SSDFS_FALSE;
}

static inline
int IS_DOTDOT_FOLDER(struct ssdfs_folder_environment *folder, int index)
{
	BUG_ON(!IS_CONTENT_VALID(folder, index));

	if (strcmp(FOLDER_NAME(folder, index), SSDFS_DOTDOT_FOLDER_NAME) == 0)
		return SSDFS_TRUE;

	return SSDFS_FALSE;
}

static inline
int ssdfs_recoverfs_prepare_name_list(struct ssdfs_folder_environment *folder)
{
	folder->content.count = scandir(folder->name,
					&folder->content.namelist,
					NULL, alphasort);
	if (folder->content.count == -1) {
		SSDFS_ERR("fail to scan output folder: %s\n",
			  strerror(errno));
		return errno;
	}

	return 0;
}

static inline
void ssdfs_wait_threads_activity_ending(struct ssdfs_recoverfs_environment *env)
{
	int i;

	SSDFS_DBG(env->base.show_debug,
		  "requested_jobs %u, tasks_capacity %u\n",
		  env->threads.requested_jobs, env->threads.capacity);

	for (i = 0; i < env->threads.requested_jobs; i++) {
		pthread_join(env->threads.jobs[i].thread, NULL);

		if (env->threads.jobs[i].err != 0) {
			SSDFS_ERR("thread %d has failed: err %d\n",
				  i, env->threads.jobs[i].err);
		}
	}
}

/* Application APIs */

/* delete_folder.c */
int ssdfs_recoverfs_delete_folder(struct ssdfs_recoverfs_environment *env,
				  const char *folder_name);

/* file_synthesis.c */
int ssdfs_recoverfs_build_files_in_folder(struct ssdfs_recoverfs_environment *env,
					  const char *folder_name);

/* options.c */
void print_usage(void);
void parse_options(int argc, char *argv[],
		   struct ssdfs_recoverfs_environment *env);

/* peb_processing.c */
int ssdfs_recoverfs_process_peb(struct ssdfs_thread_state *state);

#endif /* _SSDFS_UTILS_RECOVERFS_H */
