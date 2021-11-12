//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/snapshotfs.c - implementation of snapshot management utility.
 *
 * Copyright (c) 2014-2021 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 *
 * HGST Confidential
 * (C) Copyright 2014-2021, HGST, Inc., All rights reserved.
 *
 * Created by HGST, San Jose Research Center, Storage Architecture Group
 * Authors: Vyacheslav Dubeyko <slava@dubeyko.com>
 *
 * Acknowledgement: Cyril Guyot <Cyril.Guyot@wdc.com>
 *                  Zvonimir Bandic <Zvonimir.Bandic@wdc.com>
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

#include "snapshotfs.h"

static
void ssdfs_set_default_create_options(struct ssdfs_snapshot_options *options)
{
	options->create.name = NULL;
	options->create.mode = SSDFS_READ_ONLY_SNAPSHOT;
	options->create.type = SSDFS_ONE_TIME_SNAPSHOT;
	options->create.expiration = SSDFS_NEVER_EXPIRED;
	options->create.frequency = SSDFS_UNKNOWN_FREQUENCY;
	options->create.existing_snapshots = SSDFS_INFINITE_SNAPSHOTS_NUMBER;
}

static
void ssdfs_prepare_create_snapshot_info(struct ssdfs_snapshot_options *options,
					struct ssdfs_snapshot_info *info)
{
	if (options->create.name)
		memcpy(info->name, options->create.name, SSDFS_MAX_NAME_LEN);

	info->mode = options->create.mode;
	info->type = options->create.type;
	info->expiration = options->create.expiration;
	info->frequency = options->create.frequency;
	info->existing_snapshots = options->create.existing_snapshots;
}

static
void ssdfs_set_default_list_options(struct ssdfs_snapshot_options *options)
{
	options->list.time_range.day = SSDFS_ANY_DAY;
	options->list.time_range.month = SSDFS_ANY_MONTH;
	options->list.time_range.year = SSDFS_ANY_YEAR;
	options->list.mode = SSDFS_UNKNOWN_SNAPSHOT_MODE;
	options->list.type = SSDFS_UNKNOWN_SNAPSHOT_TYPE;
	options->list.max_number = SSDFS_INFINITE_SNAPSHOTS_NUMBER;
}

static
void ssdfs_prepare_list_snapshot_info(struct ssdfs_snapshot_options *options,
					struct ssdfs_snapshot_info *info)
{
	memcpy(&info->time_range, &options->list.time_range,
		sizeof(struct ssdfs_time_range));
	info->mode = options->list.mode;
	info->type = options->list.type;
}

static
void ssdfs_set_default_modify_options(struct ssdfs_snapshot_options *options)
{
	options->modify.name = NULL;
	options->modify.id = NULL;
	options->modify.mode = SSDFS_UNKNOWN_SNAPSHOT_MODE;
	options->modify.type = SSDFS_UNKNOWN_SNAPSHOT_TYPE;
	options->modify.expiration = SSDFS_UNKNOWN_EXPIRATION_POINT;
	options->modify.frequency = SSDFS_UNKNOWN_FREQUENCY;
	options->modify.existing_snapshots = SSDFS_UNDEFINED_SNAPSHOTS_NUMBER;
}

static
void ssdfs_prepare_modify_snapshot_info(struct ssdfs_snapshot_options *options,
					struct ssdfs_snapshot_info *info)
{
	if (options->modify.name)
		memcpy(info->name, options->modify.name, SSDFS_MAX_NAME_LEN);

	if (options->modify.id)
		memcpy(info->uuid, options->modify.id, SSDFS_UUID_SIZE);

	info->mode = options->modify.mode;
	info->type = options->modify.type;
	info->expiration = options->modify.expiration;
	info->frequency = options->modify.frequency;
	info->existing_snapshots = options->modify.existing_snapshots;
}

static
void ssdfs_set_default_remove_options(struct ssdfs_snapshot_options *options)
{
	options->remove.name = NULL;
	options->remove.id = NULL;
}

static
void ssdfs_prepare_remove_snapshot_info(struct ssdfs_snapshot_options *options,
					struct ssdfs_snapshot_info *info)
{
	if (options->remove.name)
		memcpy(info->name, options->remove.name, SSDFS_MAX_NAME_LEN);

	if (options->remove.id)
		memcpy(info->uuid, options->remove.id, SSDFS_UUID_SIZE);
}

static void
ssdfs_set_default_remove_range_options(struct ssdfs_snapshot_options *options)
{
	options->remove_range.time_range.day = SSDFS_ANY_DAY;
	options->remove_range.time_range.month = SSDFS_ANY_MONTH;
	options->remove_range.time_range.year = SSDFS_ANY_YEAR;
}

static void
ssdfs_prepare_remove_range_snapshot_info(struct ssdfs_snapshot_options *options,
					 struct ssdfs_snapshot_info *info)
{
	memcpy(&info->time_range, &options->remove_range.time_range,
		sizeof(struct ssdfs_time_range));
}

static void
ssdfs_set_default_show_details_options(struct ssdfs_snapshot_options *options)
{
	options->show_details.name = NULL;
	options->show_details.id = NULL;
}

static void
ssdfs_prepare_show_details_snapshot_info(struct ssdfs_snapshot_options *options,
					 struct ssdfs_snapshot_info *info)
{
	if (options->show_details.name) {
		memcpy(info->name, options->show_details.name,
			SSDFS_MAX_NAME_LEN);
	}

	if (options->show_details.id)
		memcpy(info->uuid, options->show_details.id, SSDFS_UUID_SIZE);
}

int main(int argc, char *argv[])
{
	int fd;
	struct ssdfs_snapshot_options options;
	struct ssdfs_snapshot_info info;
	int err = 0;

	ssdfs_set_default_create_options(&options);
	ssdfs_set_default_list_options(&options);
	ssdfs_set_default_modify_options(&options);
	ssdfs_set_default_remove_options(&options);
	ssdfs_set_default_remove_range_options(&options);
	ssdfs_set_default_show_details_options(&options);

	parse_options(argc, argv, &options);

	SSDFS_DBG(options.show_debug,
		  "options have been parsed\n");

	memset(&info, 0, sizeof(info));

	SSDFS_DBG(options.show_debug,
		  "try to open file/folder\n");

	fd = open(argv[optind], O_DIRECTORY);
	if (fd == -1) {
		fd = open(argv[optind], O_RDWR | O_LARGEFILE);
		if (fd == -1) {
			SSDFS_ERR("unable to open %s: %s\n",
				  argv[optind], strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	SSDFS_DBG(options.show_debug,
		  "execute operation\n");

	switch (options.operation) {
	case SSDFS_CREATE_SNAPSHOT:
		ssdfs_prepare_create_snapshot_info(&options, &info);

		err = ioctl(fd, SSDFS_IOC_CREATE_SNAPSHOT, &info);
		if (err) {
			SSDFS_ERR("ioctl failed for %s: %s\n",
				  argv[optind], strerror(errno));
			goto snapshotfs_failed;
		}

		err = syncfs(fd);
		if (err) {
			SSDFS_ERR("syncfs failed for %s: %s\n",
				  argv[optind], strerror(errno));
			goto snapshotfs_failed;
		}
		break;

	case SSDFS_LIST_SNAPSHOTS:
		ssdfs_prepare_list_snapshot_info(&options, &info);

		err = ioctl(fd, SSDFS_IOC_LIST_SNAPSHOTS, &info);
		if (err) {
			SSDFS_ERR("ioctl failed for %s: %s\n",
				  argv[optind], strerror(errno));
			goto snapshotfs_failed;
		}
		break;

	case SSDFS_MODIFY_SNAPSHOT:
		ssdfs_prepare_modify_snapshot_info(&options, &info);

		err = ioctl(fd, SSDFS_IOC_MODIFY_SNAPSHOT, &info);
		if (err) {
			SSDFS_ERR("ioctl failed for %s: %s\n",
				  argv[optind], strerror(errno));
			goto snapshotfs_failed;
		}
		break;

	case SSDFS_REMOVE_SNAPSHOT:
		ssdfs_prepare_remove_snapshot_info(&options, &info);

		err = ioctl(fd, SSDFS_IOC_REMOVE_SNAPSHOT, &info);
		if (err) {
			SSDFS_ERR("ioctl failed for %s: %s\n",
				  argv[optind], strerror(errno));
			goto snapshotfs_failed;
		}
		break;

	case SSDFS_REMOVE_RANGE:
		ssdfs_prepare_remove_range_snapshot_info(&options, &info);

		err = ioctl(fd, SSDFS_IOC_REMOVE_RANGE, &info);
		if (err) {
			SSDFS_ERR("ioctl failed for %s: %s\n",
				  argv[optind], strerror(errno));
			goto snapshotfs_failed;
		}
		break;

	case SSDFS_SHOW_SNAPSHOT_DETAILS:
		ssdfs_prepare_show_details_snapshot_info(&options, &info);

		err = ioctl(fd, SSDFS_IOC_SHOW_DETAILS, &info);
		if (err) {
			SSDFS_ERR("ioctl failed for %s: %s\n",
				  argv[optind], strerror(errno));
			goto snapshotfs_failed;
		}
		break;

	default:
		err = -ERANGE;
		SSDFS_ERR("unknown operation %#x\n",
			  options.operation);
		goto snapshotfs_failed;
	}

	SSDFS_DBG(options.show_debug,
		  "operation has been executed\n");

snapshotfs_failed:
	close(fd);
	exit(err ? EXIT_FAILURE : EXIT_SUCCESS);
}
