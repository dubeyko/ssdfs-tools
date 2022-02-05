//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/recoverfs.c - implementation of recoverfs.ssdfs
 *                    (volume recovering) utility.
 *
 * Copyright (c) 2020-2022 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#include <stdio.h>

#include "ssdfs_tools.h"

static void print_version(void)
{
	SSDFS_INFO("recoverfs.ssdfs, part of %s\n", SSDFS_UTILS_VERSION);
}

int main()
{
	print_version();
	return 0;
}
