/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/fsck.c - implementation of fsck.ssdfs (volume checking) utility.
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
#include <sched.h>
#include <stdio.h>
#include <unistd.h>
#include <paths.h>
#include <blkid/blkid.h>
#include <uuid/uuid.h>
#include <limits.h>
#include <pthread.h>

#include "ssdfs_tools.h"
#include "fsck.h"

static
int __get_user_answer(void)
{
	char buf[SSDFS_MAX_NAME_LEN + 1];
	int count = 0;
	int is_answer_symbol;
	int res = 0;

	memset(buf, 0, sizeof(buf));

	do {
		res = getchar();
		is_answer_symbol = res != '\n' && res != EOF;

		if (is_answer_symbol) {
			buf[count] = (unsigned char)res;
			count++;

			if (count >= SSDFS_MAX_NAME_LEN)
				break;
		}

		if (res == (int)'\n')
			break;
		else if (res == (int)EOF)
			break;
	} while (SSDFS_TRUE);

	if (strcmp(buf, SSDFS_FSCK_SHORT_YES_STRING1) == 0)
		return SSDFS_FSCK_YES_USER_ANSWER;
	else if (strcmp(buf, SSDFS_FSCK_SHORT_YES_STRING2) == 0)
		return SSDFS_FSCK_YES_USER_ANSWER;
	else if (strcmp(buf, SSDFS_FSCK_LONG_YES_STRING1) == 0)
		return SSDFS_FSCK_YES_USER_ANSWER;
	else if (strcmp(buf, SSDFS_FSCK_LONG_YES_STRING2) == 0)
		return SSDFS_FSCK_YES_USER_ANSWER;
	else if (strcmp(buf, SSDFS_FSCK_LONG_YES_STRING3) == 0)
		return SSDFS_FSCK_YES_USER_ANSWER;

	if (strcmp(buf, SSDFS_FSCK_SHORT_NO_STRING1) == 0)
		return SSDFS_FSCK_NO_USER_ANSWER;
	else if (strcmp(buf, SSDFS_FSCK_SHORT_NO_STRING2) == 0)
		return SSDFS_FSCK_NO_USER_ANSWER;
	else if (strcmp(buf, SSDFS_FSCK_LONG_NO_STRING1) == 0)
		return SSDFS_FSCK_NO_USER_ANSWER;
	else if (strcmp(buf, SSDFS_FSCK_LONG_NO_STRING2) == 0)
		return SSDFS_FSCK_NO_USER_ANSWER;
	else if (strcmp(buf, SSDFS_FSCK_LONG_NO_STRING3) == 0)
		return SSDFS_FSCK_NO_USER_ANSWER;

	return SSDFS_FSCK_UNKNOWN_USER_ANSWER;
}

static
int get_user_answer(void)
{
	int res = SSDFS_FSCK_NONE_USER_ANSWER;

	do {
		res = __get_user_answer();

		switch (res) {
		case SSDFS_FSCK_UNKNOWN_USER_ANSWER:
			SSDFS_INFO("Please, use [y|Y] or [n|N]: ");
			continue;

		default:
			/* do nothing */
			break;
		}
	} while (res == SSDFS_FSCK_UNKNOWN_USER_ANSWER ||
		 res == SSDFS_FSCK_NONE_USER_ANSWER);

	return res;
}

int main(int argc, char *argv[])
{
	struct ssdfs_fsck_environment env = {
		.base.show_debug = SSDFS_FALSE,
		.base.show_info = SSDFS_TRUE,
		.base.erase_size = SSDFS_128KB,
		.base.page_size = SSDFS_4KB,
		.base.fs_size = 0,
		.base.device_type = SSDFS_DEVICE_TYPE_MAX,
		.threads.jobs = NULL,
		.threads.capacity = SSDFS_FSCK_DEFAULT_THREADS,
		.threads.requested_jobs = 0,
		.detection_result.state = SSDFS_FSCK_UNKNOWN_DETECTION_RESULT,
		.check_result.state = SSDFS_FSCK_VOLUME_UNKNOWN_CHECK_RESULT,
		.check_result.corruption.mask = 0,
		.recovery_result.state = SSDFS_FSCK_UNKNOWN_RECOVERY_RESULT,
		.force_checking = SSDFS_FALSE,
		.no_change = SSDFS_FALSE,
		.auto_repair = SSDFS_FALSE,
		.yes_all_questions = SSDFS_FALSE,
		.be_verbose = SSDFS_FALSE,
		.seg_size = SSDFS_128KB,
	};
	int res;
	int err = 0;

	ssdfs_fsck_init_detection_result(&env);
	ssdfs_fsck_init_check_result(&env);
	ssdfs_fsck_init_recovery_result(&env);

	parse_options(argc, argv, &env);

	SSDFS_DBG(env.base.show_debug,
		  "options have been parsed\n");

	env.base.dev_name = argv[optind];

	SSDFS_FSCK_INFO(env.base.show_info,
			"[001]\tOPEN DEVICE...\n");

	err = open_device(&env.base, 0);
	if (err)
		exit(EXIT_FAILURE);

	SSDFS_FSCK_INFO(env.base.show_info,
			"[001]\t[SUCCESS]\n");
	SSDFS_FSCK_INFO(env.base.show_info,
			"[002]\tDETECT SSDFS VOLUME...\n");

	res = is_device_contains_ssdfs_volume(&env);
	switch (res) {
	case SSDFS_FSCK_DEVICE_HAS_FILE_SYSTEM:
		SSDFS_FSCK_INFO(env.base.show_info && env.be_verbose,
				"SSDFS volume has been detected on %s\n",
				env.base.dev_name);
		break;

	case SSDFS_FSCK_DEVICE_HAS_SOME_METADATA:
		SSDFS_FSCK_INFO(env.base.show_info && env.be_verbose,
				"Some SSDFS metadata have been detected on %s\n",
				env.base.dev_name);
		break;

	case SSDFS_FSCK_NO_FILE_SYSTEM_DETECTED:
		/* continue logic */
		break;

	case SSDFS_FSCK_FAILED_DETECT_FILE_SYSTEM:
		SSDFS_ERR("fail to detect SSDFS file system on %s\n",
			  env.base.dev_name);
		goto fsck_finish;

	default:
		SSDFS_ERR("unknown detection result on %s\n",
			  env.base.dev_name);
		goto fsck_finish;
	}

	SSDFS_FSCK_INFO(env.base.show_info,
			"[002]\t[SUCCESS]\n");

	if (res == SSDFS_FSCK_NO_FILE_SYSTEM_DETECTED) {
		SSDFS_FSCK_INFO(env.base.show_info,
				"No SSDFS file system has been detected on %s\n",
				env.base.dev_name);
		goto fsck_finish;
	}

	SSDFS_FSCK_INFO(env.base.show_info,
			"[003]\tCHECK SSDFS VOLUME...\n");

	res = is_ssdfs_volume_corrupted(&env);
	switch (res) {
	case SSDFS_FSCK_VOLUME_COMPLETELY_DESTROYED:
		SSDFS_FSCK_INFO(env.base.show_info && env.be_verbose,
				"SSDFS volume is completely destroyed on %s\n",
				env.base.dev_name);
		break;

	case SSDFS_FSCK_VOLUME_HEAVILY_CORRUPTED:
		SSDFS_FSCK_INFO(env.base.show_info && env.be_verbose,
				"SSDFS volume is heavily corrupted on %s\n",
				env.base.dev_name);
		break;

	case SSDFS_FSCK_VOLUME_SLIGHTLY_CORRUPTED:
		SSDFS_FSCK_INFO(env.base.show_info && env.be_verbose,
				"SSDFS volume is slightly corrupted on %s\n",
				env.base.dev_name);
		break;

	case SSDFS_FSCK_VOLUME_UNCLEAN_UMOUNT:
		SSDFS_FSCK_INFO(env.base.show_info && env.be_verbose,
				"SSDFS volume experienced unclean umount on %s\n",
				env.base.dev_name);
		break;

	case SSDFS_FSCK_VOLUME_HEALTHY:
		SSDFS_FSCK_INFO(env.base.show_info && env.be_verbose,
				"SSDFS volume is healthy on %s\n",
				env.base.dev_name);
		break;

	case SSDFS_FSCK_VOLUME_CHECK_FAILED:
		SSDFS_ERR("fail to check SSDFS file system on %s\n",
			  env.base.dev_name);
		goto fsck_finish;

	default:
		SSDFS_ERR("unknown check result on %s\n",
			  env.base.dev_name);
		goto fsck_finish;
	}

	SSDFS_FSCK_INFO(env.base.show_info,
			"[003]\t[SUCCESS]\n");

	switch (res) {
	case SSDFS_FSCK_VOLUME_HEALTHY:
		SSDFS_FSCK_INFO(env.base.show_info,
				"No corruptions have been detected. "
				"Have a nice day.\n");
		goto fsck_finish;

	case SSDFS_FSCK_VOLUME_COMPLETELY_DESTROYED:
	case SSDFS_FSCK_VOLUME_HEAVILY_CORRUPTED:
	case SSDFS_FSCK_VOLUME_SLIGHTLY_CORRUPTED:
	case SSDFS_FSCK_VOLUME_UNCLEAN_UMOUNT:
		if (env.no_change) {
			SSDFS_FSCK_INFO(env.base.show_info,
					"Volume is corrupted. "
					"Please, use FSCK or RECOVERFS tool.\n");
			goto fsck_finish;
		}

		if (env.auto_repair || env.yes_all_questions) {
			SSDFS_FSCK_INFO(env.base.show_info && env.be_verbose,
					"Try to recover SSDFS volume on %s\n",
					env.base.dev_name);
			goto try_recover_volume;
		}

		if (!env.base.show_info)
			goto fsck_finish;

		SSDFS_INFO("Volume on %s is corrupted. "
			   "Would you like to recover the volume? [y|N]: ",
			   env.base.dev_name);

		switch (get_user_answer()) {
		case SSDFS_FSCK_YES_USER_ANSWER:
			goto try_recover_volume;

		case SSDFS_FSCK_NO_USER_ANSWER:
			SSDFS_INFO("Volume is corrupted. "
				   "Please, use FSCK or RECOVERFS tool.\n");
			goto fsck_finish;

		default:
			SSDFS_INFO("Unrecognized answer. Volume is corrupted. "
				   "Please, use FSCK or RECOVERFS tool.\n");
			goto fsck_finish;
		}
		break;

	default:
		BUG();
		break;
	}

try_recover_volume:
	SSDFS_FSCK_INFO(env.base.show_info,
			"[004]\tRECOVER SSDFS VOLUME...\n");

	res = recover_corrupted_ssdfs_volume(&env);
	switch (res) {
	case SSDFS_FSCK_NO_RECOVERY_NECCESSARY:
		SSDFS_FSCK_INFO(env.base.show_info,
				"No need for recovery on %s\n",
				env.base.dev_name);
		break;

	case SSDFS_FSCK_UNABLE_RECOVER:
		SSDFS_FSCK_INFO(env.base.show_info,
				"Unable to recover volume on %s\n",
				env.base.dev_name);
		goto fsck_finish;

	case SSDFS_FSCK_COMPLETE_METADATA_REBUILD:
		SSDFS_FSCK_INFO(env.base.show_info && env.be_verbose,
				"Metadata were completely rebuilt on %s\n",
				env.base.dev_name);
		break;

	case SSDFS_FSCK_METADATA_PARTIALLY_LOST:
		SSDFS_FSCK_INFO(env.base.show_info,
				"Metadata were partially lost on %s\n",
				env.base.dev_name);
		goto fsck_finish;

	case SSDFS_FSCK_USER_DATA_PARTIALLY_LOST:
		SSDFS_FSCK_INFO(env.base.show_info,
				"User data were partially lost on %s\n",
				env.base.dev_name);
		goto fsck_finish;

	case SSDFS_FSCK_RECOVERY_NAND_DEGRADED:
		SSDFS_FSCK_INFO(env.base.show_info && env.be_verbose,
				"Probably, NAND was degraded on %s\n",
				env.base.dev_name);
		break;

	case SSDFS_FSCK_RECOVERY_DEVICE_MALFUNCTION:
		SSDFS_FSCK_INFO(env.base.show_info,
				"Hardware malfunctioning on %s\n",
				env.base.dev_name);
		goto fsck_finish;

	case SSDFS_FSCK_RECOVERY_INTERRUPTED:
		SSDFS_FSCK_INFO(env.base.show_info,
				"Recovery has been interrupted on %s\n",
				env.base.dev_name);
		goto fsck_finish;

	case SSDFS_FSCK_RECOVERY_SUCCESS:
		SSDFS_FSCK_INFO(env.base.show_info && env.be_verbose,
				"SSDFS volume has been recovered on %s\n",
				env.base.dev_name);
		break;

	case SSDFS_FSCK_RECOVERY_FAILED:
		SSDFS_ERR("fail to recover SSDFS file system on %s\n",
			  env.base.dev_name);
		goto fsck_finish;

	default:
		SSDFS_ERR("unknown recovery result on %s\n",
			  env.base.dev_name);
		goto fsck_finish;
	}

	SSDFS_FSCK_INFO(env.base.show_info,
			"[004]\t[SUCCESS]\n");

fsck_finish:
	ssdfs_fsck_destroy_detection_result(&env);
	ssdfs_fsck_destroy_check_result(&env);
	ssdfs_fsck_destroy_recovery_result(&env);
	close(env.base.fd);
	exit(err ? EXIT_FAILURE : EXIT_SUCCESS);
}
