/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/test.ssdfs/testing.c - implementation of testing utility.
 *
 * Copyright (c) 2021-2026 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#define _LARGEFILE64_SOURCE
#define __USE_FILE_OFFSET64
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

#include "testing.h"

int main(int argc, char *argv[])
{
	int fd;
	struct ssdfs_testing_environment testing_env;
	int err = 0;

	testing_env.subsystems = 0;
	testing_env.page_size = SSDFS_TESTFS_DEFAULT_PAGE_SIZE;

	testing_env.dentries_tree.files_number_threshold =
				SSDFS_TESTFS_DEFAULT_FILE_COUNT_MAX;

	testing_env.extents_tree.file_size_threshold = (u64)SSDFS_1GB * 1024;
	testing_env.extents_tree.extent_len_threshold =
						SSDFS_TESTFS_DEFAULT_EXTENT_LEN;

	testing_env.block_bitmap.capacity =
					SSDFS_TESTFS_DEFAULT_BLK_BMAP_CAPACITY;
	testing_env.block_bitmap.pre_alloc_blks_per_iteration =
					SSDFS_TESTFS_DEFAULT_PRE_ALLOC_PER_ITER;
	testing_env.block_bitmap.alloc_blks_per_iteration =
					SSDFS_TESTFS_DEFAULT_ALLOC_PER_ITER;
	testing_env.block_bitmap.invalidate_blks_per_iteration =
				SSDFS_TESTFS_DEFAULT_INVALIDATE_PER_ITER;
	testing_env.block_bitmap.reserved_metadata_blks_per_iteration =
					SSDFS_TESTFS_DEFAULT_RESERVED_PER_ITER;

	testing_env.blk2off_table.capacity =
				SSDFS_TESTFS_DEFAULT_BLK2OFF_TBL_CAPACITY;

	testing_env.mapping_table.iterations_number =
				SSDFS_TESTFS_DEFAULT_MAPPING_TBL_ITERATIONS;
	testing_env.mapping_table.peb_mappings_per_iteration =
				SSDFS_TESTFS_DEFAULT_MAPPINGS_PER_ITER;
	testing_env.mapping_table.add_migrations_per_iteration =
				SSDFS_TESTFS_DEFAULT_ADD_MIGRATIONS_PER_ITER;
	testing_env.mapping_table.exclude_migrations_per_iteration =
				SSDFS_TESTFS_DEFAULT_DELETE_MIGRATIONS_PER_ITER;

	testing_env.memory_primitives.iterations_number =
				SSDFS_TESTFS_DEFAULT_MEM_PRIMITIVES_ITERATIONS;
	testing_env.memory_primitives.capacity =
				SSDFS_TESTFS_DEFAULT_MEM_PRIMITIVES_CAPACITY;
	testing_env.memory_primitives.count =
				SSDFS_TESTFS_DEFAULT_MEM_PRIMITIVES_COUNT;
	testing_env.memory_primitives.item_size =
				SSDFS_TESTFS_DEFAULT_MEM_PRIMITIVES_ITEM_SIZE;
	testing_env.memory_primitives.test_types = 0;

	testing_env.segment_bitmap.iterations_number =
				SSDFS_TESTFS_DEFAULT_SEGBMAP_ITERATIONS;
	testing_env.segment_bitmap.using_segs_per_iteration =
				SSDFS_TESTFS_DEFAULT_USING_SEGS_PER_ITER;
	testing_env.segment_bitmap.used_segs_per_iteration =
				SSDFS_TESTFS_DEFAULT_USED_SEGS_PER_ITER;
	testing_env.segment_bitmap.pre_dirty_segs_per_iteration =
				SSDFS_TESTFS_DEFAULT_PRE_DIRTY_SEGS_PER_ITER;
	testing_env.segment_bitmap.dirty_segs_per_iteration =
				SSDFS_TESTFS_DEFAULT_DIRTY_SEGS_PER_ITER;
	testing_env.segment_bitmap.cleaned_segs_per_iteration =
				SSDFS_TESTFS_DEFAULT_CLEANED_SEGS_PER_ITER;

	testing_env.shared_dictionary.names_number =
				SSDFS_TESTFS_DEFAULT_LONG_NAMES_NUMBER;
	testing_env.shared_dictionary.name_len =
				SSDFS_TESTFS_DEFAULT_LONG_NAME_LENGTH;
	testing_env.shared_dictionary.step_factor =
				SSDFS_TESTFS_DEFAULT_NAME_STEP_FACTOR;

	testing_env.xattr_tree.xattrs_number =
				SSDFS_TESTFS_DEFAULT_XATTRS_NUMBER;
	testing_env.xattr_tree.name_len =
				SSDFS_TESTFS_DEFAULT_LONG_NAME_LENGTH;
	testing_env.xattr_tree.step_factor =
				SSDFS_TESTFS_DEFAULT_NAME_STEP_FACTOR;
	testing_env.xattr_tree.blob_len =
				SSDFS_TESTFS_DEFAULT_XATTR_BLOB_LEN;
	testing_env.xattr_tree.blob_pattern =
				SSDFS_TESTFS_DEFAULT_XATTR_BLOB_PATTERN;

	testing_env.shextree.extents_number_threshold =
				SSDFS_TESTFS_DEFAULT_SHARED_EXTENTS_NUMBER;
	testing_env.shextree.extent_len =
				SSDFS_TESTFS_DEFAULT_SHARED_EXTENT_LENGTH;
	testing_env.shextree.ref_count_threshold =
				SSDFS_TESTFS_DEFAULT_SHARED_EXTENT_REFS_MAX;

	testing_env.snapshots_tree.snapshots_number_threshold =
				SSDFS_TESTFS_DEFAULT_SNAPSHOTS_NUMBER;

	parse_options(argc, argv, &testing_env);

	fd = open(argv[optind], O_RDWR | O_LARGEFILE);
	if (fd == -1) {
		SSDFS_ERR("unable to open %s: %s\n",
			  argv[optind], strerror(errno));
		exit(EXIT_FAILURE);
	}

	err = ioctl(fd, SSDFS_IOC_DO_TESTING, &testing_env);
	if (err) {
		SSDFS_ERR("ioctl failed for %s: %s\n",
			  argv[optind], strerror(errno));
		goto testfs_failed;
	}

testfs_failed:
	close(fd);
	exit(err ? EXIT_FAILURE : EXIT_SUCCESS);
}
