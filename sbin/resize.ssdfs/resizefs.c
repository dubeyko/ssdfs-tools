/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/resize.ssdfs/resizefs.c - implementation of volume resizing utility.
 *
 * Copyright (c) 2014-2019 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 * Copyright (c) 2014-2025 Viacheslav Dubeyko <slava@dubeyko.com>
 *              http://www.ssdfs.org/
 *
 * (C) Copyright 2014-2019, HGST, Inc., All rights reserved.
 *
 * Created by HGST, San Jose Research Center, Storage Architecture Group
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 *
 * Acknowledgement: Cyril Guyot
 *                  Zvonimir Bandic
 */

#include <stdio.h>

#include "ssdfs_tools.h"
#include "resizefs.h"

int main(int argc, char *argv[])
{
	struct ssdfs_resizefs_environment env = {
		.generic.show_debug = SSDFS_FALSE,
		.generic.show_info = SSDFS_TRUE,
		.generic.device_type = SSDFS_DEVICE_TYPE_MAX,
		.grow_option.state = SSDFS_IGNORE_OPTION,
		.shrink_option.state = SSDFS_IGNORE_OPTION,
		.new_size.state = SSDFS_IGNORE_OPTION,
		.volume_label.state = SSDFS_IGNORE_OPTION,
		.need_make_snapshot = SSDFS_FALSE,
		.check_by_fsck = SSDFS_FALSE,
		.force_resize = SSDFS_FALSE,
		.rollback_resize = SSDFS_FALSE,
	};
	int err = 0;

	parse_options(argc, argv, &env);

	SSDFS_RESIZEFS_INFO(env.generic.show_info,
			    "functionality is under implementation yet!!!\n");

	exit(err ? EXIT_FAILURE : EXIT_SUCCESS);
}
