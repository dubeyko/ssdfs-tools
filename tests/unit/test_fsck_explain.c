/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * tests/unit/test_fsck_explain.c - unit-tests of volume corruption
 *                                  explanation (fsck.ssdfs -R option).
 *
 * Copyright (c) 2026 Viacheslav Dubeyko <slava@dubeyko.com>
 *              http://www.ssdfs.org/
 * All rights reserved.
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

/*
 * Static functions are tested by means of including the source
 * files. The check_file_system.c, check_*.c and detect_file_system.c
 * are included because explain_corruption.c depends on them.
 */
#include "explain_corruption.c"
#include "check_file_system.c"
#include "check_segment_header.c"
#include "check_partial_log_header.c"
#include "check_log_footer.c"
#include "detect_file_system.c"

#include "test_common.h"

#define OUTPUT_MAX	(16 * 1024)

static struct ssdfs_fsck_environment env;
static struct ssdfs_fsck_volume_creation_point creation_point;
static char output[OUTPUT_MAX];

/*
 * prepare_items() - prepare items of on-disk metadata
 * @metadata: on-disk metadata items [SSDFS_FSCK_METADATA_ITEMS_MAX]
 * @metadata_id: on-disk metadata ID
 * @capacity: number of items
 *
 * All items are OK.
 */
static
void prepare_items(struct ssdfs_fsck_metadata_items_array *metadata,
		   int metadata_id, int capacity)
{
	struct ssdfs_fsck_metadata_items_array *items = &metadata[metadata_id];
	int i;

	if (ssdfs_fsck_init_metadata_items(items, capacity)) {
		fprintf(stderr, "fail to allocate items array\n");
		exit(2);
	}

	for (i = 0; i < capacity; i++) {
		items->states[i].metadata_id = metadata_id;
		items->states[i].item_id = i;
		items->states[i].state = SSDFS_FSCK_METADATA_ITEM_OK;
	}

	items->count = capacity;
	items->checked = capacity;
}

/*
 * prepare_volume() - prepare check result of volume
 *
 * All critical metadata have been found. All items of the base
 * snapshot segment's log are OK. Other metadata structures
 * haven't been checked.
 */
static
void prepare_volume(void)
{
	struct ssdfs_fsck_base_snapshot_segment_corruption *ptr;

	ssdfs_fsck_destroy_check_result(&env);
	memset(&env, 0, sizeof(env));
	memset(&creation_point, 0, sizeof(creation_point));

	env.base.show_info = SSDFS_TRUE;
	env.base.dev_name = "/dev/test";
	env.report_level = SSDFS_FSCK_REPORT_DEFAULT_LEVEL;

	creation_point.found_metadata =
			SSDFS_FSCK_ALL_CRITICAL_METADATA_FOUND_MASK |
			SSDFS_FSCK_METADATA_PEB_MAP_PREPARED;
	creation_point.searched_metadata =
			SSDFS_FSCK_ALL_CRITICAL_METADATA_FOUND_MASK;
	env.check_result.corruption.creation_point = &creation_point;

	ptr = &env.check_result.corruption.base_snapshot_seg;
	ptr->state = SSDFS_FSCK_METADATA_STRUCTURE_CORRUPTED;

	prepare_items(ptr->metadata, SSDFS_FSCK_LOG_HDR_METADATA_ID,
		      SSDFS_FSCK_HDR_ITEMS_MAX);
	prepare_items(ptr->metadata, SSDFS_FSCK_LOG_FOOTER_METADATA_ID,
		      SSDFS_FSCK_FOOTER_ITEMS_MAX);
}

static
void set_item(int metadata_id, int index, int state)
{
	struct ssdfs_fsck_metadata_items_array *items;

	items = &env.check_result.corruption.base_snapshot_seg.metadata[metadata_id];
	items->states[index].state = state;

	if (state == SSDFS_FSCK_METADATA_ITEM_CORRUPTED)
		items->corrupted++;
}

static
void set_header_item(int index, int state)
{
	set_item(SSDFS_FSCK_LOG_HDR_METADATA_ID, index, state);
}

static
void set_footer_item(int index, int state)
{
	set_item(SSDFS_FSCK_LOG_FOOTER_METADATA_ID, index, state);
}

/*
 * explain() - capture the output of corruption explanation
 * @report_level: detail level of corruption report
 */
static
const char *explain(int report_level)
{
	FILE *tmp;
	size_t len;
	int saved_fd;

	env.report_level = report_level;

	fflush(stdout);
	saved_fd = dup(STDOUT_FILENO);
	tmp = tmpfile();
	if (saved_fd < 0 || !tmp) {
		fprintf(stderr, "fail to redirect stdout\n");
		exit(2);
	}

	dup2(fileno(tmp), STDOUT_FILENO);
	ssdfs_fsck_explain_volume_corruption(&env);
	fflush(stdout);
	dup2(saved_fd, STDOUT_FILENO);
	close(saved_fd);

	rewind(tmp);
	len = fread(output, 1, OUTPUT_MAX - 1, tmp);
	output[len] = '\0';
	fclose(tmp);

	return output;
}

static
int contains(const char *str)
{
	return strstr(output, str) != NULL;
}

static
void test_footer_item_corruption(void)
{
	prepare_volume();
	set_footer_item(SSDFS_FSCK_FOOTER_UUID_ID,
			SSDFS_FSCK_METADATA_ITEM_CORRUPTED);

	explain(SSDFS_FSCK_REPORT_SUMMARY);
	SSDFS_TEST_CHECK("summary: header line",
			 contains("Corruption details on /dev/test:\n"));
	SSDFS_TEST_CHECK("summary: structure state",
			 contains("  base snapshot segment: CORRUPTED\n"));
	SSDFS_TEST_CHECK("summary: no parts",
			 !contains("log footer"));
	SSDFS_TEST_CHECK("summary: no item",
			 !contains("volume UUID"));

	explain(SSDFS_FSCK_REPORT_PARTS);
	SSDFS_TEST_CHECK("parts: structure state without location",
			 contains("  base snapshot segment: CORRUPTED\n") &&
			 !contains("PEB"));
	SSDFS_TEST_CHECK("parts: corrupted part",
			 contains("    log footer: 1 of 27 items corrupted\n"));
	SSDFS_TEST_CHECK("parts: healthy part is not shown",
			 !contains("log header:"));
	SSDFS_TEST_CHECK("parts: no item",
			 !contains("volume UUID"));

	explain(SSDFS_FSCK_REPORT_FULL);
	SSDFS_TEST_CHECK("full: corrupted part",
			 contains("    log footer: 1 of 27 items corrupted\n"));
	SSDFS_TEST_CHECK("full: corrupted item",
			 contains("    log footer: 1 of 27 items corrupted\n"
				  "      volume UUID: corrupted\n"));
}

static
void test_header_and_failed_check(void)
{
	prepare_volume();
	set_header_item(SSDFS_FSCK_HDR_FLAGS_ID,
			SSDFS_FSCK_METADATA_ITEM_CORRUPTED);
	set_header_item(SSDFS_FSCK_HDR_LOG_PAGES_ID,
			SSDFS_FSCK_METADATA_ITEM_CHECK_FAILED);
	/* absent items are not reported */
	set_header_item(SSDFS_FSCK_HDR_SB_PEBS_ID,
			SSDFS_FSCK_METADATA_ITEM_ABSENT);

	explain(SSDFS_FSCK_REPORT_PARTS);
	SSDFS_TEST_CHECK("header: part name and failed checks",
			 contains("    log header: 1 of 41 items "
				  "corrupted, 1 checks failed\n"));
	SSDFS_TEST_CHECK("header: healthy footer is not shown",
			 !contains("log footer"));

	explain(SSDFS_FSCK_REPORT_FULL);
	SSDFS_TEST_CHECK("header: corrupted item",
			 contains("      volume flags: corrupted\n"));
	SSDFS_TEST_CHECK("header: failed check",
			 contains("      log pages: check failed\n"));
	SSDFS_TEST_CHECK("header: absent item is not shown",
			 !contains("superblock PEBs"));
}

static
void test_missing_metadata(void)
{
	prepare_volume();
	creation_point.found_metadata &= ~(SSDFS_FSCK_SB_SEGS_FOUND |
					   SSDFS_FSCK_MAPPING_TBL_FOUND);
	env.check_result.corruption.base_snapshot_seg.state =
				SSDFS_FSCK_METADATA_STRUCTURE_ABSENT;
	/* no log has been checked */
	ssdfs_fsck_destroy_metadata_items_array(
			env.check_result.corruption.base_snapshot_seg.metadata);

	explain(SSDFS_FSCK_REPORT_SUMMARY);
	SSDFS_TEST_CHECK("missing: not found list",
			 contains("  not found: superblock segments, "
				  "mapping table\n"));
	SSDFS_TEST_CHECK("missing: base snapshot isn't found",
			 contains("  base snapshot segment: NOT FOUND\n"));
	SSDFS_TEST_CHECK("missing: base snapshot is shown once",
			 !contains("not found: base snapshot"));

	SSDFS_TEST_CHECK("missing: nothing is reported as not searched",
			 !contains("not searched"));

	explain(SSDFS_FSCK_REPORT_FULL);
	SSDFS_TEST_CHECK("missing: no parts without log",
			 contains("  base snapshot segment: NOT FOUND\n") &&
			 !contains("log header") && !contains("log footer"));
}

static
void test_not_searched_metadata(void)
{
	prepare_volume();
	/*
	 * Superblock segments haven't been found, so segment bitmap,
	 * mapping table and mapping table cache cannot be located.
	 */
	creation_point.found_metadata = SSDFS_FSCK_BASE_SNAPSHOT_SEG_FOUND;
	creation_point.searched_metadata =
			SSDFS_FSCK_BASE_SNAPSHOT_SEG_FOUND |
			SSDFS_FSCK_SB_SEGS_FOUND;

	explain(SSDFS_FSCK_REPORT_SUMMARY);
	SSDFS_TEST_CHECK("not searched: not found list",
			 contains("  not found: superblock segments\n"));
	SSDFS_TEST_CHECK("not searched: not searched list",
			 contains("  not searched: mapping table, "
				  "segment bitmap, mapping table cache "
				  "(location is unknown)\n"));

	/*
	 * Base snapshot segment is not found but the rest is found:
	 * the state line shows it, so the not found list is empty.
	 */
	prepare_volume();
	creation_point.found_metadata &= ~SSDFS_FSCK_BASE_SNAPSHOT_SEG_FOUND;
	env.check_result.corruption.base_snapshot_seg.state =
				SSDFS_FSCK_METADATA_STRUCTURE_ABSENT;

	explain(SSDFS_FSCK_REPORT_SUMMARY);
	SSDFS_TEST_CHECK("not searched: only base snapshot isn't found",
			 contains("  base snapshot segment: NOT FOUND\n") &&
			 !contains("not found:") && !contains("not searched"));
}

static
void test_not_found_with_state_line(void)
{
	prepare_volume();
	/* superblock segment has been checked and it is absent */
	creation_point.found_metadata &= ~(SSDFS_FSCK_SB_SEGS_FOUND |
					   SSDFS_FSCK_MAPPING_TBL_FOUND);
	env.check_result.corruption.superblock_seg.state =
				SSDFS_FSCK_METADATA_STRUCTURE_ABSENT;

	explain(SSDFS_FSCK_REPORT_SUMMARY);
	SSDFS_TEST_CHECK("state line: structure is shown once",
			 contains("  superblock segment: NOT FOUND\n") &&
			 contains("  not found: mapping table\n") &&
			 !contains("superblock segments"));
}

static
void test_not_checked_structures(void)
{
	prepare_volume();

	explain(SSDFS_FSCK_REPORT_SUMMARY);
	SSDFS_TEST_CHECK("not checked: all stubs",
			 contains("  not checked: superblock segment, "
				  "segment bitmap, mapping table, "
				  "inodes b-tree, snapshots b-tree, "
				  "invalidated extents b-tree, "
				  "shared dictionary b-tree\n"));

	env.check_result.corruption.superblock_seg.state =
				SSDFS_FSCK_METADATA_STRUCTURE_OK;
	env.check_result.corruption.snapshots_btree.state =
				SSDFS_FSCK_METADATA_STRUCTURE_CORRUPTED;

	explain(SSDFS_FSCK_REPORT_SUMMARY);
	SSDFS_TEST_CHECK("not checked: checked structure state",
			 contains("  superblock segment: OK\n") &&
			 contains("  snapshots b-tree: CORRUPTED\n"));
	SSDFS_TEST_CHECK("not checked: checked structures are excluded",
			 contains("  not checked: segment bitmap, "
				  "mapping table, inodes b-tree, "
				  "invalidated extents b-tree, "
				  "shared dictionary b-tree\n"));
}

static
void test_quiet_and_no_metadata(void)
{
	prepare_volume();
	env.base.show_info = SSDFS_FALSE;

	explain(SSDFS_FSCK_REPORT_FULL);
	SSDFS_TEST_CHECK("quiet: no output", output[0] == '\0');

	prepare_volume();
	env.check_result.corruption.creation_point = NULL;

	explain(SSDFS_FSCK_REPORT_FULL);
	SSDFS_TEST_CHECK("no metadata: message",
			 contains("  no SSDFS metadata have been found\n"));
	SSDFS_TEST_CHECK("no metadata: nothing else",
			 !contains("base snapshot segment"));
}

static
void test_item_names(void)
{
	int i;
	int unnamed = 0;

	for (i = 0; i < SSDFS_FSCK_HDR_ITEMS_MAX; i++) {
		if (!ssdfs_fsck_hdr_item_names[i]) {
			printf("unnamed header item #%d\n", i);
			unnamed++;
		}
	}

	for (i = 0; i < SSDFS_FSCK_FOOTER_ITEMS_MAX; i++) {
		if (!ssdfs_fsck_footer_item_names[i]) {
			printf("unnamed footer item #%d\n", i);
			unnamed++;
		}
	}

	for (i = 0; i < SSDFS_FSCK_METADATA_ITEMS_MAX; i++) {
		if (!ssdfs_fsck_metadata_names[i] ||
		    !ssdfs_fsck_metadata_item_names[i] ||
		    ssdfs_fsck_metadata_items_count[i] <= 0) {
			printf("undescribed on-disk metadata #%d\n", i);
			unnamed++;
		}
	}

	SSDFS_TEST_CHECK("names: every metadata and its items have names",
			 unnamed == 0);
}

int main(void)
{
	test_footer_item_corruption();
	test_header_and_failed_check();
	test_missing_metadata();
	test_not_searched_metadata();
	test_not_found_with_state_line();
	test_not_checked_structures();
	test_quiet_and_no_metadata();
	test_item_names();

	ssdfs_fsck_destroy_check_result(&env);

	return ssdfs_test_summary("test_fsck_explain");
}
