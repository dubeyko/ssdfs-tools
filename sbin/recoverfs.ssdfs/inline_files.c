/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/recoverfs.ssdfs/inline_files.c - implementation of inline files
 *                                         extraction logic.
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

int ssdfs_recoverfs_find_first_valid_node(struct ssdfs_recoverfs_environment *env,
					  int fd,
					  u32 *node_size,
					  u32 *node_offset,
					  u32 *nodes_count)
{
	struct stat stat;
	struct ssdfs_inodes_btree_node_header *node_hdr;
	u8 buffer[PAGE_CACHE_SIZE];
	u64 file_size = 0;
	u32 pagesize = env->base.page_size;
	off_t offset;
	ssize_t read_bytes = 0;
	u64 rest_bytes;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "output_folder %s, fd %d\n",
		  env->output_folder.name, fd);

	*node_size = U32_MAX;
	*node_offset = U32_MAX;
	*nodes_count = 0;

	memset(buffer, 0, sizeof(buffer));

	err = fstat(fd, &stat);
	if (err) {
		SSDFS_ERR("unable to get file status: %s\n",
			  strerror(errno));
		return errno;
	}

	switch (stat.st_mode & S_IFMT) {
	case S_IFREG:
		/* regular file */
		file_size = stat.st_size;
		break;

	default:
		SSDFS_ERR("unexpected file type\n");
		return -ERANGE;
	}

	if (file_size == 0) {
		SSDFS_DBG(env->base.show_debug,
			  "empty file: output_folder %s, fd %d\n",
			  env->output_folder.name, fd);
		return -ENOENT;
	}

	*node_offset = 0;

	while (*node_offset < file_size) {
		offset = lseek(fd, *node_offset, SEEK_SET);
		if (offset < 0) {
			err = errno;
			SSDFS_ERR("fail to set offset %u in file: %s\n",
				  *node_offset, strerror(errno));
			return err;
		} else if (offset != *node_offset) {
			SSDFS_ERR("fail to set offset %u in file\n",
				  *node_offset);
			return -ERANGE;
		}

		read_bytes = read(fd, buffer, PAGE_CACHE_SIZE);
		if (read_bytes < 0) {
			err = errno;
			SSDFS_ERR("unable to read file: %s\n",
				  strerror(errno));
			return err;
		} else if (read_bytes != PAGE_CACHE_SIZE) {
			SSDFS_ERR("unable to read the whole portion: "
				  "read_bytes %zd\n",
				  read_bytes);
			return -EIO;
		}

		node_hdr = (struct ssdfs_inodes_btree_node_header *)buffer;

		if (le32_to_cpu(node_hdr->node.magic.common) ==
						SSDFS_SUPER_MAGIC &&
		    le16_to_cpu(node_hdr->node.magic.key) ==
						SSDFS_INODES_BNODE_MAGIC) {
			*node_size = 1 << node_hdr->node.log_node_size;

			if (*node_size == 0 || *node_size % pagesize) {
				SSDFS_ERR("invalid node size %u\n",
					  *node_size);
				return -EIO;
			}

			rest_bytes = file_size - *node_offset;

			*nodes_count = rest_bytes / *node_size;

			SSDFS_DBG(env->base.show_debug,
				  "output_folder %s, fd %d, "
				  "nodes_count %u\n",
				  env->output_folder.name, fd,
				  *nodes_count);

			return 0;
		}

		*node_offset += pagesize;
	}

	return -ENOENT;
}

int ssdfs_recoverfs_node_extract_inline_file(struct ssdfs_recoverfs_environment *env,
					     int fd,
					     u32 node_offset,
					     u32 node_size,
					     u8 *buffer)
{
	struct ssdfs_inodes_btree_node_header *node_hdr = NULL;
	struct ssdfs_inode *raw_inode = NULL;
	char name[SSDFS_MAX_NAME_LEN];
	ssize_t read_bytes = 0;
	ssize_t written_bytes = 0;
	u32 item_area_offset;
	u32 cur_offset;
	off_t offset;
	u32 item_area_size;
	u32 items_capacity;
	u32 item_size;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "output_folder %s, fd %d, "
		  "node_size %u, node_offset %u\n",
		  env->output_folder.name, fd,
		  node_size, node_offset);

	offset = lseek(fd, node_offset, SEEK_SET);
	if (offset < 0) {
		err = errno;
		SSDFS_ERR("fail to set offset %u in file: %s\n",
			  node_offset, strerror(errno));
		return err;
	} else if (offset != node_offset) {
		SSDFS_ERR("fail to set offset %u in file\n",
			  node_offset);
		return -ERANGE;
	}

	read_bytes = read(fd, buffer, node_size);
	if (read_bytes < 0) {
		err = errno;
		SSDFS_ERR("unable to read file: %s\n",
			  strerror(errno));
		return err;
	} else if (read_bytes != node_size) {
		SSDFS_ERR("unable to read the whole portion: "
			  "read_bytes %zd\n",
			  read_bytes);
		return -EIO;
	}

	node_hdr = (struct ssdfs_inodes_btree_node_header *)buffer;

	if (le32_to_cpu(node_hdr->node.magic.common) != SSDFS_SUPER_MAGIC ||
	    le16_to_cpu(node_hdr->node.magic.key) != SSDFS_INODES_BNODE_MAGIC) {
		SSDFS_DBG(env->base.show_debug,
			  "corruped node: "
			  "output_folder %s, fd %d, "
			  "node_size %u, node_offset %u, "
			  "magic (common %#x, key %#x)\n",
			  env->output_folder.name, fd,
			  node_size, node_offset,
			  le32_to_cpu(node_hdr->node.magic.common),
			  le16_to_cpu(node_hdr->node.magic.key));
		return -EIO;
	}

	switch (node_hdr->node.type) {
	case SSDFS_BTREE_HYBRID_NODE:
	case SSDFS_BTREE_LEAF_NODE:
		/* process the node */
		break;

	case SSDFS_BTREE_ROOT_NODE:
	case SSDFS_BTREE_INDEX_NODE:
		SSDFS_DBG(env->base.show_debug,
			  "ignore node: "
			  "output_folder %s, fd %d, "
			  "node_size %u, node_offset %u, "
			  "type %#x\n",
			  env->output_folder.name, fd,
			  node_size, node_offset,
			  node_hdr->node.type);
		return 0;

	default:
		SSDFS_ERR("corrupted node: "
			  "output_folder %s, fd %d, "
			  "node_size %u, node_offset %u, "
			  "type %#x\n",
			  env->output_folder.name, fd,
			  node_size, node_offset,
			  node_hdr->node.type);
		return -EIO;
	}

	item_area_offset = le32_to_cpu(node_hdr->node.item_area_offset);

	if (item_area_offset >= node_size) {
		SSDFS_ERR("invalid item_area_offset: "
			  "node_offset %u, node_size %u, "
			  "item_area_offset %u\n",
			  node_offset, node_size,
			  item_area_offset);
		return -EIO;
	}

	items_capacity = le16_to_cpu(node_hdr->node.items_capacity);

	if (items_capacity == 0 || items_capacity >= U16_MAX) {
		SSDFS_ERR("invalid items_capacity: "
			  "node_offset %u, node_size %u, "
			  "items_capacity %u\n",
			  node_offset, node_size,
			  items_capacity);
		return -EIO;
	}

	item_area_size = node_size - item_area_offset;
	item_size = item_area_size / items_capacity;

	if (item_size == 0) {
		SSDFS_ERR("invalid items_size: "
			  "node_offset %u, node_size %u, "
			  "items_capacity %u, item_area_size %u, "
			  "item_size %u\n",
			  node_offset, node_size,
			  items_capacity, item_area_size,
			  item_size);
		return -EIO;
	}

	cur_offset = item_area_offset;
	while ((cur_offset + item_size) < node_size) {
		u8 *ptr;
		u16 private_flags;
		u64 file_size;

		raw_inode = (struct ssdfs_inode *)(buffer + cur_offset);

		private_flags = le16_to_cpu(raw_inode->private_flags);

		if (private_flags & SSDFS_INODE_HAS_INLINE_FILE) {
			file_size = le64_to_cpu(raw_inode->size);

			if (file_size == 0 || file_size >= U64_MAX) {
				/* ignore file */
				SSDFS_DBG(env->base.show_debug,
					  "corruped node: "
					  "output_folder %s, fd %d, "
					  "file_size %llu\n",
					  env->output_folder.name, fd,
					  file_size);
			} else {
				u64 ino;
				int data_fd;

				ino = le64_to_cpu(raw_inode->ino);

				memset(name, 0, sizeof(name));

				snprintf(name, sizeof(name) - 1,
					 "%llu", ino);

				data_fd = openat(env->output_folder.fd,
						 name,
						 O_CREAT | O_RDWR | O_LARGEFILE,
						 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
				if (data_fd < 0) {
					err = errno;
					SSDFS_ERR("unable to create %s: %s\n",
						  name, strerror(errno));
					return err;
				}

				ptr = raw_inode->internal[0].area1.inline_stream.bytes;
				written_bytes = write(data_fd, ptr, file_size);
				if (written_bytes < 0) {
					err = errno;
					SSDFS_ERR("fail to write: %s\n",
						  strerror(errno));
				} else if (written_bytes != file_size) {
					SSDFS_ERR("unable to write the whole portion: "
						  "written_bytes %zd, file_size %llu\n",
						  written_bytes, file_size);
				}

				if (fsync(data_fd) < 0) {
					err = errno;
					SSDFS_ERR("fail to sync: %s\n",
						  strerror(errno));
				}

				close(data_fd);
			}
		}

		cur_offset += item_size;
	}

	return 0;
}
