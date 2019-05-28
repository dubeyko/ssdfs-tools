//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * include/ssdfs_constants.h - SSDFS constant values.
 *
 * Copyright (c) 2019 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#ifndef _SSDFS_CONSTANTS_H
#define _SSDFS_CONSTANTS_H

#define SSDFS_TRUE (1)
#define SSDFS_FALSE (0)

enum {
	SSDFS_MTD_DEVICE,
	SSDFS_BLK_DEVICE,
	SSDFS_DEVICE_TYPE_MAX
};

enum {
	SSDFS_PEB_ERASURE_OK = 0,
	SSDFS_PEB_IS_BAD,
	SSDFS_RECOVERING_PEB,
	SSDFS_CHECK_PEB_CODE_MAX
};

#define SSDFS_MTD_MAJOR_DEV	(90)

enum {
	SSDFS_256B	= 256,
	SSDFS_512B	= 512,
	SSDFS_1KB	= 1024,
	SSDFS_2KB	= 2048,
	SSDFS_4KB	= 4096,
	SSDFS_8KB	= 8192,
	SSDFS_16KB	= 16384,
	SSDFS_32KB	= 32768,
	SSDFS_64KB	= 65536,
	SSDFS_128KB	= 131072,
	SSDFS_256KB	= 262144,
	SSDFS_512KB	= 524288,
	SSDFS_2MB	= 2097152,
	SSDFS_8MB	= 8388608,
};

#endif /* _SSDFS_CONSTANTS_H */
