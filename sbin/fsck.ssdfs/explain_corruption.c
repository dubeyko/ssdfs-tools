/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/fsck.ssdfs/explain_corruption.c - explanation of volume corruption.
 *
 * Copyright (c) 2026 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#include "fsck.h"
#include "check_file_system.h"

/* State of metadata structure */
static const char *
ssdfs_fsck_structure_state_names[SSDFS_FSCK_METADATA_STRUCTURE_STATE_MAX] = {
	[SSDFS_FSCK_METADATA_STRUCTURE_UNKNOWN_STATE]	= "NOT CHECKED",
	[SSDFS_FSCK_METADATA_STRUCTURE_OK]		= "OK",
	[SSDFS_FSCK_METADATA_STRUCTURE_CORRUPTED]	= "CORRUPTED",
	[SSDFS_FSCK_METADATA_STRUCTURE_ABSENT]		= "NOT FOUND",
	[SSDFS_FSCK_METADATA_STRUCTURE_CHECK_FAILED]	= "CHECK FAILED",
};

/*
 * Verdict on metadata item.
 * OK or absent items are not reported (NULL).
 */
static const char *
ssdfs_fsck_item_verdicts[SSDFS_FSCK_METADATA_ITEM_STATE_MAX] = {
	[SSDFS_FSCK_METADATA_ITEM_CORRUPTED]		= "corrupted",
	[SSDFS_FSCK_METADATA_ITEM_CHECK_FAILED]		= "check failed",
};

/*
 * struct ssdfs_fsck_critical_metadata_name - name of critical metadata
 * @flag: flag of found metadata (detection phase)
 * @name: name of metadata structure
 */
struct ssdfs_fsck_critical_metadata_name {
	u32 flag;
	const char *name;
};

/* Critical metadata that detection phase is looking for */
static const struct ssdfs_fsck_critical_metadata_name
ssdfs_fsck_critical_metadata_names[] = {
	{ SSDFS_FSCK_BASE_SNAPSHOT_SEG_FOUND,	"base snapshot segment" },
	{ SSDFS_FSCK_SB_SEGS_FOUND,		"superblock segments" },
	{ SSDFS_FSCK_MAPPING_TBL_FOUND,		"mapping table" },
	{ SSDFS_FSCK_SEGMENT_BITMAP_FOUND,	"segment bitmap" },
	{ SSDFS_FSCK_MAPTBL_CACHE_FOUND,	"mapping table cache" },
};

/* Metadata structures that are checked besides base snapshot segment */
enum {
	SSDFS_FSCK_EXPLAIN_SUPERBLOCK_SEG,
	SSDFS_FSCK_EXPLAIN_SEGMENT_BITMAP,
	SSDFS_FSCK_EXPLAIN_MAPPING_TABLE,
	SSDFS_FSCK_EXPLAIN_INODES_BTREE,
	SSDFS_FSCK_EXPLAIN_SNAPSHOTS_BTREE,
	SSDFS_FSCK_EXPLAIN_INVALIDATED_EXTENTS_BTREE,
	SSDFS_FSCK_EXPLAIN_SHARED_DICTIONARY_BTREE,
	SSDFS_FSCK_EXPLAIN_STRUCTURES_MAX
};

static const char *
ssdfs_fsck_structure_names[SSDFS_FSCK_EXPLAIN_STRUCTURES_MAX] = {
	[SSDFS_FSCK_EXPLAIN_SUPERBLOCK_SEG]		= "superblock segment",
	[SSDFS_FSCK_EXPLAIN_SEGMENT_BITMAP]		= "segment bitmap",
	[SSDFS_FSCK_EXPLAIN_MAPPING_TABLE]		= "mapping table",
	[SSDFS_FSCK_EXPLAIN_INODES_BTREE]		= "inodes b-tree",
	[SSDFS_FSCK_EXPLAIN_SNAPSHOTS_BTREE]		= "snapshots b-tree",
	[SSDFS_FSCK_EXPLAIN_INVALIDATED_EXTENTS_BTREE]	= "invalidated extents b-tree",
	[SSDFS_FSCK_EXPLAIN_SHARED_DICTIONARY_BTREE]	= "shared dictionary b-tree",
};

/* Items of segment header or partial log header */
static const char *ssdfs_fsck_hdr_item_names[SSDFS_FSCK_HDR_ITEMS_MAX] = {
	[SSDFS_FSCK_HDR_SIGNATURE_ID]		= "signature",
	[SSDFS_FSCK_HDR_VERSION_ID]		= "on-disk layout version",
	[SSDFS_FSCK_HDR_CHECKSUM_ID]		= "checksum",
	[SSDFS_FSCK_HDR_PAGESIZE_ID]		= "page size",
	[SSDFS_FSCK_HDR_ERASESIZE_ID]		= "erase size",
	[SSDFS_FSCK_HDR_SEGSIZE_ID]		= "segment size",
	[SSDFS_FSCK_HDR_MEGS_PER_PEB_ID]	= "megabytes per PEB",
	[SSDFS_FSCK_HDR_PEBS_PER_SEG_ID]	= "PEBs per segment",
	[SSDFS_FSCK_HDR_CREATE_TIME_ID]		= "volume creation time",
	[SSDFS_FSCK_HDR_CREATE_CNO_ID]		= "volume creation checkpoint",
	[SSDFS_FSCK_HDR_FLAGS_ID]		= "volume flags",
	[SSDFS_FSCK_HDR_LEBS_PER_PEB_INDEX_ID]	= "LEBs per PEB index",
	[SSDFS_FSCK_HDR_SB_PEBS_ID]		= "superblock PEBs",
	[SSDFS_FSCK_HDR_SEGBMAP_ID]		= "segment bitmap descriptor",
	[SSDFS_FSCK_HDR_MAPTBL_ID]		= "mapping table descriptor",
	[SSDFS_FSCK_HDR_LOG_PAGES_ID]		= "log pages",
	[SSDFS_FSCK_HDR_CREATE_THREADS_ID]	= "create threads per segment",
	[SSDFS_FSCK_HDR_DENTRIES_BTREE_ID]	= "dentries b-tree descriptor",
	[SSDFS_FSCK_HDR_EXTENTS_BTREE_ID]	= "extents b-tree descriptor",
	[SSDFS_FSCK_HDR_XATTR_BTREE_ID]		= "xattrs b-tree descriptor",
	[SSDFS_FSCK_HDR_INVEXTREE_ID]		= "invalidated extents b-tree descriptor",
	[SSDFS_FSCK_HDR_UUID_ID]		= "volume UUID",
	[SSDFS_FSCK_HDR_LOG_TIMESTAMP_ID]	= "log timestamp",
	[SSDFS_FSCK_HDR_LOG_CNO_ID]		= "log checkpoint",
	[SSDFS_FSCK_HDR_SEG_TYPE_ID]		= "segment type",
	[SSDFS_FSCK_HDR_SEG_FLAGS_ID]		= "segment flags",
	[SSDFS_FSCK_HDR_DESC_ARRAY_ID]		= "metadata descriptors",
	[SSDFS_FSCK_HDR_PEB_MIGRATION_ID]	= "PEB migration state",
	[SSDFS_FSCK_HDR_PEB_CREATE_TIME_ID]	= "PEB creation time",
	[SSDFS_FSCK_HDR_RELATION_PEB_ID]	= "relation PEB",
	[SSDFS_FSCK_HDR_PL_FLAGS_ID]		= "partial log flags",
	[SSDFS_FSCK_HDR_LOG_BYTES_ID]		= "log size in bytes",
	[SSDFS_FSCK_HDR_NSEGS_ID]		= "segments count",
	[SSDFS_FSCK_HDR_FREE_PAGES_ID]		= "free pages",
	[SSDFS_FSCK_HDR_ROOT_FOLDER_ID]		= "root folder inode",
	[SSDFS_FSCK_HDR_INODES_BTREE_ID]	= "inodes b-tree descriptor",
	[SSDFS_FSCK_HDR_SHEXTREE_ID]		= "shared extents b-tree descriptor",
	[SSDFS_FSCK_HDR_SHDICT_BTREE_ID]	= "shared dictionary b-tree descriptor",
	[SSDFS_FSCK_HDR_SEQUENCE_ID]		= "partial log sequence",
	[SSDFS_FSCK_HDR_SNAP_BTREE_ID]		= "snapshots b-tree descriptor",
	[SSDFS_FSCK_HDR_OPEN_ZONES_ID]		= "open zones",
};

/* Items of log footer */
static const char *ssdfs_fsck_footer_item_names[SSDFS_FSCK_FOOTER_ITEMS_MAX] = {
	[SSDFS_FSCK_FOOTER_SIGNATURE_ID]	= "signature",
	[SSDFS_FSCK_FOOTER_VERSION_ID]		= "on-disk layout version",
	[SSDFS_FSCK_FOOTER_CHECKSUM_ID]		= "checksum",
	[SSDFS_FSCK_FOOTER_NSEGS_ID]		= "segments count",
	[SSDFS_FSCK_FOOTER_FREE_PAGES_ID]	= "free pages",
	[SSDFS_FSCK_FOOTER_LOG_TIMESTAMP_ID]	= "log timestamp",
	[SSDFS_FSCK_FOOTER_LOG_CNO_ID]		= "log checkpoint",
	[SSDFS_FSCK_FOOTER_FLAGS_ID]		= "volume state flags",
	[SSDFS_FSCK_FOOTER_STATE_ID]		= "file system state",
	[SSDFS_FSCK_FOOTER_ERRORS_ID]		= "errors behavior",
	[SSDFS_FSCK_FOOTER_FEATURES_ID]		= "feature flags",
	[SSDFS_FSCK_FOOTER_UUID_ID]		= "volume UUID",
	[SSDFS_FSCK_FOOTER_CUR_SEGS_ID]		= "current segments",
	[SSDFS_FSCK_FOOTER_MIGRATION_THRESHOLD_ID] = "migration threshold",
	[SSDFS_FSCK_FOOTER_BLKBMAP_OPTIONS_ID]	= "block bitmap options",
	[SSDFS_FSCK_FOOTER_BLK2OFF_OPTIONS_ID]	= "offset translation table options",
	[SSDFS_FSCK_FOOTER_USER_DATA_OPTIONS_ID] = "user data options",
	[SSDFS_FSCK_FOOTER_OPEN_ZONES_ID]	= "open zones",
	[SSDFS_FSCK_FOOTER_ROOT_FOLDER_ID]	= "root folder inode",
	[SSDFS_FSCK_FOOTER_INODES_BTREE_ID]	= "inodes b-tree descriptor",
	[SSDFS_FSCK_FOOTER_SHEXTREE_ID]		= "shared extents b-tree descriptor",
	[SSDFS_FSCK_FOOTER_SHDICT_BTREE_ID]	= "shared dictionary b-tree descriptor",
	[SSDFS_FSCK_FOOTER_SNAP_BTREE_ID]	= "snapshots b-tree descriptor",
	[SSDFS_FSCK_FOOTER_LOG_BYTES_ID]	= "log size in bytes",
	[SSDFS_FSCK_FOOTER_LOG_FLAGSS_ID]	= "log flags",
	[SSDFS_FSCK_FOOTER_DESC_ARRAY_ID]	= "metadata descriptors",
	[SSDFS_FSCK_FOOTER_PEB_CREATE_TIME_ID]	= "PEB creation time",
};

static
const char *ssdfs_fsck_structure_state_str(int state)
{
	if (state < 0 || state >= SSDFS_FSCK_METADATA_STRUCTURE_STATE_MAX)
		state = SSDFS_FSCK_METADATA_STRUCTURE_UNKNOWN_STATE;

	return ssdfs_fsck_structure_state_names[state];
}

static
const char *ssdfs_fsck_item_verdict(int state)
{
	if (state < 0 || state >= SSDFS_FSCK_METADATA_ITEM_STATE_MAX)
		return NULL;

	return ssdfs_fsck_item_verdicts[state];
}

/* Names of on-disk metadata */
static const char *ssdfs_fsck_metadata_names[SSDFS_FSCK_METADATA_ITEMS_MAX] = {
	[SSDFS_FSCK_LOG_HDR_METADATA_ID]		= "log header",
	[SSDFS_FSCK_LOG_FOOTER_METADATA_ID]		= "log footer",
};

/* Names of items of on-disk metadata */
static const char **ssdfs_fsck_metadata_item_names[SSDFS_FSCK_METADATA_ITEMS_MAX] = {
	[SSDFS_FSCK_LOG_HDR_METADATA_ID]		= ssdfs_fsck_hdr_item_names,
	[SSDFS_FSCK_LOG_FOOTER_METADATA_ID]		= ssdfs_fsck_footer_item_names,
};

/* Number of items of on-disk metadata */
static const int ssdfs_fsck_metadata_items_count[SSDFS_FSCK_METADATA_ITEMS_MAX] = {
	[SSDFS_FSCK_LOG_HDR_METADATA_ID]		= SSDFS_FSCK_HDR_ITEMS_MAX,
	[SSDFS_FSCK_LOG_FOOTER_METADATA_ID]		= SSDFS_FSCK_FOOTER_ITEMS_MAX,
};

/*
 * ssdfs_fsck_explain_metadata_items() - explain items of on-disk metadata
 * @items: items array of on-disk metadata
 * @place: name of on-disk metadata
 * @names: names of the items
 * @names_count: number of names in @names
 * @report_level: detail level of corruption report
 *
 * SSDFS_FSCK_REPORT_PARTS level shows the number of corrupted
 * items of on-disk metadata. SSDFS_FSCK_REPORT_FULL level shows every
 * corrupted item too. Absent items are not shown because some
 * items are absent by design (for example, partial log header
 * instead of footer).
 */
static
void ssdfs_fsck_explain_metadata_items(struct ssdfs_fsck_metadata_items_array *items,
				       const char *place,
				       const char **names, int names_count,
				       int report_level)
{
	struct ssdfs_fsck_metadata_item_state *item;
	const char *verdict;
	int checked = 0;
	int corrupted = 0;
	int failed = 0;
	int i;

	if (!items->states)
		return;

	for (i = 0; i < items->count; i++) {
		item = &items->states[i];

		switch (item->state) {
		case SSDFS_FSCK_METADATA_ITEM_UNKNOWN_STATE:
			continue;

		case SSDFS_FSCK_METADATA_ITEM_CORRUPTED:
			corrupted++;
			break;

		case SSDFS_FSCK_METADATA_ITEM_CHECK_FAILED:
			failed++;
			break;

		default:
			/* OK or absent item */
			break;
		}

		checked++;
	}

	if (corrupted == 0 && failed == 0)
		return;

	SSDFS_INFO("    %s: %d of %d items corrupted", place,
		   corrupted, checked);
	if (failed > 0)
		SSDFS_INFO(", %d checks failed", failed);
	SSDFS_INFO("\n");

	if (report_level < SSDFS_FSCK_REPORT_FULL)
		return;

	for (i = 0; i < items->count; i++) {
		verdict = ssdfs_fsck_item_verdict(items->states[i].state);
		if (!verdict)
			continue;

		if (i < names_count && names[i])
			SSDFS_INFO("      %s: %s\n", names[i], verdict);
		else
			SSDFS_INFO("      item #%d: %s\n", i, verdict);
	}
}

static
void ssdfs_fsck_explain_base_snapshot(struct ssdfs_fsck_base_snapshot_segment_corruption *ptr,
				      int report_level)
{
	int i;

	SSDFS_INFO("  base snapshot segment: %s\n",
		   ssdfs_fsck_structure_state_str(ptr->state));

	if (report_level < SSDFS_FSCK_REPORT_PARTS)
		return;

	for (i = 0; i < SSDFS_FSCK_METADATA_ITEMS_MAX; i++) {
		ssdfs_fsck_explain_metadata_items(&ptr->metadata[i],
					ssdfs_fsck_metadata_names[i],
					ssdfs_fsck_metadata_item_names[i],
					ssdfs_fsck_metadata_items_count[i],
					report_level);
	}
}

/*
 * has_state_line() - check that metadata structure has own state line
 * @details: corruption details
 * @flag: flag of critical metadata (detection phase)
 *
 * The state of base snapshot segment is always shown. The state
 * of other checked metadata structure is shown if the structure
 * has been checked. Such structure is not shown in the lists of
 * not found and not searched metadata, because the state line
 * already shows that the structure is absent.
 */
static
int has_state_line(struct ssdfs_fsck_corruption_details *details, u32 flag)
{
	int state = SSDFS_FSCK_METADATA_STRUCTURE_UNKNOWN_STATE;

	switch (flag) {
	case SSDFS_FSCK_BASE_SNAPSHOT_SEG_FOUND:
		return SSDFS_TRUE;

	case SSDFS_FSCK_SB_SEGS_FOUND:
		state = details->superblock_seg.state;
		break;

	case SSDFS_FSCK_SEGMENT_BITMAP_FOUND:
		state = details->segment_bitmap.state;
		break;

	case SSDFS_FSCK_MAPPING_TBL_FOUND:
		state = details->mapping_table.state;
		break;

	default:
		/* mapping table cache is checked as part of other structures */
		break;
	}

	return state != SSDFS_FSCK_METADATA_STRUCTURE_UNKNOWN_STATE;
}

/*
 * ssdfs_fsck_explain_volume_corruption() - explain found corruption(s)
 * @env: fsck environment
 *
 * The method shows: (1) critical metadata structures that
 * have been searched but haven't been found by detection phase,
 * and critical metadata structures that haven't been searched
 * because their location is unknown (except the structures
 * that have own state line); (2) the state of
 * every checked metadata structure; (3) metadata structures
 * that haven't been checked. The detail level is defined by
 * env->report_level:
 *   SSDFS_FSCK_REPORT_SUMMARY - state of key metadata structures;
 *   SSDFS_FSCK_REPORT_PARTS - + corrupted parts of metadata
 *                             structures;
 *   SSDFS_FSCK_REPORT_FULL - + every corrupted item.
 */
void ssdfs_fsck_explain_volume_corruption(struct ssdfs_fsck_environment *env)
{
	struct ssdfs_fsck_corruption_details *details;
	struct ssdfs_fsck_volume_creation_point *creation_point;
	const struct ssdfs_fsck_critical_metadata_name *critical;
	int states[SSDFS_FSCK_EXPLAIN_STRUCTURES_MAX];
	int not_found = 0;
	int not_searched = 0;
	int not_checked = 0;
	int i;

	SSDFS_DBG(env->base.show_debug,
		  "explain volume corruption\n");

	if (!env->base.show_info)
		return;

	details = &env->check_result.corruption;
	creation_point = details->creation_point;

	SSDFS_FSCK_INFO(env->base.show_info,
			"Corruption details on %s:\n",
			env->base.dev_name);

	if (!creation_point) {
		SSDFS_INFO("  no SSDFS metadata have been found\n");
		return;
	}

	/* metadata that have been searched but haven't been found */
	for (i = 0; i < ARRAY_SIZE(ssdfs_fsck_critical_metadata_names); i++) {
		critical = &ssdfs_fsck_critical_metadata_names[i];

		if (creation_point->found_metadata & critical->flag)
			continue;

		if (!(creation_point->searched_metadata & critical->flag))
			continue;

		if (has_state_line(details, critical->flag))
			continue;

		SSDFS_INFO("%s%s", not_found ? ", " : "  not found: ",
			   critical->name);
		not_found++;
	}

	if (not_found)
		SSDFS_INFO("\n");

	/*
	 * Metadata that haven't been searched because their location
	 * is unknown (the metadata that define the location haven't
	 * been found).
	 */
	for (i = 0; i < ARRAY_SIZE(ssdfs_fsck_critical_metadata_names); i++) {
		critical = &ssdfs_fsck_critical_metadata_names[i];

		if (creation_point->found_metadata & critical->flag)
			continue;

		if (creation_point->searched_metadata & critical->flag)
			continue;

		if (has_state_line(details, critical->flag))
			continue;

		SSDFS_INFO("%s%s", not_searched ? ", " : "  not searched: ",
			   critical->name);
		not_searched++;
	}

	if (not_searched)
		SSDFS_INFO(" (location is unknown)\n");

	ssdfs_fsck_explain_base_snapshot(&details->base_snapshot_seg,
					 env->report_level);

	states[SSDFS_FSCK_EXPLAIN_SUPERBLOCK_SEG] =
				details->superblock_seg.state;
	states[SSDFS_FSCK_EXPLAIN_SEGMENT_BITMAP] =
				details->segment_bitmap.state;
	states[SSDFS_FSCK_EXPLAIN_MAPPING_TABLE] =
				details->mapping_table.state;
	states[SSDFS_FSCK_EXPLAIN_INODES_BTREE] =
				details->inodes_btree.state;
	states[SSDFS_FSCK_EXPLAIN_SNAPSHOTS_BTREE] =
				details->snapshots_btree.state;
	states[SSDFS_FSCK_EXPLAIN_INVALIDATED_EXTENTS_BTREE] =
				details->invalid_extents.state;
	states[SSDFS_FSCK_EXPLAIN_SHARED_DICTIONARY_BTREE] =
				details->shared_dictionary.state;

	for (i = 0; i < SSDFS_FSCK_EXPLAIN_STRUCTURES_MAX; i++) {
		if (states[i] == SSDFS_FSCK_METADATA_STRUCTURE_UNKNOWN_STATE)
			continue;

		SSDFS_INFO("  %s: %s\n", ssdfs_fsck_structure_names[i],
			   ssdfs_fsck_structure_state_str(states[i]));
	}

	for (i = 0; i < SSDFS_FSCK_EXPLAIN_STRUCTURES_MAX; i++) {
		if (states[i] != SSDFS_FSCK_METADATA_STRUCTURE_UNKNOWN_STATE)
			continue;

		SSDFS_INFO("%s%s", not_checked ? ", " : "  not checked: ",
			   ssdfs_fsck_structure_names[i]);
		not_checked++;
	}

	if (not_checked)
		SSDFS_INFO("\n");
}
