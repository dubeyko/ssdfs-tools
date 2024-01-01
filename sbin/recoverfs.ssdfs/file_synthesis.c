/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/recoverfs.ssdfs/file_synthesis.c - implementation of files
 *                                         synthesis logic.
 *
 * Copyright (c) 2023-2024 Viacheslav Dubeyko <slava@dubeyko.com>
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
u64 ssdfs_recoverfs_extract_inode_id(struct ssdfs_environment *base,
				     const char *file_name)
{
	char inode_id_string[SSDFS_MAX_NAME_LEN];
	const char *delimiter;
	size_t len;

	SSDFS_DBG(base->show_debug,
		  "file_name %s\n",
		  file_name);

	memset(inode_id_string, 0, sizeof(inode_id_string));

	delimiter = strchr(file_name, SSDFS_FILE_NAME_DELIMITER);

	if (file_name < delimiter) {
		len = delimiter - file_name;

		strncat(inode_id_string, file_name, len);

		SSDFS_DBG(base->show_debug,
			  "INODE %s\n",
			  inode_id_string);

		return atoll(inode_id_string);
	}

	SSDFS_ERR("corrupted file name\n");

	return U64_MAX;
}

static inline
u64 ssdfs_recoverfs_extract_offset(struct ssdfs_thread_state *state,
				   const char *file_name)
{
	char byte_offset_string[SSDFS_MAX_NAME_LEN];
	const char *delimiter;
	size_t len;
	u64 offset;

	SSDFS_DBG(state->base.show_debug,
		  "file_name %s\n",
		  file_name);

	memset(byte_offset_string, 0, sizeof(byte_offset_string));

	delimiter = strchr(file_name, SSDFS_FILE_NAME_DELIMITER);

	if (file_name < delimiter) {
		delimiter++;

		len = delimiter - file_name;
		len = strlen(file_name) - len;

		strncat(byte_offset_string, delimiter, len);

		offset = atoll(byte_offset_string);
		offset *= state->base.page_size;

		SSDFS_DBG(state->base.show_debug,
			  "BYTE OFFSET %llu\n",
			  offset);

		return offset;
	}

	SSDFS_ERR("corrupted file name\n");

	return U64_MAX;
}

static
int ssdfs_recoverfs_prepare_file_buffer(struct ssdfs_file_environment *env,
					u32 buf_size)
{
	if (env->content.buffer == NULL) {
		env->content.buffer = calloc(1, buf_size);
		if (!env->content.buffer) {
			SSDFS_ERR("fail to allocate buffer: "
				  "size %u, err: %s\n",
				  buf_size,
				  strerror(errno));
			return errno;
		}
		env->content.size = buf_size;
	} else if (buf_size > env->content.size) {
		env->content.buffer = realloc(env->content.buffer, buf_size);
		if (!env->content.buffer) {
			SSDFS_ERR("fail to re-allocate buffer: "
				  "size %u, err: %s\n",
				  buf_size,
				  strerror(errno));
			return errno;
		}
		env->content.size = buf_size;
	}

	memset(env->content.buffer, 0, env->content.size);

	return 0;
}

static
int ssdfs_recoverfs_create_data_file(struct ssdfs_thread_state *state)
{
	int err;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, inode_id %llu\n",
		  state->id, state->data_file.inode_id);

	memset(state->name_buf, 0, sizeof(state->name_buf));

	snprintf(state->name_buf, sizeof(state->name_buf) - 1,
		 "%llu", state->data_file.inode_id);

	/*
	 * Check that file is absent
	 */
	state->data_file.fd = openat(state->output_folder.fd,
				     state->name_buf,
				     O_RDWR | O_LARGEFILE,
				     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (state->data_file.fd < 0) {
		state->data_file.fd = openat(state->output_folder.fd,
					     state->name_buf,
					     O_CREAT | O_RDWR | O_LARGEFILE,
					     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	}

	if (state->data_file.fd < 0) {
		err = errno;
		SSDFS_ERR("unable to create/open %s: %s\n",
			  state->name_buf, strerror(errno));
		return err;
	}

	return 0;
}

static
int ssdfs_recoverfs_synthesize_blocks(struct ssdfs_thread_state *state,
					const char *folder_name)
{
	struct ssdfs_folder_environment parent;
	int index;
	u64 inode_id;
	u64 byte_offset;
	int parent_fd;
	int err = 0;

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, inode_id %llu\n",
		  state->id, state->data_file.inode_id);

	err = ssdfs_recoverfs_create_data_file(state);
	if (err) {
		SSDFS_ERR("fail to create file: "
			  "inode_id %llu, err %d\n",
			  state->data_file.inode_id, err);
		return err;
	}

	memset(state->name_buf, 0, sizeof(state->name_buf));

	err = snprintf(state->name_buf, sizeof(state->name_buf) - 1,
			"%s/%s",
			state->output_folder.name,
			folder_name);
	if (err < 0) {
		SSDFS_ERR("fail to prepare string: %s\n",
			  strerror(errno));
		return err;
	}

	memset(&parent, 0, sizeof(struct ssdfs_folder_environment));
	parent.name = state->name_buf;

	parent_fd = openat(state->output_folder.fd, folder_name,
			   O_DIRECTORY, 0777);
	if (parent_fd < 1) {
		SSDFS_ERR("unable to open %s: %s\n",
			  folder_name, strerror(errno));
		return errno;
	}

	err = ssdfs_recoverfs_prepare_name_list(&parent);
	if (err) {
		SSDFS_ERR("fail to scan folder %s: err %d\n",
			  parent.name, err);
		return err;
	}

	for (index = 0; index < parent.content.count; index++) {
		struct stat file_stat;
		off_t offset;
		int fd;
		ssize_t read_bytes;
		ssize_t written_bytes;

		if (IS_FILE(&parent, index)) {
			inode_id = ssdfs_recoverfs_extract_inode_id(&state->base,
						FILE_NAME(&parent, index));
			if (inode_id >= U64_MAX) {
				SSDFS_ERR("fail to extract inode ID: "
					  "name %s\n",
					  FILE_NAME(&parent, index));
				continue;
			}

			if (inode_id != state->data_file.inode_id) {
				SSDFS_DBG(state->base.show_debug,
					  "thread %d, inode_id1 %llu, "
					  "inode_id2 %llu\n",
					  state->id,
					  state->data_file.inode_id,
					  inode_id);
				continue;
			}

			byte_offset = ssdfs_recoverfs_extract_offset(state,
						    FILE_NAME(&parent, index));
			if (byte_offset >= U64_MAX) {
				SSDFS_ERR("fail to extract byte offset: "
					  "name %s\n",
					  FILE_NAME(&parent, index));
				continue;
			}

			offset = lseek(state->data_file.fd, byte_offset, SEEK_SET);
			if (offset < 0) {
				err = errno;
				SSDFS_ERR("fail to set offset %llu in file: %s\n",
					  byte_offset, strerror(errno));
				goto finish_synthesize_blocks;
			} else if (offset != byte_offset) {
				err = -ERANGE;
				SSDFS_ERR("fail to set offset %llu in file\n",
					  byte_offset);
				goto finish_synthesize_blocks;
			}

			fd = openat(parent_fd,
				    FILE_NAME(&parent, index),
				    O_RDWR | O_LARGEFILE,
				    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if (fd < 0) {
				err = errno;
				SSDFS_ERR("unable to open %s: %s\n",
					  FILE_NAME(&parent, index),
					  strerror(errno));
				continue;
			}

			err = fstat(fd, &file_stat);
			if (err) {
				err = errno;
				SSDFS_ERR("unable to get stats of file %s: %s\n",
					  FILE_NAME(&parent, index),
					  strerror(errno));
				goto finish_block_processing;
			}

			if (file_stat.st_size == 0) {
				err = -ENODATA;
				SSDFS_ERR("invalid bytes_count %lu\n",
					  file_stat.st_size);
				goto finish_block_processing;
			}

			SSDFS_DBG(state->base.show_debug,
				  "thread %d, inode_id %llu, "
				  "offset %llu, bytes_count %lu\n",
				  state->id, state->data_file.inode_id,
				  (u64)offset, file_stat.st_size);

			err = ssdfs_recoverfs_prepare_file_buffer(&state->data_file,
								  file_stat.st_size);
			if (err) {
				SSDFS_ERR("fail to prepare buffer: "
					  "bytes_count %lu, err %d\n",
					  file_stat.st_size, err);
				goto finish_block_processing;
			}

			read_bytes = read(fd, state->data_file.content.buffer,
					  file_stat.st_size);
			if (read_bytes < 0) {
				err = errno;
				SSDFS_ERR("unable to read file %s: %s\n",
					  FILE_NAME(&parent, index),
					  strerror(errno));
				goto finish_block_processing;
			} else if (read_bytes != file_stat.st_size) {
				SSDFS_ERR("unable to read the whole file: "
					  "file_size %lu, read_bytes %zd\n",
					  file_stat.st_size, read_bytes);
			}

			written_bytes = write(state->data_file.fd,
						state->data_file.content.buffer,
						read_bytes);
			if (written_bytes < 0) {
				err = errno;
				SSDFS_ERR("fail to write: %s\n",
					  strerror(errno));
				goto finish_block_processing;
			} else if (written_bytes != read_bytes) {
				SSDFS_ERR("unable to write the whole portion: "
					  "written_bytes %zd, read_bytes %zd\n",
					  written_bytes, read_bytes);
			}

finish_block_processing:
			close(fd);

			err = unlinkat(parent_fd, FILE_NAME(&parent, index), 0);
			if (err) {
				err = errno;
				SSDFS_ERR("unable to delete file %s: %s\n",
					  FILE_NAME(&parent, index),
					  strerror(errno));
			}
		}
	}

finish_synthesize_blocks:
	for (index = 0; index < parent.content.count; index++) {
		free(parent.content.namelist[index]);
	}

	free(parent.content.namelist);

	if (fsync(state->data_file.fd) < 0) {
		err = errno;
		SSDFS_ERR("fail to sync: %s\n",
			  strerror(errno));
	}

	close(parent_fd);
	close(state->data_file.fd);

	return err;
}

void *ssdfs_recoverfs_build_file(void *arg)
{
	struct ssdfs_thread_state *state = (struct ssdfs_thread_state *)arg;
	int index;
	int err = 0;

	if (!state)
		pthread_exit((void *)1);

	SSDFS_DBG(state->base.show_debug,
		  "thread %d, inode_id %llu\n",
		  state->id, state->data_file.inode_id);

	for (index = 0; index < state->output_folder.content.count; index++) {
		if (IS_DOT_FOLDER(&state->output_folder, index)) {
			/* do nothing */
		} else if (IS_DOTDOT_FOLDER(&state->output_folder, index)) {
			/* do nothing */
		} else if (IS_FOLDER(&state->output_folder, index)) {
			u64 timestamp = atoll(FOLDER_NAME(&state->output_folder,
							  index));

			if (is_timestamp_inside_range(&state->timestamp,
						      timestamp)) {
				SSDFS_DBG(state->base.show_debug,
					  "timestamp %s\n",
					  ssdfs_nanoseconds_to_time(timestamp));

				err = ssdfs_recoverfs_synthesize_blocks(state,
						FOLDER_NAME(&state->output_folder,
							    index));
				if (err) {
					SSDFS_ERR("fail to process folder %s: "
						  "err %d\n",
						  FOLDER_NAME(&state->output_folder,
							      index),
						  err);
				}
			}
		}
	}

	if (!err) {
		SSDFS_DBG(state->base.show_debug,
			  "file processed: inode_id %llu\n",
			  state->data_file.inode_id);
		SSDFS_RECOVERFS_INFO(state->base.show_info,
				     "FILE CREATED: "
				     "inode_id %llu\n",
				     state->data_file.inode_id);
	}

	pthread_exit((void *)0);
}

static
int ssdfs_recoverfs_add_inode_id_job(struct ssdfs_recoverfs_environment *env,
				     u64 inode_id)
{
	struct ssdfs_thread_state *state;
	int thread_id;
	u64 pebs_count;
	u64 pebs_per_thread;
	u32 logs_count;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "INODE ID %llu\n",
		  inode_id);

	if (env->threads.requested_jobs >= env->threads.capacity) {
		SSDFS_ERR("requested_jobs %u >= threads.capacity %u\n",
			  env->threads.requested_jobs,
			  env->threads.capacity);
		return -EAGAIN;
	}

	pebs_count = env->base.fs_size / env->base.erase_size;
	pebs_per_thread = pebs_count / env->threads.capacity;
	logs_count = env->base.erase_size / SSDFS_4KB;

	if (env->threads.requested_jobs == 0) {
		state = &env->threads.jobs[env->threads.requested_jobs];

		err = ssdfs_init_thread_state(state,
					      env->threads.requested_jobs,
					      &env->base,
					      pebs_per_thread,
					      pebs_count,
					      env->base.erase_size,
					      logs_count,
					      env->output_folder.name,
					      env->output_folder.fd,
					      &env->timestamp);
		if (err) {
			SSDFS_ERR("fail to initialize thread state: "
				  "index %d, err %d\n",
				  env->threads.requested_jobs, err);
			return err;
		}

		state->output_folder.content.namelist =
				env->output_folder.content.namelist;
		state->output_folder.content.count =
				env->output_folder.content.count;

		state->data_file.inode_id = inode_id;

		err = pthread_create(&state->thread, NULL,
				     ssdfs_recoverfs_build_file,
				     (void *)state);
		if (err) {
			SSDFS_ERR("fail to create thread %d: %s\n",
				  env->threads.requested_jobs, strerror(errno));
			return errno;
		}

		env->threads.requested_jobs++;
	} else {
		thread_id = env->threads.requested_jobs - 1;
		state = &env->threads.jobs[thread_id];
		if (state->data_file.inode_id < inode_id) {
			thread_id = env->threads.requested_jobs;
			state = &env->threads.jobs[thread_id];

			err = ssdfs_init_thread_state(state,
						      thread_id,
						      &env->base,
						      pebs_per_thread,
						      pebs_count,
						      env->base.erase_size,
						      logs_count,
						      env->output_folder.name,
						      env->output_folder.fd,
						      &env->timestamp);
			if (err) {
				SSDFS_ERR("fail to initialize thread state: "
					  "index %d, err %d\n",
					  thread_id, err);

				for (thread_id--; thread_id >= 0; thread_id--) {
					pthread_join(env->threads.jobs[thread_id].thread,
						     NULL);
					env->threads.requested_jobs--;
				}

				return err;
			}

			state->output_folder.content.namelist =
					env->output_folder.content.namelist;
			state->output_folder.content.count =
					env->output_folder.content.count;

			state->data_file.inode_id = inode_id;

			err = pthread_create(&state->thread, NULL,
					     ssdfs_recoverfs_build_file,
					     (void *)state);
			if (err) {
				SSDFS_ERR("fail to create thread %d: %s\n",
					  thread_id, strerror(errno));
				for (thread_id--; thread_id >= 0; thread_id--) {
					pthread_join(env->threads.jobs[thread_id].thread,
						     NULL);
					env->threads.requested_jobs--;
				}

				return errno;
			}

			env->threads.requested_jobs++;
		}
	}

	return 0;
}

int ssdfs_recoverfs_build_files_in_folder(struct ssdfs_recoverfs_environment *env,
					  const char *folder_name)
{
	struct ssdfs_folder_environment parent;
	char name[SSDFS_MAX_NAME_LEN];
	u64 inode_id;
	int index;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "folder_name %s\n",
		  folder_name);

	memset(name, 0, sizeof(name));

	err = snprintf(name, sizeof(name) - 1,
			"%s/%s",
			env->output_folder.name,
			folder_name);
	if (err < 0) {
		SSDFS_ERR("fail to prepare string: %s\n",
			  strerror(errno));
		return err;
	}

	memset(&parent, 0, sizeof(struct ssdfs_folder_environment));
	parent.name = name;

	do {
		err = ssdfs_recoverfs_prepare_name_list(&parent);
		if (err) {
			SSDFS_ERR("fail to scan folder %s: err %d\n",
				  parent.name, err);
			return err;
		}

		env->threads.requested_jobs = 0;

		if (parent.content.count <= SSDFS_EMPTY_FOLDER_DEFAULT_ITEMS_COUNT)
			break;

		for (index = 0; index < parent.content.count; index++) {
			if (IS_FILE(&parent, index)) {
				inode_id = ssdfs_recoverfs_extract_inode_id(&env->base,
							    FILE_NAME(&parent, index));
				if (inode_id >= U64_MAX) {
					SSDFS_ERR("fail to extract inode ID: "
						  "name %s\n",
						  FILE_NAME(&parent, index));
				}

				if (env->threads.requested_jobs >= env->threads.capacity) {
					break;
				} else {
					err = ssdfs_recoverfs_add_inode_id_job(env,
									    inode_id);
					if (err) {
						SSDFS_ERR("fail to add inode thread: "
							  "inode_id %llu, err %d\n",
							  inode_id, err);
						ssdfs_wait_threads_activity_ending(env);
						env->threads.requested_jobs = 0;
					}
				}
			}
		}

		ssdfs_wait_threads_activity_ending(env);
		env->threads.requested_jobs = 0;

		for (index = 0; index < parent.content.count; index++) {
			free(parent.content.namelist[index]);
		}

		free(parent.content.namelist);
	} while (parent.content.count > SSDFS_EMPTY_FOLDER_DEFAULT_ITEMS_COUNT);

	return 0;
}
