/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * tests/unit/test_fsck_partial_log_header.c - unit-tests of
 *                                            partial log header's checks.
 *
 * Copyright (c) 2026 Viacheslav Dubeyko <slava@dubeyko.com>
 *              http://www.ssdfs.org/
 * All rights reserved.
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

/*
 * Static check functions are tested by means of including
 * the source files. The check_segment_header.c contains
 * the shared check helpers. The check_file_system.c contains
 * the metadata items helpers, and it depends on the rest of
 * included files.
 */
#include "check_segment_header.c"
#include "check_partial_log_header.c"
#include "check_log_footer.c"
#include "check_file_system.c"
#include "explain_corruption.c"
#include "detect_file_system.c"

#include "test_common.h"

static struct ssdfs_fsck_found_log test_log;

static
void prepare_partial_log(u16 seg_type, u32 flags)
{
	memset(&test_log, 0, sizeof(test_log));
	test_log.header.pl_hdr.seg_type = cpu_to_le16(seg_type);
	test_log.header.pl_hdr.flags = cpu_to_le32(flags);
}

static
void check_flags(const char *name, u16 seg_type, u32 flags,
		 int expected_state, int expected_res)
{
	struct ssdfs_thread_state state;
	struct ssdfs_fsck_metadata_item_state item;
	int res;

	memset(&state, 0, sizeof(state));
	memset(&item, 0, sizeof(item));
	prepare_partial_log(seg_type, flags);

	res = ssdfs_fsck_partial_header_flags_check(&state, &item,
						SSDFS_FSCK_LOG_HDR_METADATA_ID,
						&test_log);

	SSDFS_TEST_CHECK(name,
			 res == expected_res &&
			 item.state == expected_state &&
			 item.item_id == SSDFS_FSCK_HDR_FLAGS_ID &&
			 item.metadata_id ==
				SSDFS_FSCK_LOG_HDR_METADATA_ID);
}

static
void test_partial_header_flags_check(void)
{
	int ok = SSDFS_FSCK_METADATA_ITEM_OK;
	int corrupted = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
	int success = SSDFS_FSCK_CHECK_RESULT_SUCCESS;
	int corruption = SSDFS_FSCK_CHECK_RESULT_CORRUPTION;

	check_flags("flags: base snapshot with 4KB page flag",
		    SSDFS_INITIAL_SNAPSHOT_SEG_TYPE,
		    SSDFS_VS_4KB_PAGE_SIZE_PEB,
		    ok, success);
	check_flags("flags: superblock with 4KB page and inline tree flags",
		    SSDFS_SB_SEG_TYPE,
		    SSDFS_VS_4KB_PAGE_SIZE_PEB | SSDFS_HAS_INLINE_INODES_TREE,
		    ok, success);
	/* fsi->fs_flags is initialized by superblock's volume state */
	check_flags("flags: user data with inherited 4KB page flag",
		    SSDFS_USER_DATA_SEG_TYPE,
		    SSDFS_VS_4KB_PAGE_SIZE_PEB,
		    ok, success);
	check_flags("flags: user data without flags",
		    SSDFS_USER_DATA_SEG_TYPE, 0,
		    ok, success);
	check_flags("flags: base snapshot without 4KB page flag",
		    SSDFS_INITIAL_SNAPSHOT_SEG_TYPE, 0,
		    corrupted, corruption);
	check_flags("flags: superblock with inline tree flag only",
		    SSDFS_SB_SEG_TYPE,
		    SSDFS_HAS_INLINE_INODES_TREE,
		    corrupted, corruption);
	check_flags("flags: user data with unknown flag",
		    SSDFS_USER_DATA_SEG_TYPE,
		    0x4 /* the first bit after SSDFS_VOLUME_STATE_FLAGS_MASK */,
		    corrupted, corruption);
	check_flags("flags: base snapshot with 4KB page and bit 31",
		    SSDFS_INITIAL_SNAPSHOT_SEG_TYPE,
		    SSDFS_VS_4KB_PAGE_SIZE_PEB | 0x80000000,
		    corrupted, corruption);
}

static
void check_open_zones(const char *name, u32 open_zones, u64 nsegs,
		      int expected_state, int expected_res)
{
	struct ssdfs_thread_state state;
	struct ssdfs_fsck_metadata_item_state item;
	int res;

	memset(&state, 0, sizeof(state));
	memset(&item, 0, sizeof(item));
	memset(&test_log, 0, sizeof(test_log));
	test_log.header.pl_hdr.open_zones = cpu_to_le32(open_zones);
	test_log.header.pl_hdr.nsegs = cpu_to_le64(nsegs);

	res = ssdfs_fsck_partial_header_open_zones_check(&state, &item,
						SSDFS_FSCK_LOG_HDR_METADATA_ID,
						&test_log);

	SSDFS_TEST_CHECK(name,
			 res == expected_res &&
			 item.state == expected_state &&
			 item.item_id == SSDFS_FSCK_HDR_OPEN_ZONES_ID &&
			 item.metadata_id ==
				SSDFS_FSCK_LOG_HDR_METADATA_ID);
}

static
void test_partial_header_open_zones_check(void)
{
	int ok = SSDFS_FSCK_METADATA_ITEM_OK;
	int corrupted = SSDFS_FSCK_METADATA_ITEM_CORRUPTED;
	int success = SSDFS_FSCK_CHECK_RESULT_SUCCESS;
	int corruption = SSDFS_FSCK_CHECK_RESULT_CORRUPTION;

	check_open_zones("open_zones: conventional device",
			 0, 256, ok, success);
	check_open_zones("open_zones: some open zones",
			 14, 256, ok, success);
	check_open_zones("open_zones: all segments open",
			 256, 256, ok, success);
	check_open_zones("open_zones: more than segments",
			 257, 256, corrupted, corruption);
	/* negative driver's counter stored as le32 */
	check_open_zones("open_zones: U32_MAX",
			 U32_MAX, 256, corrupted, corruption);
}

int main(void)
{
	test_partial_header_flags_check();
	test_partial_header_open_zones_check();

	return ssdfs_test_summary("test_fsck_partial_log_header");
}
