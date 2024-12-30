/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/tune.ssdfs/options.c - parsing command line options functionality.
 *
 * Copyright (c) 2023-2024 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#include <sys/types.h>
#include <getopt.h>

#include "tunefs.h"

/************************************************************************
 *                    Options parsing functionality                     *
 ************************************************************************/

void print_version(void)
{
	SSDFS_INFO("tune.ssdfs, part of %s\n", SSDFS_UTILS_VERSION);
}

void print_usage(void)
{
	SSDFS_TUNEFS_INFO(SSDFS_TRUE, "tune volume of SSDFS file system\n\n");
	SSDFS_INFO("Usage: tune.ssdfs <options> [<device> | <image-file>]\n");
	SSDFS_INFO("Options:\n");
	SSDFS_INFO("\t [-B|--blkbmap has_copy=(enable|disable),"
		   "compression=(none|zlib|lzo)]\t  "
		   "block bitmap options.\n");
	SSDFS_INFO("\t [-d|--debug]\t\t  show debug output.\n");
	SSDFS_INFO("\t [-h|--help]\t\t  display help message and exit.\n");
	SSDFS_INFO("\t [-g|--get_config]\t\t  get current volume configuration.\n");
	SSDFS_INFO("\t [-L|--label]\t\t  set a volume label.\n");
	SSDFS_INFO("\t [-M|--maptbl has_copy=(enable|disable),log_pages=value,"
		   "migration_threshold=value,reserved_pebs_per_fragment=percentage,"
		   "compression=(none|zlib|lzo)]\t  "
		   "PEB mapping table options.\n");
	SSDFS_INFO("\t [-O|--offsets_table has_copy=(enable|disable),"
		   "compression=(none|zlib|lzo)]\t  "
		   "offsets table options.\n");
	SSDFS_INFO("\t [-S|--segbmap has_copy=(enable|disable),log_pages=value,"
		   "migration_threshold=value,compression=(none|zlib|lzo)]\t  "
		   "segment bitmap options.\n");
	SSDFS_INFO("\t [-T|--btree min_index_area_size=value,"
		   "leaf_node_log_pages=value,hybrid_node_log_pages=value,"
		   "index_node_log_pages=value]\t  "
		   "btrees' options.\n");
	SSDFS_INFO("\t [-U|--user_data_segment log_pages=value,"
		   "migration_threshold=value,compression=(none|zlib|lzo)]\t  "
		   "user data segment options.\n");
	SSDFS_INFO("\t [-V|--version]\t\t  print version and exit.\n");
}

static int check_string(const char *str1, const char *str2)
{
	return strncasecmp(str1, str2, strlen(str2));
}

static int detect_option_status(const char *str)
{
	if (check_string(str, SSDFS_ENABLE_OPTION_STRING) == 0)
		return SSDFS_ENABLE_OPTION;
	else if (check_string(str, SSDFS_DISABLE_OPTION_STRING) == 0)
		return SSDFS_DISABLE_OPTION;

	return SSDFS_IGNORE_OPTION;
}

static void check_segbmap_log_pages(int value)
{
	if (value == 0 || value >= USHRT_MAX) {
		SSDFS_ERR("invalid segbmap option: "
			  "log_pages %d is huge\n",
			  value);
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static void check_maptbl_log_pages(int value)
{
	if (value == 0 || value >= USHRT_MAX) {
		SSDFS_ERR("invalid maptbl option: "
			  "log_pages %d is huge\n",
			  value);
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static void check_user_data_seg_log_pages(int value)
{
	if (value == 0 || value >= USHRT_MAX) {
		SSDFS_ERR("invalid user data segment option: "
			  "log_pages %d is huge\n",
			  value);
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static void check_migration_threshold(int value)
{
	if (value == 0 || value >= U16_MAX) {
		SSDFS_ERR("invalid migration threshold option: "
			  "migration_threshold %d is huge\n",
			  value);
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static void check_reserved_pebs_per_fragment(int value)
{
	if (value == 0 || value > 80) {
		SSDFS_ERR("invalid reserved PEBs per fragment %d option: "
			  "Please, use any value 1%%-80%% in the range\n",
			  value);
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static int get_compression_id(char *value)
{
	int id = SSDFS_UNCOMPRESSED_BLOB;

	if (strcmp(value, SSDFS_NONE_COMPRESSION_STRING) == 0)
		id = SSDFS_UNCOMPRESSED_BLOB;
	else if (strcmp(value, SSDFS_ZLIB_COMPRESSION_STRING) == 0)
		id = SSDFS_ZLIB_BLOB;
	else if (strcmp(value, SSDFS_LZO_COMPRESSION_STRING) == 0)
		id = SSDFS_LZO_BLOB;
	else {
		SSDFS_ERR("Unsupported compression type %s.\n",
			  value);
		print_usage();
		exit(EXIT_FAILURE);
	}

	return id;
}

static void check_btree_min_index_area_size(int min_index_area_size)
{
	u32 index_size = sizeof(struct ssdfs_btree_index_key);

	if (min_index_area_size % index_size) {
		SSDFS_ERR("invalid minimal index area size option: "
			  "min_index_area_size %d, "
			  "index_size %u\n",
			  min_index_area_size,
			  index_size);
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static void check_btree_node_log_pages(int value)
{
	if (value == 0 || value >= USHRT_MAX) {
		SSDFS_ERR("invalid btree node segment option: "
			  "log_pages %d is huge\n",
			  value);
		print_usage();
		exit(EXIT_FAILURE);
	}
}

void parse_options(int argc, char *argv[],
		   struct ssdfs_volume_environment *env)
{
	int c;
	int oi = 1;
	char *p;
	char sopts[] = "B:dhgL:M:O:S:T:U:V";
	static const struct option lopts[] = {
		{"blkbmap", 1, NULL, 'B'},
		{"debug", 0, NULL, 'd'},
		{"help", 0, NULL, 'h'},
		{"get_config", 0, NULL, 'g'},
		{"label", 1, NULL, 'L'},
		{"maptbl", 1, NULL, 'M'},
		{"offsets_table", 1, NULL, 'O'},
		{"segbmap", 1, NULL, 'S'},
		{"btree", 1, NULL, 'T'},
		{"user_data_segment", 1, NULL, 'U'},
		{"version", 0, NULL, 'V'},
		{ }
	};
	enum {
		BLKBMAP_HAS_COPY_OPT = 0,
		BLKBMAP_COMPRESSION_OPT,
	};
	char *const blkbmap_tokens[] = {
		[BLKBMAP_HAS_COPY_OPT]		= "has_copy",
		[BLKBMAP_COMPRESSION_OPT]	= "compression",
		NULL
	};
	enum {
		BLK2OFF_TABLE_HAS_COPY_OPT = 0,
		BLK2OFF_TABLE_COMPRESSION_OPT,
	};
	char *const blk2off_tbl_tokens[] = {
		[BLK2OFF_TABLE_HAS_COPY_OPT]	= "has_copy",
		[BLK2OFF_TABLE_COMPRESSION_OPT]	= "compression",
		NULL
	};
	enum {
		MAPTBL_HAS_COPY_OPT = 0,
		MAPTBL_LOG_PAGES_OPT,
		MAPTBL_MIGRATION_THRESHOLD_OPT,
		MAPTBL_RESERVED_PEBS_PER_FRAGMENT_OPT,
		MAPTBL_COMPRESSION_OPT,
	};
	char *const maptbl_tokens[] = {
		[MAPTBL_HAS_COPY_OPT]		 = "has_copy",
		[MAPTBL_LOG_PAGES_OPT]		 = "log_pages",
		[MAPTBL_MIGRATION_THRESHOLD_OPT] = "migration_threshold",
		[MAPTBL_RESERVED_PEBS_PER_FRAGMENT_OPT] =
					    "reserved_pebs_per_fragment",
		[MAPTBL_COMPRESSION_OPT]	 = "compression",
		NULL
	};
	enum {
		SEGBMAP_HAS_COPY_OPT = 0,
		SEGBMAP_LOG_PAGES_OPT,
		SEGBMAP_MIGRATION_THRESHOLD_OPT,
		SEGBMAP_COMPRESSION_OPT,
	};
	char *const segbmap_tokens[] = {
		[SEGBMAP_HAS_COPY_OPT]		  = "has_copy",
		[SEGBMAP_LOG_PAGES_OPT]		  = "log_pages",
		[SEGBMAP_MIGRATION_THRESHOLD_OPT] = "migration_threshold",
		[SEGBMAP_COMPRESSION_OPT]	  = "compression",
		NULL
	};
	enum {
		BTREE_MIN_INDEX_AREA_SIZE_OPT = 0,
		BTREE_LNODE_LOG_PAGES_OPT,
		BTREE_HNODE_LOG_PAGES_OPT,
		BTREE_INODE_LOG_PAGES_OPT,
	};
	char *const btree_tokens[] = {
		[BTREE_MIN_INDEX_AREA_SIZE_OPT]	  = "min_index_area_size",
		[BTREE_LNODE_LOG_PAGES_OPT]	  = "leaf_node_log_pages",
		[BTREE_HNODE_LOG_PAGES_OPT]	  = "hybrid_node_log_pages",
		[BTREE_INODE_LOG_PAGES_OPT]	  = "index_node_log_pages",
		NULL
	};
	enum {
		USER_DATA_LOG_PAGES_OPT = 0,
		USER_DATA_MIGRATION_THRESHOLD_OPT,
		USER_DATA_COMPRESSION_OPT,
	};
	char *const dataseg_tokens[] = {
		[USER_DATA_LOG_PAGES_OPT]	    = "log_pages",
		[USER_DATA_MIGRATION_THRESHOLD_OPT] = "migration_threshold",
		[USER_DATA_COMPRESSION_OPT]	    = "compression",
		NULL
	};

	while ((c = getopt_long(argc, argv, sopts, lopts, &oi)) != EOF) {
		switch (c) {
		case 'B':
			p = optarg;
			while (*p != '\0') {
				struct ssdfs_tunefs_blkbmap_options *blkbmap;
				char *value;

				blkbmap = &env->options.new_config.blkbmap;
				switch (getsubopt(&p, blkbmap_tokens, &value)) {
				case BLKBMAP_HAS_COPY_OPT:
					blkbmap->has_backup_copy.state =
						detect_option_status(value);
					blkbmap->has_backup_copy.value =
						blkbmap->has_backup_copy.state;
					break;
				case BLKBMAP_COMPRESSION_OPT:
					blkbmap->compression.state =
							SSDFS_ENABLE_OPTION;
					blkbmap->compression.value =
						get_compression_id(value);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'd':
			env->generic.show_debug = SSDFS_TRUE;
			break;
		case 'h':
			print_usage();
			exit(EXIT_SUCCESS);
		case 'g':
			env->need_get_config = SSDFS_TRUE;
			break;
		case 'L':
			{
				struct ssdfs_tunefs_volume_label_option *label;

				label = &env->options.new_config.label;
				label->state = SSDFS_ENABLE_OPTION;
				strncpy(label->volume_label, optarg,
					sizeof(label->volume_label) - 1);
			}
			break;
		case 'M':
			p = optarg;
			while (*p != '\0') {
				struct ssdfs_tunefs_maptbl_options *maptbl;
				char *value;
				int count;

				maptbl = &env->options.new_config.maptbl;
				switch (getsubopt(&p, maptbl_tokens, &value)) {
				case MAPTBL_HAS_COPY_OPT:
					maptbl->has_backup_copy.state =
						detect_option_status(value);
					maptbl->has_backup_copy.value =
						maptbl->has_backup_copy.state;
					break;
				case MAPTBL_LOG_PAGES_OPT:
					count = atoi(value);
					check_maptbl_log_pages(count);
					maptbl->log_pages.state =
							SSDFS_ENABLE_OPTION;
					maptbl->log_pages.value = (u16)count;
					break;
				case MAPTBL_MIGRATION_THRESHOLD_OPT:
					count = atoi(value);
					check_migration_threshold(count);
					maptbl->migration_threshold.state =
							SSDFS_ENABLE_OPTION;
					maptbl->migration_threshold.value =
								    (u16)count;
					break;
				case MAPTBL_RESERVED_PEBS_PER_FRAGMENT_OPT:
					count = atoi(value);
					check_reserved_pebs_per_fragment(count);
					maptbl->reserved_pebs_per_fragment.state =
							SSDFS_ENABLE_OPTION;
					maptbl->reserved_pebs_per_fragment.value =
								    (u16)count;
					break;
				case MAPTBL_COMPRESSION_OPT:
					maptbl->compression.state =
							SSDFS_ENABLE_OPTION;
					maptbl->compression.value =
						get_compression_id(value);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'O':
			p = optarg;
			while (*p != '\0') {
				struct ssdfs_tunefs_blk2off_table_options *blk2off_tbl;
				char *value;

				blk2off_tbl = &env->options.new_config.blk2off_tbl;
				switch (getsubopt(&p, blk2off_tbl_tokens,
						  &value)) {
				case BLK2OFF_TABLE_HAS_COPY_OPT:
					blk2off_tbl->has_backup_copy.state =
						detect_option_status(value);
					blk2off_tbl->has_backup_copy.value =
						blk2off_tbl->has_backup_copy.state;
					break;
				case BLK2OFF_TABLE_COMPRESSION_OPT:
					blk2off_tbl->compression.state =
							SSDFS_ENABLE_OPTION;
					blk2off_tbl->compression.value =
						get_compression_id(value);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'S':
			p = optarg;
			while (*p != '\0') {
				struct ssdfs_tunefs_segbmap_options *segbmap;
				char *value;
				int count;

				segbmap = &env->options.new_config.segbmap;
				switch (getsubopt(&p, segbmap_tokens, &value)) {
				case SEGBMAP_HAS_COPY_OPT:
					segbmap->has_backup_copy.state =
						detect_option_status(value);
					segbmap->has_backup_copy.value =
						segbmap->has_backup_copy.state;
					break;
				case SEGBMAP_LOG_PAGES_OPT:
					count = atoi(value);
					check_segbmap_log_pages(count);
					segbmap->log_pages.state =
							SSDFS_ENABLE_OPTION;
					segbmap->log_pages.value = (u16)count;
					break;
				case SEGBMAP_MIGRATION_THRESHOLD_OPT:
					count = atoi(value);
					check_migration_threshold(count);
					segbmap->migration_threshold.state =
							SSDFS_ENABLE_OPTION;
					segbmap->migration_threshold.value =
								    (u16)count;
					break;
				case SEGBMAP_COMPRESSION_OPT:
					segbmap->compression.state =
							SSDFS_ENABLE_OPTION;
					segbmap->compression.value =
						get_compression_id(value);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'T':
			p = optarg;
			while (*p != '\0') {
				struct ssdfs_tunefs_btree_options *btree;
				char *value;
				int count;

				btree = &env->options.new_config.btree;
				switch (getsubopt(&p, btree_tokens, &value)) {
				case BTREE_MIN_INDEX_AREA_SIZE_OPT:
					count = atoi(value);
					check_btree_min_index_area_size(count);
					btree->min_index_area_size.state =
							SSDFS_ENABLE_OPTION;
					btree->min_index_area_size.value =
								    (u16)count;
					break;
				case BTREE_LNODE_LOG_PAGES_OPT:
					count = atoi(value);
					check_btree_node_log_pages(count);
					btree->lnode_log_pages.state =
							SSDFS_ENABLE_OPTION;
					btree->lnode_log_pages.value =
								    (u16)count;
					break;
				case BTREE_HNODE_LOG_PAGES_OPT:
					count = atoi(value);
					check_btree_node_log_pages(count);
					btree->hnode_log_pages.state =
							SSDFS_ENABLE_OPTION;
					btree->hnode_log_pages.value =
								    (u16)count;
					break;
				case BTREE_INODE_LOG_PAGES_OPT:
					count = atoi(value);
					check_btree_node_log_pages(count);
					btree->inode_log_pages.state =
							SSDFS_ENABLE_OPTION;
					btree->inode_log_pages.value =
								    (u16)count;
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'U':
			p = optarg;
			while (*p != '\0') {
				struct ssdfs_tunefs_user_data_options *data_seg;
				char *value;
				int count;

				data_seg = &env->options.new_config.user_data_seg;
				switch (getsubopt(&p, dataseg_tokens, &value)) {
				case USER_DATA_LOG_PAGES_OPT:
					count = atoi(value);
					check_user_data_seg_log_pages(count);
					data_seg->log_pages.state =
							SSDFS_ENABLE_OPTION;
					data_seg->log_pages.value =
								    (u16)count;
					break;
				case USER_DATA_MIGRATION_THRESHOLD_OPT:
					count = atoi(value);
					check_migration_threshold(count);
					data_seg->migration_threshold.state =
							SSDFS_ENABLE_OPTION;
					data_seg->migration_threshold.value =
								    (u16)count;
					break;
				case USER_DATA_COMPRESSION_OPT:
					data_seg->compression.state =
							SSDFS_ENABLE_OPTION;
					data_seg->compression.value =
						get_compression_id(value);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'V':
			print_version();
			exit(EXIT_SUCCESS);
		default:
			print_usage();
			exit(EXIT_FAILURE);
		}
	}

	if (optind != argc - 1) {
		print_usage();
		exit(EXIT_FAILURE);
	}
}
