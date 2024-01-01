/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/dump.ssdfs/show_granularity.c - show granularity command.
 *
 * Copyright (c) 2014-2019 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 * Copyright (c) 2014-2024 Viacheslav Dubeyko <slava@dubeyko.com>
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

#include "dumpfs.h"

/************************************************************************
 *                     Show granularity command                         *
 ************************************************************************/

int ssdfs_dumpfs_show_granularity(struct ssdfs_dumpfs_environment *env)
{
	struct ssdfs_segment_header sg_buf;
	int err;

	SSDFS_DBG(env->base.show_debug,
		  "command: %#x\n",
		  env->command);

	SSDFS_DUMPFS_INFO(env->base.show_info,
			  "[002]\tFIND FIRST VALID PEB...\n");

	err = ssdfs_dumpfs_find_any_valid_peb(env, &sg_buf);
	if (err)
		return err;

	SSDFS_DUMPFS_INFO(env->base.show_info,
			  "[002]\t[SUCCESS]\n");

	ssdfs_dumpfs_show_key_volume_details(env, &sg_buf);

	return 0;
}
