//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/test.ssdfs/testing.c - implementation of testing utility.
 *
 * Copyright (c) 2021 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
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
	testing_env.files_number_threshold =
				SSDFS_TESTFS_DEFAULT_FILE_COUNT_MAX;
	testing_env.file_size_threshold = (u64)SSDFS_1GB * 1024;
	testing_env.extent_len_threshold = SSDFS_TESTFS_DEFAULT_EXTENT_LEN;

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
