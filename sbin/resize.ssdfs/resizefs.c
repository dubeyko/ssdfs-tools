//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/resizefs.c - implementation of resizefs.ssdfs (volume resizing) utility.
 *
 * Copyright (c) 2014-2018 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 *
 * HGST Confidential
 * (C) Copyright 2009-2018, HGST, Inc., All rights reserved.
 *
 * Created by HGST, San Jose Research Center, Storage Architecture Group
 * Authors: Vyacheslav Dubeyko <slava@dubeyko.com>
 *
 * Acknowledgement: Cyril Guyot <Cyril.Guyot@wdc.com>
 *                  Zvonimir Bandic <Zvonimir.Bandic@wdc.com>
 */

#include <stdio.h>

#include "ssdfs_tools.h"

static void print_version(void)
{
	SSDFS_INFO("resizefs.ssdfs, part of %s\n", SSDFS_UTILS_VERSION);
}

int main()
{
	print_version();
	return 0;
}
