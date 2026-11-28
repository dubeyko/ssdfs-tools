/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * lib/blk_readwrite.c - block layer read/write operations.
 *
 * Copyright (c) 2014-2019 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 * Copyright (c) 2014-2026 Viacheslav Dubeyko <slava@dubeyko.com>
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

#include <linux/fs.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>

#include "ssdfs_tools.h"

/************************************************************************
 *                       Write/Erase operations                         *
 ************************************************************************/

int bdev_read(int fd, u64 offset, size_t size, void *buf, int is_debug)
{
	return ssdfs_pread(fd, offset, size, buf);
}

int bdev_write(int fd, struct ssdfs_nand_geometry *info,
		u64 offset, size_t size, void *buf,
		u32 *open_zones, int is_debug)
{
	return ssdfs_pwrite(fd, offset, size, buf);
}

#define BDEV_ERASE_VERIFY_BLOCK		4096

/*
 * Whether discarded ranges read back as erased is a property of the
 * device. But reading a block back as erased proves something only
 * if the block contained data before the discard: a device that
 * ignores discard leaves an unused block erased too.
 *
 * Every discard is followed by a check of the range's first block.
 * Discard becomes trusted when it has erased a first block that
 * contained data. A failed discard or check stops trusting discard
 * for the rest of the run, and every later erase zero-fills. The
 * state is shared by the erase threads.
 */
enum {
	BDEV_DISCARD_UNVERIFIED,
	BDEV_DISCARD_TRUSTED,
	BDEV_DISCARD_UNTRUSTED,
};

static int bdev_discard_state = BDEV_DISCARD_UNVERIFIED;

/*
 * State of a block before discard. Only a block that contained data
 * and reads as erased after discard proves that discard works.
 */
enum bdev_block_state {
	BDEV_BLOCK_STATE_UNKNOWN,	/* not checked or unreadable */
	BDEV_BLOCK_STATE_ERASED,
	BDEV_BLOCK_STATE_HAS_DATA,
};

static int is_block_uniform(const u8 *buf, size_t size, u8 pattern)
{
	size_t i;

	for (i = 0; i < size; i++) {
		if (buf[i] != pattern)
			return SSDFS_FALSE;
	}

	return SSDFS_TRUE;
}

/*
 * bdev_get_block_state() - check whether block reads as erased
 *
 * The driver accepts only blocks that are all 0x00 or all 0xFF.
 */
static enum bdev_block_state bdev_get_block_state(int fd, u64 offset)
{
	/* the device can be opened with O_DIRECT */
	u8 buf[BDEV_ERASE_VERIFY_BLOCK]
		__attribute__((aligned(BDEV_ERASE_VERIFY_BLOCK)));

	if (ssdfs_pread(fd, offset, sizeof(buf), buf))
		return BDEV_BLOCK_STATE_UNKNOWN;

	if (is_block_uniform(buf, sizeof(buf), 0x00) ||
	    is_block_uniform(buf, sizeof(buf), 0xFF))
		return BDEV_BLOCK_STATE_ERASED;

	return BDEV_BLOCK_STATE_HAS_DATA;
}

/*
 * bdev_stop_trusting_discard() - zero-fill every later erase
 *
 * RETURN:
 * [SSDFS_TRUE]  - discard has been trusted or unverified until now.
 * [SSDFS_FALSE] - discard has already been untrusted.
 */
static int bdev_stop_trusting_discard(void)
{
	return __atomic_exchange_n(&bdev_discard_state,
				   BDEV_DISCARD_UNTRUSTED,
				   __ATOMIC_ACQ_REL) != BDEV_DISCARD_UNTRUSTED;
}

/*
 * bdev_read_sysfs_uint() - read unsigned value from sysfs
 */
static int bdev_read_sysfs_uint(const char *path, unsigned int *value)
{
	FILE *fp;
	int res;

	fp = fopen(path, "r");
	if (!fp)
		return -errno;

	res = fscanf(fp, "%u", value);
	fclose(fp);

	return res == 1 ? 0 : -EINVAL;
}

enum bdev_limits_state {
	BDEV_LIMITS_UNKNOWN,		/* not read yet */
	BDEV_LIMITS_VALID,
	BDEV_LIMITS_UNAVAILABLE,	/* not a block device or no sysfs */
};

/*
 * bdev_get_discard_limits() - get discard granularity and alignment
 *
 * The limits are read from sysfs once and cached: every erase needs
 * them, and the volume is the same during the whole run.
 *
 * RETURN:
 * [0]       - the limits are known.
 * [-ENODEV] - the limits are unknown (for example, for an image file).
 */
static int bdev_get_discard_limits(int fd, unsigned int *granularity,
				   unsigned int *alignment)
{
	static unsigned int cached_granularity;
	static unsigned int cached_alignment;
	static enum bdev_limits_state cached_state = BDEV_LIMITS_UNKNOWN;
	enum bdev_limits_state state;
	char path[PATH_MAX];
	struct stat st;

	state = __atomic_load_n(&cached_state, __ATOMIC_ACQUIRE);
	if (state == BDEV_LIMITS_UNKNOWN) {
		state = BDEV_LIMITS_UNAVAILABLE;

		if (!fstat(fd, &st) && S_ISBLK(st.st_mode)) {
			unsigned int major_id = major(st.st_rdev);
			unsigned int minor_id = minor(st.st_rdev);

			/* a partition keeps the queue in its parent's dir */
			snprintf(path, sizeof(path),
				 "/sys/dev/block/%u:%u/queue/"
				 "discard_granularity",
				 major_id, minor_id);
			if (!bdev_read_sysfs_uint(path, &cached_granularity))
				state = BDEV_LIMITS_VALID;
			else {
				snprintf(path, sizeof(path),
					 "/sys/dev/block/%u:%u/../queue/"
					 "discard_granularity",
					 major_id, minor_id);
				if (!bdev_read_sysfs_uint(path,
							&cached_granularity))
					state = BDEV_LIMITS_VALID;
			}

			snprintf(path, sizeof(path),
				 "/sys/dev/block/%u:%u/discard_alignment",
				 major_id, minor_id);
			if (bdev_read_sysfs_uint(path, &cached_alignment))
				cached_alignment = 0;
		}

		/* concurrent callers store the same values */
		__atomic_store_n(&cached_state, state, __ATOMIC_RELEASE);
	}

	if (state != BDEV_LIMITS_VALID)
		return -ENODEV;

	*granularity = cached_granularity;
	*alignment = cached_alignment;
	return 0;
}

/*
 * bdev_discard_fits_range() - can discard erase the whole range?
 *
 * A device erases only whole discard granules, and it can silently
 * ignore the rest of a discarded range. If the range isn't made of
 * whole granules, then a part of it can stay unerased. Checking the
 * range's first block doesn't catch that, so don't use discard then.
 * If the limits can't be read (for example, for an image file),
 * then BLKDISCARD decides.
 */
static int bdev_discard_fits_range(int fd, u64 offset, size_t size,
				   int is_debug)
{
	unsigned int granularity, alignment;

	if (bdev_get_discard_limits(fd, &granularity, &alignment) ||
	    granularity == 0)
		return SSDFS_TRUE;

	if (offset % granularity == 0 && size % granularity == 0 &&
	    alignment % granularity == 0)
		return SSDFS_TRUE;

	if (bdev_stop_trusting_discard()) {
		SSDFS_DBG(is_debug,
			  "discard granularity %u (alignment %u) doesn't fit "
			  "range (offset %llu, size %zu): zero-fill\n",
			  granularity, alignment, offset, size);
	}

	return SSDFS_FALSE;
}

/*
 * bdev_discard_is_reliable() - decide whether a discard was enough
 * @prev_state: state of the range's first block before discard
 */
static int bdev_discard_is_reliable(int fd, u64 offset,
				    enum bdev_block_state prev_state,
				    int is_debug)
{
	int expected = BDEV_DISCARD_UNVERIFIED;

	switch (bdev_get_block_state(fd, offset)) {
	case BDEV_BLOCK_STATE_ERASED:
		if (prev_state == BDEV_BLOCK_STATE_HAS_DATA) {
			__atomic_compare_exchange_n(&bdev_discard_state,
						    &expected,
						    BDEV_DISCARD_TRUSTED,
						    SSDFS_FALSE,
						    __ATOMIC_RELEASE,
						    __ATOMIC_RELAXED);
		}
		return SSDFS_TRUE;

	case BDEV_BLOCK_STATE_HAS_DATA:
		if (bdev_stop_trusting_discard()) {
			SSDFS_DBG(is_debug,
				  "discarded block doesn't read as erased: "
				  "offset %llu: zero-fill\n", offset);
		}
		return SSDFS_FALSE;

	default:
		/* can't check: zero-fill this range only */
		return SSDFS_FALSE;
	}
}

int bdev_erase(int fd, u64 offset, size_t size,
		void *buf, size_t buf_size, int is_debug)
{
	u64 range[2] = {offset, size};
	u64 erased_bytes = 0;
	int err;

	/*
	 * Prefer a plain discard (TRIM): it is fast and doesn't wear
	 * the flash. Verify that the range reads as erased and fall
	 * back to zero-filling when it doesn't (or discard fails).
	 */
	if (__atomic_load_n(&bdev_discard_state, __ATOMIC_ACQUIRE) !=
						BDEV_DISCARD_UNTRUSTED &&
	    bdev_discard_fits_range(fd, offset, size, is_debug)) {
		enum bdev_block_state prev_state = BDEV_BLOCK_STATE_UNKNOWN;

		if (__atomic_load_n(&bdev_discard_state, __ATOMIC_ACQUIRE) ==
						BDEV_DISCARD_UNVERIFIED)
			prev_state = bdev_get_block_state(fd, offset);

		if (ioctl(fd, BLKDISCARD, &range) < 0) {
			if (bdev_stop_trusting_discard()) {
				SSDFS_DBG(is_debug,
					  "BLKDISCARD failed (err %d): "
					  "zero-fill\n", errno);
			}
		} else if (bdev_discard_is_reliable(fd, offset, prev_state,
						    is_debug))
			return 0;
	}

	SSDFS_DBG(is_debug,
		  "discard isn't enough, zero-fill: "
		  "offset %llu, size %zu\n",
		  offset, size);

	if (ioctl(fd, BLKZEROOUT, &range) < 0) {
		SSDFS_DBG(is_debug,
			  "BLKZEROOUT is not supported: "
			  "trying write: offset %llu, size %zu\n",
			   offset, size);

		do {
			err = ssdfs_pwrite(fd,
					   offset + erased_bytes,
					   buf_size, buf);
			if (err) {
				SSDFS_ERR("fail to erase: "
					  "offset %llu, "
					  "erased_bytes %llu, "
					  "size %zu, "
					  "buf_size %zu, "
					  "err %d\n",
					  offset, erased_bytes,
					  size, buf_size, err);
				return err;
			}

			erased_bytes += buf_size;
		} while (erased_bytes < size);
	}

	return 0;
}

int bdev_check_nand_geometry(int fd, struct ssdfs_nand_geometry *info,
			     int is_debug)
{
	return -EOPNOTSUPP;
}

int bdev_check_peb(int fd, u64 offset, u32 erasesize,
		   int need_close_zone, int is_debug)
{
	return -EOPNOTSUPP;
}
