/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/recoverfs.ssdfs/recoverfs.c - implementation of recoverfs.ssdfs
 *                                    (volume recovering) utility.
 *
 * Copyright (c) 2020-2026 Viacheslav Dubeyko <slava@dubeyko.com>
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
#include "snapshot.h"

static
int ssdfs_recoverfs_open_output_folder(struct ssdfs_recoverfs_environment *env)
{
	struct ssdfs_folder_environment *parent;
	int index;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "output_folder %s\n",
		  env->output_folder.name);

	env->output_folder.fd = open(env->output_folder.name, O_DIRECTORY);
	if (env->output_folder.fd == -1) {
		err = mkdir(env->output_folder.name, 0777);
		if (err < 0) {
			close(env->base.fd);
			SSDFS_ERR("unable to create folder %s: %s\n",
				  env->output_folder.name, strerror(errno));
			return errno;
		}

		env->output_folder.fd = open(env->output_folder.name, O_DIRECTORY);
		if (env->output_folder.fd == -1) {
			close(env->base.fd);
			SSDFS_ERR("unable to open %s: %s\n",
				  env->output_folder.name, strerror(errno));
			return errno;
		}
	}

	parent = &env->output_folder;

	err = ssdfs_recoverfs_prepare_name_list(parent);
	if (err) {
		SSDFS_ERR("fail to scan output folder: err %d\n",
			  err);
		return err;
	}

	for (index = 0; index < parent->content.count; index++) {
		free(parent->content.namelist[index]);
	}

	free(parent->content.namelist);

	if (parent->content.count > SSDFS_EMPTY_FOLDER_DEFAULT_ITEMS_COUNT) {
		SSDFS_ERR("Output folder %s is not empty!!!! "
			  "Please, prepare empty folder.\n",
			  env->output_folder.name);
		return -EEXIST;
	}

	return 0;
}

void *ssdfs_recoverfs_process_peb_range(void *arg)
{
	struct ssdfs_thread_state *state = (struct ssdfs_thread_state *)arg;
	u64 per_1_percent = 0;
	u64 message_threshold = 0;
	u64 start_peb_id;
	u64 i;
	int err;

	if (!state)
		pthread_exit((void *)1);

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, PEB %llu\n",
		  state->id, state->peb.id);

	state->err = 0;
	start_peb_id = state->peb.id;

	per_1_percent = state->peb.pebs_count / 100;
	if (per_1_percent == 0)
		per_1_percent = 1;

	message_threshold = per_1_percent;

	SSDFS_RECOVERFS_INFO(state->base.show_info,
			     "thread %d, PEB %llu, percentage %u\n",
			     state->id, state->peb.id, 0);

	for (i = 0; i < state->peb.pebs_count; i++) {
		state->peb.id = start_peb_id + i;

		if (i >= message_threshold) {
			SSDFS_RECOVERFS_INFO(state->base.show_info,
				     "thread %d, PEB %llu, percentage %llu\n",
				     state->id, state->peb.id,
				     (i / per_1_percent));

			message_threshold += per_1_percent;
		}

		err = ssdfs_recoverfs_process_peb(state);
		if (err) {
			SSDFS_ERR("fail to process PEB: "
				  "peb_id %llu, err %d\n",
				  state->peb.id, err);
		}
	}

	SSDFS_RECOVERFS_INFO(state->base.show_info,
			     "FINISHED: thread %d\n",
			     state->id);

	pthread_exit((void *)0);
}

static
int ssdfs_recoverfs_synthesize_files(struct ssdfs_recoverfs_environment *env)
{
	struct ssdfs_folder_environment *parent;
	int index;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "output_folder %s\n",
		  env->output_folder.name);

	parent = &env->output_folder;

	err = ssdfs_recoverfs_prepare_name_list(parent);
	if (err) {
		SSDFS_ERR("fail to scan output folder: err %d\n",
			  err);
		return err;
	}

	for (index = 0; index < parent->content.count; index++) {
		if (IS_DOT_FOLDER(parent, index)) {
			/* do nothing */
		} else if (IS_DOTDOT_FOLDER(parent, index)) {
			/* do nothing */
		} else if (IS_FOLDER(parent, index)) {
			u64 timestamp = atoll(FOLDER_NAME(parent, index));

			if (is_timestamp_inside_range(&env->timestamp, timestamp)) {
				SSDFS_DBG(env->base.show_debug,
					  "timestamp %s\n",
					  ssdfs_nanoseconds_to_time(timestamp));

				err = ssdfs_recoverfs_build_files_in_folder(env,
						    FOLDER_NAME(parent, index));
				if (err) {
					SSDFS_ERR("fail to process folder %s: "
						  "err %d\n",
						  FOLDER_NAME(parent, index),
						  err);
				}
			}
		}
	}

	for (index = 0; index < parent->content.count; index++) {
		if (IS_DOT_FOLDER(parent, index)) {
			/* do nothing */
		} else if (IS_DOTDOT_FOLDER(parent, index)) {
			/* do nothing */
		} else {
			err = ssdfs_recoverfs_delete_folder(env,
						FOLDER_NAME(parent, index));
			if (err) {
				SSDFS_ERR("fail to delete %s, err %d\n",
					  FOLDER_NAME(parent, index),
					  err);
			}
		}
	}

	for (index = 0; index < parent->content.count; index++) {
		free(parent->content.namelist[index]);
	}

	free(parent->content.namelist);

	return 0;
}

static
int ssdfs_recoverfs_extract_inline_files(struct ssdfs_recoverfs_environment *env)
{
	char name[SSDFS_MAX_NAME_LEN];
	u8 *buffer = NULL;
	int fd;
	u32 node_size;
	u32 node_offset;
	u32 nodes_count = 0;
	int i;
	int err = 0;

	SSDFS_DBG(env->base.show_debug,
		  "output_folder %s\n",
		  env->output_folder.name);

	memset(name, 0, sizeof(name));

	snprintf(name, sizeof(name) - 1,
		 "%u", SSDFS_INODES_BTREE_INO);

	fd = openat(env->output_folder.fd,
		    name,
		    O_RDWR | O_LARGEFILE,
		    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd < 0) {
		err = errno;
		SSDFS_ERR("unable to open %s: %s\n",
			  name, strerror(errno));
		return err;
	}

	err = ssdfs_recoverfs_find_first_valid_node(env, fd,
						    &node_size,
						    &node_offset,
						    &nodes_count);
	if (err == -ENOENT) {
		err = 0;
		SSDFS_DBG(env->base.show_debug,
			  "unable to find any valid node: "
			  "output_folder %s, file %s\n",
			  env->output_folder.name,
			  name);
		goto close_file;
	} else if (err) {
		SSDFS_ERR("fail to find valid node: "
			  "output_folder %s, file %s\n",
			  env->output_folder.name,
			  name);
		goto close_file;
	}

	if (nodes_count == 0) {
		err = 0;
		SSDFS_DBG(env->base.show_debug,
			  "nodes_count %u, "
			  "output_folder %s, file %s\n",
			  nodes_count,
			  env->output_folder.name,
			  name);
		goto close_file;
	} else if (nodes_count >= U32_MAX) {
		err = -ERANGE;
		SSDFS_ERR("fail to calculate nodes count\n");
		goto close_file;
	}

	buffer = malloc(node_size);
	if (!buffer) {
		err = -ENOMEM;
		SSDFS_ERR("fail to allocate buffer: "
			  "node_size %u\n",
			  node_size);
		goto close_file;
	}

	memset(buffer, 0, node_size);

	for (i = 0; i < nodes_count; i++) {
		err = ssdfs_recoverfs_node_extract_inline_file(env, fd,
								node_offset,
								node_size,
								buffer);
		if (err) {
			SSDFS_ERR("fail to process node: "
				  "index %d, node_offset %u, err %d\n",
				  i, node_offset, err);
		}

		node_offset += node_size;
	}

	free(buffer);

close_file:
	close(fd);

	return err;
}

int main(int argc, char *argv[])
{
	struct ssdfs_recoverfs_environment env = {
		.base.show_debug = SSDFS_FALSE,
		.base.show_info = SSDFS_TRUE,
		.base.erase_size = SSDFS_128KB,
		.base.page_size = SSDFS_4KB,
		.base.fs_size = 0,
		.base.device_type = SSDFS_DEVICE_TYPE_MAX,
		.threads.jobs = NULL,
		.threads.capacity = SSDFS_RECOVERFS_DEFAULT_THREADS,
		.threads.requested_jobs = 0,
		.output_folder.name = NULL,
		.output_folder.fd = -1,
		.output_folder.content.namelist = NULL,
		.output_folder.content.count = 0,
		.timestamp.minute = SSDFS_ANY_MINUTE,
		.timestamp.hour = SSDFS_ANY_HOUR,
		.timestamp.day = SSDFS_ANY_DAY,
		.timestamp.month = SSDFS_ANY_MONTH,
		.timestamp.year = SSDFS_ANY_YEAR,
	};
	union ssdfs_metadata_header buf;
	u64 pebs_count;
	u64 pebs_per_thread;
	u32 logs_count;
	int i;
	int err = 0;

	parse_options(argc, argv, &env);

	SSDFS_DBG(env.base.show_debug,
		  "options have been parsed\n");

	env.base.dev_name = argv[optind];
	env.output_folder.name = argv[optind + 1];

	SSDFS_RECOVERFS_INFO(env.base.show_info,
			     "[001]\tOPEN DEVICE...\n");

	err = open_device(&env.base, 0);
	if (err)
		exit(EXIT_FAILURE);

	err = ssdfs_recoverfs_open_output_folder(&env);
	if (err)
		exit(EXIT_FAILURE);

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
	env.base.page_size = 1 << buf.seg_hdr.volume_hdr.log_pagesize;

	pebs_count = env.base.fs_size / env.base.erase_size;
	pebs_per_thread = (pebs_count + env.threads.capacity - 1);
	pebs_per_thread /= env.threads.capacity;

	SSDFS_RECOVERFS_INFO(env.base.show_info,
			     "[003]\tCREATE THREADS...\n");

	env.threads.jobs = calloc(env.threads.capacity,
				  sizeof(struct ssdfs_thread_state));
	if (!env.threads.jobs) {
		err = -ENOMEM;
		SSDFS_ERR("fail to allocate threads pool: %s\n",
			  strerror(errno));
		goto close_device;
	}

	logs_count = env.base.erase_size / SSDFS_4KB;

	for (i = 0; i < env.threads.capacity; i++) {
		err = ssdfs_init_thread_state(&env.threads.jobs[i],
					      i,
					      &env.base,
					      pebs_per_thread,
					      pebs_count,
					      env.base.erase_size,
					      logs_count,
					      env.output_folder.name,
					      env.output_folder.fd,
					      &env.timestamp);
		if (err) {
			SSDFS_ERR("fail to initialize thread state: "
				  "index %d, err %d\n",
				  i, err);
			goto free_threads_pool;
		}

		err = pthread_create(&env.threads.jobs[i].thread, NULL,
				     ssdfs_recoverfs_process_peb_range,
				     (void *)&env.threads.jobs[i]);
		if (err) {
			SSDFS_ERR("fail to create thread %d: %s\n",
				  i, strerror(errno));
			for (i--; i >= 0; i--) {
				pthread_join(env.threads.jobs[i].thread, NULL);
				env.threads.requested_jobs--;
			}
			goto free_threads_pool;
		}

		env.threads.requested_jobs++;
	}

	SSDFS_RECOVERFS_INFO(env.base.show_info,
			     "[003]\t[SUCCESS]\n");

	SSDFS_RECOVERFS_INFO(env.base.show_info,
			     "[004]\tWAITING THREADS...\n");

	ssdfs_wait_threads_activity_ending(&env);
	env.threads.requested_jobs = 0;

	SSDFS_RECOVERFS_INFO(env.base.show_info,
			     "[004]\t[SUCCESS]\n");

	SSDFS_RECOVERFS_INFO(env.base.show_info,
			     "[005]\tBUILD FILES...\n");

	err = ssdfs_recoverfs_synthesize_files(&env);
	if (err) {
		SSDFS_ERR("fail to synthesize files in folder %s: err %d\n",
			  env.output_folder.name, err);
		goto free_threads_pool;
	}

	SSDFS_RECOVERFS_INFO(env.base.show_info,
			     "[005]\t[SUCCESS]\n");

	SSDFS_RECOVERFS_INFO(env.base.show_info,
			     "[006]\tEXTRACT INLINE FILES...\n");

	err = ssdfs_recoverfs_extract_inline_files(&env);
	if (err) {
		SSDFS_ERR("fail to extract inline files: err %d\n",
			  err);
		goto free_threads_pool;
	}

	SSDFS_RECOVERFS_INFO(env.base.show_info,
			     "[006]\t[SUCCESS]\n");

free_threads_pool:
	if (env.threads.jobs) {
		for (i = 0; i < env.threads.capacity; i++) {
			ssdfs_destroy_raw_dump_environment(&env.threads.jobs[i].raw_dump);
		}
		free(env.threads.jobs);
	}

close_device:
	close(env.base.fd);
	close(env.output_folder.fd);
	exit(err ? EXIT_FAILURE : EXIT_SUCCESS);
}
