/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/tunefs.c - implementation of tunefs.ssdfs (volume tuning) utility.
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

#define _LARGEFILE64_SOURCE
#define __USE_FILE_OFFSET64
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "ssdfs_tools.h"
#include "tunefs.h"

static
void ssdfs_tunefs_show_current_configuration(struct ssdfs_tunefs_options *options)
{
	struct ssdfs_metadata_options *metadata;

	metadata = &options->old_config.metadata_options;

	SSDFS_TUNEFS_SHOW("CURRENT VOLUME CONFIGURATION:\n");

	SSDFS_TUNEFS_SHOW("UUID: %s\n",
			  uuid_string(options->old_config.fs_uuid));
	SSDFS_TUNEFS_SHOW("LABEL: %s\n", options->old_config.fs_label);

	SSDFS_TUNEFS_SHOW("\n");

	SSDFS_TUNEFS_SHOW("SEGMENT NUMBER: %llu\n", options->old_config.nsegs);
	SSDFS_TUNEFS_SHOW("PAGE (LOGICAL BLOCK) SIZE: %u\n",
			  options->old_config.pagesize);
	SSDFS_TUNEFS_SHOW("ERASE BLOCK SIZE: %u\n",
			  options->old_config.erasesize);
	SSDFS_TUNEFS_SHOW("SEGMENT SIZE: %u\n",
			  options->old_config.segsize);
	SSDFS_TUNEFS_SHOW("ERASE BLOCKS PER SEGMENT: %u\n",
			  options->old_config.pebs_per_seg);
	SSDFS_TUNEFS_SHOW("LOGICAL BLOCKS (PAGES) PER ERASE BLOCK: %u\n",
			  options->old_config.pages_per_peb);
	SSDFS_TUNEFS_SHOW("LOGICAL BLOCKS (PAGES) PER SEGMENT: %u\n",
			  options->old_config.pages_per_seg);
	SSDFS_TUNEFS_SHOW("VOLUME CREATION_TIME: %s\n",
		   ssdfs_nanoseconds_to_time(options->old_config.fs_ctime));
	SSDFS_TUNEFS_SHOW("RAW INODE SIZE: %u\n",
			  options->old_config.raw_inode_size);
	SSDFS_TUNEFS_SHOW("CREATE THREADS PER SEGMENT: %u\n",
			  options->old_config.create_threads_per_seg);
	SSDFS_TUNEFS_SHOW("MIGRATION THRESHOLD: %u\n",
			  options->old_config.migration_threshold);

	SSDFS_TUNEFS_SHOW("\n");

	metadata = &options->old_config.metadata_options;

	if (metadata->blk_bmap.flags & SSDFS_BLK_BMAP_CREATE_COPY) {
		SSDFS_TUNEFS_SHOW("BLOCK BITMAP: HAS_BACKUP_COPY\n");
	}

	switch (metadata->blk_bmap.compression) {
	case SSDFS_BLK_BMAP_NOCOMPR_TYPE:
		SSDFS_TUNEFS_SHOW("BLOCK BITMAP: UNCOMPRESSED BLOB\n");
		break;

	case SSDFS_BLK_BMAP_ZLIB_COMPR_TYPE:
		SSDFS_TUNEFS_SHOW("BLOCK BITMAP: ZLIB COMPRESSION\n");
		break;

	case SSDFS_BLK_BMAP_LZO_COMPR_TYPE:
		SSDFS_TUNEFS_SHOW("BLOCK BITMAP: LZO COMPRESSION\n");
		break;

	default:
		SSDFS_TUNEFS_SHOW("BLOCK BITMAP: UNRECOGNIZED COMPRESSION\n");
		break;
	}

	if (metadata->blk2off_tbl.flags & SSDFS_BLK2OFF_TBL_CREATE_COPY) {
		SSDFS_TUNEFS_SHOW("OFFSET TRANSLATION TABLE: HAS_BACKUP_COPY\n");
	}

	switch (metadata->blk2off_tbl.compression) {
	case SSDFS_BLK2OFF_TBL_NOCOMPR_TYPE:
		SSDFS_TUNEFS_SHOW("OFFSET TRANSLATION TABLE: UNCOMPRESSED BLOB\n");
		break;

	case SSDFS_BLK2OFF_TBL_ZLIB_COMPR_TYPE:
		SSDFS_TUNEFS_SHOW("OFFSET TRANSLATION TABLE: ZLIB COMPRESSION\n");
		break;

	case SSDFS_BLK2OFF_TBL_LZO_COMPR_TYPE:
		SSDFS_TUNEFS_SHOW("OFFSET TRANSLATION TABLE: LZO COMPRESSION\n");
		break;

	default:
		SSDFS_TUNEFS_SHOW("OFFSET TRANSLATION TABLE: UNRECOGNIZED COMPRESSION\n");
		break;
	}

	SSDFS_TUNEFS_SHOW("\n");

	SSDFS_TUNEFS_SHOW("SUPERBLOCK SEGMENT: FULL LOG PAGES: %u\n",
			  options->old_config.sb_seg_log_pages);

	SSDFS_TUNEFS_SHOW("SEGMENT BITMAP: FULL LOG PAGES: %u\n",
			  options->old_config.segbmap_log_pages);

	if (options->old_config.segbmap_flags & SSDFS_SEGBMAP_HAS_COPY) {
		SSDFS_TUNEFS_SHOW("SEGMENT BITMAP: HAS_BACKUP_COPY\n");
	}

	if (options->old_config.segbmap_flags & SSDFS_SEGBMAP_MAKE_ZLIB_COMPR) {
		SSDFS_TUNEFS_SHOW("SEGMENT BITMAP: ZLIB COMPRESSION\n");
	} else if (options->old_config.segbmap_flags &
						SSDFS_SEGBMAP_MAKE_LZO_COMPR) {
		SSDFS_TUNEFS_SHOW("SEGMENT BITMAP: LZO COMPRESSION\n");
	} else {
		SSDFS_TUNEFS_SHOW("SEGMENT BITMAP: UNCOMPRESSED BLOB\n");
	}

	SSDFS_TUNEFS_SHOW("MAPPING TABLE: FULL LOG PAGES: %u\n",
			  options->old_config.maptbl_log_pages);

	if (options->old_config.maptbl_flags & SSDFS_MAPTBL_HAS_COPY) {
		SSDFS_TUNEFS_SHOW("MAPPING TABLE: HAS_BACKUP_COPY\n");
	}

	if (options->old_config.maptbl_flags & SSDFS_MAPTBL_MAKE_ZLIB_COMPR) {
		SSDFS_TUNEFS_SHOW("MAPPING TABLE: ZLIB COMPRESSION\n");
	} else if (options->old_config.maptbl_flags &
						SSDFS_MAPTBL_MAKE_LZO_COMPR) {
		SSDFS_TUNEFS_SHOW("MAPPING TABLE: LZO COMPRESSION\n");
	} else {
		SSDFS_TUNEFS_SHOW("MAPPING TABLE: UNCOMPRESSED BLOB\n");
	}

	SSDFS_TUNEFS_SHOW("BTREE: LEAF NODE: FULL LOG PAGES: %u\n",
			  options->old_config.lnodes_seg_log_pages);
	SSDFS_TUNEFS_SHOW("BTREE: HYBRID NODE: FULL LOG PAGES: %u\n",
			  options->old_config.hnodes_seg_log_pages);
	SSDFS_TUNEFS_SHOW("BTREE: INDEX NODE: FULL LOG PAGES: %u\n",
			  options->old_config.inodes_seg_log_pages);

	SSDFS_TUNEFS_SHOW("\n");

	SSDFS_TUNEFS_SHOW("USER DATA: FULL LOG PAGES: %u\n",
			  options->old_config.user_data_log_pages);

	switch (metadata->user_data.compression) {
	case SSDFS_USER_DATA_NOCOMPR_TYPE:
		SSDFS_TUNEFS_SHOW("USER DATA: UNCOMPRESSED BLOB\n");
		break;

	case SSDFS_USER_DATA_ZLIB_COMPR_TYPE:
		SSDFS_TUNEFS_SHOW("USER DATA: ZLIB COMPRESSION\n");
		break;

	case SSDFS_USER_DATA_LZO_COMPR_TYPE:
		SSDFS_TUNEFS_SHOW("USER DATA: LZO COMPRESSION\n");
		break;

	default:
		SSDFS_TUNEFS_SHOW("USER DATA: UNRECOGNIZED COMPRESSION\n");
		break;
	}

	SSDFS_TUNEFS_SHOW("USER DATA: MIGRATION THRESHOLD: %u\n",
			  metadata->user_data.migration_threshold);

	if (options->old_config.is_zns_device) {
		SSDFS_TUNEFS_SHOW("ZNS DEVICE: ZONE SIZE: %llu\n",
				  options->old_config.zone_size);
		SSDFS_TUNEFS_SHOW("ZNS DEVICE: ZONE CAPACITY: %llu\n",
				  options->old_config.zone_capacity);
		SSDFS_TUNEFS_SHOW("ZNS DEVICE: MAX OPEN ZONES: %u\n",
				  options->old_config.max_open_zones);
		SSDFS_TUNEFS_SHOW("ZNS DEVICE: LOGICAL BLOCKS PER ZONE: %u\n",
				  options->old_config.leb_pages_capacity);
		SSDFS_TUNEFS_SHOW("ZNS DEVICE: "
				  "LOGICAL BLOCKS AVAILABLE FOR WRITE PER ZONE: %u\n",
				  options->old_config.peb_pages_capacity);
	}
}

static inline
void ssdfs_tunefs_show_backup_copy(struct ssdfs_tunefs_option *option,
				   const char *subsystem)
{
	switch (option->state) {
	case SSDFS_ENABLE_OPTION:
		SSDFS_TUNEFS_SHOW("%s: enable backup copy\n",
				  subsystem);
		break;

	case SSDFS_DISABLE_OPTION:
		SSDFS_TUNEFS_SHOW("%s: disable backup copy\n",
				  subsystem);
		break;

	default:
		/* do nothing */
		break;
	}
}

static inline
void ssdfs_tunefs_show_compression(struct ssdfs_tunefs_option *option,
				   const char *subsystem)
{
	switch (option->state) {
	case SSDFS_ENABLE_OPTION:
		switch (option->value) {
		case SSDFS_UNCOMPRESSED_BLOB:
			SSDFS_TUNEFS_SHOW("%s: enable %s compression\n",
					  subsystem,
					  SSDFS_NONE_COMPRESSION_STRING);
			break;

		case SSDFS_ZLIB_BLOB:
			SSDFS_TUNEFS_SHOW("%s: enable %s compression\n",
					  subsystem,
					  SSDFS_ZLIB_COMPRESSION_STRING);
			break;

		case SSDFS_LZO_BLOB:
			SSDFS_TUNEFS_SHOW("%s: enable %s compression\n",
					  subsystem,
					  SSDFS_LZO_COMPRESSION_STRING);
			break;

		default:
			/* do nothing */
			break;
		}
		break;

	default:
		/* do nothing */
		break;
	}
}

static inline
void ssdfs_tunefs_show_log_pages(struct ssdfs_tunefs_option *option,
				 const char *subsystem)
{
	switch (option->state) {
	case SSDFS_ENABLE_OPTION:
		SSDFS_TUNEFS_SHOW("%s: set log pages %d\n",
				  subsystem,
				  option->value);
		break;

	default:
		/* do nothing */
		break;
	}
}

static inline
void ssdfs_tunefs_show_migration_threshold(struct ssdfs_tunefs_option *option,
					   const char *subsystem)
{
	switch (option->state) {
	case SSDFS_ENABLE_OPTION:
		SSDFS_TUNEFS_SHOW("%s: set migration threshold %d\n",
				  subsystem,
				  option->value);
		break;

	default:
		/* do nothing */
		break;
	}
}

static inline
void ssdfs_tunefs_show_reserved_pebs_per_fragment(struct ssdfs_tunefs_option *option,
						  const char *subsystem)
{
	switch (option->state) {
	case SSDFS_ENABLE_OPTION:
		SSDFS_TUNEFS_SHOW("%s: set reserved PEBs per fragment %d\n",
				  subsystem,
				  option->value);
		break;

	default:
		/* do nothing */
		break;
	}
}

static inline
void ssdfs_tunefs_show_min_index_area_size(struct ssdfs_tunefs_option *option,
					   const char *subsystem)
{
	switch (option->state) {
	case SSDFS_ENABLE_OPTION:
		SSDFS_TUNEFS_SHOW("%s: set min index area size %d\n",
				  subsystem,
				  option->value);
		break;

	default:
		/* do nothing */
		break;
	}
}

#define SSDFS_TUNEFS_BLOCK_BITMAP_STRING	"BLOCK BITMAP"
#define SSDFS_TUNEFS_BLK2OFF_TABLE_STRING	"OFFSETS TRANSLATION TABLE"
#define SSDFS_TUNEFS_SEGMENT_BITMAP_STRING	"SEGMENT BITMAP"
#define SSDFS_TUNEFS_MAPPING_TABLE_STRING	"PEB MAPPING TABLE"
#define SSDFS_TUNEFS_BTREE_STRING		"B-TREE"
#define SSDFS_TUNEFS_BTREE_LNODE_STRING		"B-TREE: LEAF NODE"
#define SSDFS_TUNEFS_BTREE_HNODE_STRING		"B-TREE: HYBRID NODE"
#define SSDFS_TUNEFS_BTREE_INODE_STRING		"B-TREE: INDEX NODE"
#define SSDFS_TUNEFS_USER_DATA_STRING		"USER DATA"

static
void ssdfs_tunefs_show_requested_configuration(struct ssdfs_tunefs_options *options)
{
	struct ssdfs_tunefs_config_request *config;
	struct ssdfs_tunefs_volume_label_option *label;
	struct ssdfs_tunefs_blkbmap_options *blkbmap;
	struct ssdfs_tunefs_blk2off_table_options *blk2off_tbl;
	struct ssdfs_tunefs_segbmap_options *segbmap;
	struct ssdfs_tunefs_maptbl_options *maptbl;
	struct ssdfs_tunefs_btree_options *btree;
	struct ssdfs_tunefs_user_data_options *user_data_seg;

	config = &options->new_config;

	SSDFS_TUNEFS_SHOW("REQUESTED VOLUME CONFIGURATION:\n");
	SSDFS_TUNEFS_SHOW("\n");

	/* volume label option */
	label = &config->label;

	if (label->state == SSDFS_ENABLE_OPTION) {
		SSDFS_TUNEFS_SHOW("LABEL: %s\n", label->volume_label);
	}

	/* block bitmap options */
	blkbmap = &config->blkbmap;
	ssdfs_tunefs_show_backup_copy(&blkbmap->has_backup_copy,
				      SSDFS_TUNEFS_BLOCK_BITMAP_STRING);
	ssdfs_tunefs_show_compression(&blkbmap->compression,
				      SSDFS_TUNEFS_BLOCK_BITMAP_STRING);

	/* offsets table options */
	blk2off_tbl = &config->blk2off_tbl;
	ssdfs_tunefs_show_backup_copy(&blk2off_tbl->has_backup_copy,
				      SSDFS_TUNEFS_BLK2OFF_TABLE_STRING);
	ssdfs_tunefs_show_compression(&blk2off_tbl->compression,
				      SSDFS_TUNEFS_BLK2OFF_TABLE_STRING);

	/* segment bitmap options */
	segbmap = &config->segbmap;
	ssdfs_tunefs_show_backup_copy(&segbmap->has_backup_copy,
				      SSDFS_TUNEFS_SEGMENT_BITMAP_STRING);
	ssdfs_tunefs_show_log_pages(&segbmap->log_pages,
				    SSDFS_TUNEFS_SEGMENT_BITMAP_STRING);
	ssdfs_tunefs_show_migration_threshold(&segbmap->migration_threshold,
					      SSDFS_TUNEFS_SEGMENT_BITMAP_STRING);
	ssdfs_tunefs_show_compression(&segbmap->compression,
				      SSDFS_TUNEFS_SEGMENT_BITMAP_STRING);

	/* PEB mapping table options */
	maptbl = &config->maptbl;
	ssdfs_tunefs_show_backup_copy(&maptbl->has_backup_copy,
				      SSDFS_TUNEFS_MAPPING_TABLE_STRING);
	ssdfs_tunefs_show_log_pages(&maptbl->log_pages,
				    SSDFS_TUNEFS_MAPPING_TABLE_STRING);
	ssdfs_tunefs_show_migration_threshold(&maptbl->migration_threshold,
					      SSDFS_TUNEFS_MAPPING_TABLE_STRING);
	ssdfs_tunefs_show_reserved_pebs_per_fragment(&maptbl->reserved_pebs_per_fragment,
						     SSDFS_TUNEFS_MAPPING_TABLE_STRING);
	ssdfs_tunefs_show_compression(&maptbl->compression,
				      SSDFS_TUNEFS_MAPPING_TABLE_STRING);

	/* btree options */
	btree = &config->btree;
	ssdfs_tunefs_show_min_index_area_size(&btree->min_index_area_size,
						SSDFS_TUNEFS_BTREE_STRING);
	ssdfs_tunefs_show_log_pages(&btree->lnode_log_pages,
				    SSDFS_TUNEFS_BTREE_LNODE_STRING);
	ssdfs_tunefs_show_log_pages(&btree->hnode_log_pages,
				    SSDFS_TUNEFS_BTREE_HNODE_STRING);
	ssdfs_tunefs_show_log_pages(&btree->inode_log_pages,
				    SSDFS_TUNEFS_BTREE_INODE_STRING);

	/* user data options */
	user_data_seg = &config->user_data_seg;
	ssdfs_tunefs_show_log_pages(&user_data_seg->log_pages,
				    SSDFS_TUNEFS_USER_DATA_STRING);
	ssdfs_tunefs_show_migration_threshold(&user_data_seg->migration_threshold,
					      SSDFS_TUNEFS_USER_DATA_STRING);
	ssdfs_tunefs_show_compression(&user_data_seg->compression,
				      SSDFS_TUNEFS_USER_DATA_STRING);
}

static
void ssdfs_tunefs_explain_configuration_failure(struct ssdfs_tunefs_options *options)
{
	SSDFS_ERR("TODO: Function is not implemented yet!!!\n");
}

int main(int argc, char *argv[])
{
	struct ssdfs_volume_environment env = {
		.need_get_config = SSDFS_FALSE,
		.generic.show_debug = SSDFS_FALSE,
		.generic.show_info = SSDFS_TRUE,
		.generic.device_type = SSDFS_DEVICE_TYPE_MAX,
		.options.new_config.label.state = SSDFS_IGNORE_OPTION,
		.options.new_config.blkbmap.has_backup_copy.state = SSDFS_IGNORE_OPTION,
		.options.new_config.blkbmap.has_backup_copy.value = SSDFS_IGNORE_OPTION,
		.options.new_config.blkbmap.compression.state = SSDFS_IGNORE_OPTION,
		.options.new_config.blkbmap.compression.value = SSDFS_UNKNOWN_COMPRESSION,
		.options.new_config.blk2off_tbl.has_backup_copy.state = SSDFS_IGNORE_OPTION,
		.options.new_config.blk2off_tbl.has_backup_copy.value = SSDFS_IGNORE_OPTION,
		.options.new_config.blk2off_tbl.compression.state = SSDFS_IGNORE_OPTION,
		.options.new_config.blk2off_tbl.compression.value = SSDFS_UNKNOWN_COMPRESSION,
		.options.new_config.segbmap.has_backup_copy.state = SSDFS_IGNORE_OPTION,
		.options.new_config.segbmap.has_backup_copy.value = SSDFS_IGNORE_OPTION,
		.options.new_config.segbmap.log_pages.state = SSDFS_IGNORE_OPTION,
		.options.new_config.segbmap.log_pages.value = -1,
		.options.new_config.segbmap.migration_threshold.state = SSDFS_IGNORE_OPTION,
		.options.new_config.segbmap.migration_threshold.value = -1,
		.options.new_config.segbmap.compression.state = SSDFS_IGNORE_OPTION,
		.options.new_config.segbmap.compression.value = SSDFS_UNKNOWN_COMPRESSION,
		.options.new_config.maptbl.has_backup_copy.state = SSDFS_IGNORE_OPTION,
		.options.new_config.maptbl.has_backup_copy.value = SSDFS_IGNORE_OPTION,
		.options.new_config.maptbl.log_pages.state = SSDFS_IGNORE_OPTION,
		.options.new_config.maptbl.log_pages.value = -1,
		.options.new_config.maptbl.migration_threshold.state = SSDFS_IGNORE_OPTION,
		.options.new_config.maptbl.migration_threshold.value = -1,
		.options.new_config.maptbl.reserved_pebs_per_fragment.state = SSDFS_IGNORE_OPTION,
		.options.new_config.maptbl.reserved_pebs_per_fragment.value = -1,
		.options.new_config.maptbl.compression.state = SSDFS_IGNORE_OPTION,
		.options.new_config.maptbl.compression.value = SSDFS_UNKNOWN_COMPRESSION,
		.options.new_config.btree.min_index_area_size.state = SSDFS_IGNORE_OPTION,
		.options.new_config.btree.min_index_area_size.value = -1,
		.options.new_config.btree.lnode_log_pages.state = SSDFS_IGNORE_OPTION,
		.options.new_config.btree.lnode_log_pages.value = -1,
		.options.new_config.btree.hnode_log_pages.state = SSDFS_IGNORE_OPTION,
		.options.new_config.btree.hnode_log_pages.value = -1,
		.options.new_config.btree.inode_log_pages.state = SSDFS_IGNORE_OPTION,
		.options.new_config.btree.inode_log_pages.value = -1,
		.options.new_config.user_data_seg.log_pages.state = SSDFS_IGNORE_OPTION,
		.options.new_config.user_data_seg.log_pages.value = -1,
		.options.new_config.user_data_seg.migration_threshold.state = SSDFS_IGNORE_OPTION,
		.options.new_config.user_data_seg.migration_threshold.value = -1,
		.options.new_config.user_data_seg.compression.state = SSDFS_IGNORE_OPTION,
		.options.new_config.user_data_seg.compression.value = SSDFS_UNKNOWN_COMPRESSION,
	};
	int fd;
	int err = 0;

	parse_options(argc, argv, &env);

	SSDFS_DBG(env.generic.show_debug, "try open: %s\n", argv[optind]);

	fd = open(argv[optind], O_RDWR | O_LARGEFILE);
	if (fd == -1) {
		SSDFS_ERR("unable to open %s: %s\n",
			  argv[optind], strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (env.need_get_config) {
		SSDFS_DBG(env.generic.show_debug, "try get config\n");
		err = ioctl(fd, SSDFS_IOC_TUNEFS_GET_CONFIG, &env.options);
		if (err) {
			SSDFS_ERR("ioctl failed for %s: %s\n",
				  argv[optind], strerror(errno));
			goto tunefs_failed;
		}

		SSDFS_DBG(env.generic.show_debug, "show current configuration\n");

		ssdfs_tunefs_show_current_configuration(&env.options);
	} else {
		SSDFS_DBG(env.generic.show_debug, "show requested configuration\n");

		ssdfs_tunefs_show_requested_configuration(&env.options);

		SSDFS_TUNEFS_SHOW("\n");

		SSDFS_DBG(env.generic.show_debug, "try set config\n");
		err = ioctl(fd, SSDFS_IOC_TUNEFS_SET_CONFIG, &env.options);
		if (err) {
			SSDFS_ERR("ioctl failed for %s: %s\n",
				  argv[optind], strerror(errno));

			ssdfs_tunefs_explain_configuration_failure(&env.options);
			goto tunefs_failed;
		}

		SSDFS_DBG(env.generic.show_debug, "show current configuration\n");

		ssdfs_tunefs_show_current_configuration(&env.options);

		SSDFS_TUNEFS_SHOW("\n");

		SSDFS_DBG(env.generic.show_debug, "show requested configuration\n");

		ssdfs_tunefs_show_requested_configuration(&env.options);

		SSDFS_TUNEFS_SHOW("\n");

		SSDFS_TUNEFS_SHOW("PLEASE, REMOUNT THE VOLUME. "
				  "CONFIGURATION WILL BE CHANGED DURING UNMOUNT.\n");
	}

tunefs_failed:
	close(fd);
	exit(err ? EXIT_FAILURE : EXIT_SUCCESS);
}
