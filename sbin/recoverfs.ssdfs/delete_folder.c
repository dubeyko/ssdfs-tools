/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/recoverfs.ssdfs/delete_folder.c - implementation of delete folder logic.
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

static
int ssdfs_delete_non_empty_folder(struct ssdfs_recoverfs_environment *env,
				  const char *path)
{
	struct ssdfs_folder_environment parent;
	int index;
	int err = 0;

	SSDFS_DBG(env->base.show_debug,
		  "path %s\n",
		  path);

	memset(&parent, 0, sizeof(struct ssdfs_folder_environment));
	parent.name = path;

	parent.fd = open(path, O_DIRECTORY, 0777);
	if (parent.fd < 1) {
		SSDFS_ERR("unable to open %s: %s\n",
			  path, strerror(errno));
		return errno;
	}

	err = ssdfs_recoverfs_prepare_name_list(&parent);
	if (err) {
		SSDFS_ERR("fail to scan folder %s: err %d\n",
			  parent.name, err);
		goto close_parent_folder;
	}

	for (index = 0; index < parent.content.count; index++) {
		if (IS_FILE(&parent, index)) {
			err = unlinkat(parent.fd, FILE_NAME(&parent, index), 0);
			if (err) {
				err = errno;
				SSDFS_ERR("unable to delete file %s: %s\n",
					  FILE_NAME(&parent, index),
					  strerror(errno));
			}
		}
	}

close_parent_folder:
	close(parent.fd);

	if (err)
		return err;

	err = rmdir(parent.name);
	if (err) {
		err = errno;
		SSDFS_ERR("fail to delete %s: %s\n",
			  parent.name, strerror(errno));
		return err;
	}

	return 0;
}

int ssdfs_recoverfs_delete_folder(struct ssdfs_recoverfs_environment *env,
				  const char *folder_name)
{
	char path[SSDFS_MAX_NAME_LEN];
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "folder_name %s\n",
		  folder_name);

	memset(path, 0, sizeof(path));

	err = snprintf(path, sizeof(path) - 1,
			"%s/%s",
			env->output_folder.name,
			folder_name);
	if (err < 0) {
		SSDFS_ERR("fail to prepare string: %s\n",
			  strerror(errno));
		return err;
	}

	err = rmdir(path);
	if (err) {
		err = errno;

		if (err == ENOTEMPTY ||  err == EEXIST) {
			err = ssdfs_delete_non_empty_folder(env, path);
			if (err) {
				SSDFS_ERR("fail to delete folder: "
					  "%s, err %d\n",
					  path, err);
				return err;
			}
		} else {
			SSDFS_ERR("fail to delete %s: %s\n",
				  path, strerror(errno));
			return errno;
		}
	}

	return 0;
}
