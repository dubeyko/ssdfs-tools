//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/mkfs.ssdfs/options.c - parsing command line options functionality.
 *
 * Copyright (c) 2014-2022 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 *
 * HGST Confidential
 * (C) Copyright 2014-2022, HGST, Inc., All rights reserved.
 *
 * Created by HGST, San Jose Research Center, Storage Architecture Group
 * Authors: Vyacheslav Dubeyko <slava@dubeyko.com>
 *
 * Acknowledgement: Cyril Guyot <Cyril.Guyot@wdc.com>
 *                  Zvonimir Bandic <Zvonimir.Bandic@wdc.com>
 */

#include <sys/types.h>
#include <getopt.h>

#include "mkfs.h"

#define SSDFS_256B_STRING	"265B"
#define SSDFS_512B_STRING	"512B"
#define SSDFS_1KB_STRING	"1KB"
#define SSDFS_2KB_STRING	"2KB"
#define SSDFS_4KB_STRING	"4KB"
#define SSDFS_8KB_STRING	"8KB"
#define SSDFS_16KB_STRING	"16KB"
#define SSDFS_32KB_STRING	"32KB"
#define SSDFS_64KB_STRING	"64KB"
#define SSDFS_128KB_STRING	"128KB"
#define SSDFS_256KB_STRING	"256KB"
#define SSDFS_512KB_STRING	"512KB"
#define SSDFS_2MB_STRING	"2MB"
#define SSDFS_8MB_STRING	"8MB"
#define SSDFS_16MB_STRING	"16MB"
#define SSDFS_32MB_STRING	"32MB"
#define SSDFS_64MB_STRING	"64MB"
#define SSDFS_128MB_STRING	"128MB"
#define SSDFS_256MB_STRING	"256MB"
#define SSDFS_512MB_STRING	"512MB"
#define SSDFS_1GB_STRING	"1GB"
#define SSDFS_2GB_STRING	"2GB"
#define SSDFS_8GB_STRING	"8GB"
#define SSDFS_16GB_STRING	"16GB"
#define SSDFS_32GB_STRING	"32GB"
#define SSDFS_64GB_STRING	"64GB"

/************************************************************************
 *                    Options parsing functionality                     *
 ************************************************************************/

void print_version(void)
{
	SSDFS_INFO("mkfs.ssdfs, part of %s\n", SSDFS_UTILS_VERSION);
}

void print_usage(void)
{
	SSDFS_MKFS_INFO(SSDFS_TRUE, "create volume of SSDFS file system\n\n");
	SSDFS_INFO("Usage: mkfs.ssdfs <options> [<device> | <image-file>]\n");
	SSDFS_INFO("Options:\n");
	SSDFS_INFO("\t [-B|--blkbmap has_copy,compression=(none|zlib|lzo)]\t  "
		   "block bitmap options.\n");
	SSDFS_INFO("\t [-C|--compression (none|zlib|lzo)]\t  "
		   "compression type support.\n");
	SSDFS_INFO("\t [-D|--nand-dies count]\t  NAND dies count.\n");
	SSDFS_INFO("\t [-d|--debug]\t\t  show debug output.\n");
	SSDFS_INFO("\t [-e|--erasesize size]\t  erase size of target device "
		   "(128KB|256KB|512KB|2MB|8MB).\n");
	SSDFS_INFO("\t [-f|--force]\t\t  force overwrite of existing filesystem.\n");
	SSDFS_INFO("\t [-h|--help]\t\t  display help message and exit.\n");
	SSDFS_INFO("\t [-i|--inode_size size]\t  inode size in bytes "
		   "(265B|512B|1KB|2KB|4KB).\n");
	SSDFS_INFO("\t [-K|--not-erase-device]  do not erase device by mkfs.\n");
	SSDFS_INFO("\t [-L|--label]\t\t  set a volume label.\n");
	SSDFS_INFO("\t [-M|--maptbl has_copy,stripes_per_fragment=value,"
		   "fragments_per_peb=value,log_pages=value,"
		   "migration_threshold=value,"
		   "reserved_pebs_per_fragment=percentage,"
		   "compression=(none|zlib|lzo)]\t  "
		   "PEB mapping table options.\n");
	SSDFS_INFO("\t [-m|--migration-threshold]  max amount of migration PEBs "
		   "for segment.\n");
	SSDFS_INFO("\t [-O|--offsets_table has_copy,"
		   "compression=(none|zlib|lzo)]\t  "
		   "offsets table options.\n");
	SSDFS_INFO("\t [-p|--pagesize size]\t  page size of target device "
		   "(4KB|8KB|16KB|32KB).\n");
	SSDFS_INFO("\t [-q|--quiet]\t\t  quiet execution "
		   "(useful for scripts).\n");
	SSDFS_INFO("\t [-S|--segbmap has_copy,segs_per_chain=value,"
		   "fragments_per_peb=value,log_pages=value,"
		   "migration_threshold=value,compression=(none|zlib|lzo)]\t  "
		   "segment bitmap options.\n");
	SSDFS_INFO("\t [-s|--segsize size]\t  segment size of target device "
		   "(128KB|256KB|512KB|2MB|8MB|16MB|32MB|64MB|...).\n");
	SSDFS_INFO("\t [-T|--btree node_size=value,min_index_area_size=value,"
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

static u64 detect_granularity(const char *str)
{
	if (check_string(str, SSDFS_256B_STRING) == 0)
		return SSDFS_256B;
	else if (check_string(str, SSDFS_512B_STRING) == 0)
		return SSDFS_512B;
	else if (check_string(str, SSDFS_1KB_STRING) == 0)
		return SSDFS_1KB;
	else if (check_string(str, SSDFS_2KB_STRING) == 0)
		return SSDFS_2KB;
	else if (check_string(str, SSDFS_4KB_STRING) == 0)
		return SSDFS_4KB;
	else if (check_string(str, SSDFS_8KB_STRING) == 0)
		return SSDFS_8KB;
	else if (check_string(str, SSDFS_16KB_STRING) == 0)
		return SSDFS_16KB;
	else if (check_string(str, SSDFS_32KB_STRING) == 0)
		return SSDFS_32KB;
	else if (check_string(str, SSDFS_64KB_STRING) == 0)
		return SSDFS_64KB;
	else if (check_string(str, SSDFS_128KB_STRING) == 0)
		return SSDFS_128KB;
	else if (check_string(str, SSDFS_256KB_STRING) == 0)
		return SSDFS_256KB;
	else if (check_string(str, SSDFS_512KB_STRING) == 0)
		return SSDFS_512KB;
	else if (check_string(str, SSDFS_2MB_STRING) == 0)
		return SSDFS_2MB;
	else if (check_string(str, SSDFS_8MB_STRING) == 0)
		return SSDFS_8MB;
	else if (check_string(str, SSDFS_16MB_STRING) == 0)
		return SSDFS_16MB;
	else if (check_string(str, SSDFS_32MB_STRING) == 0)
		return SSDFS_32MB;
	else if (check_string(str, SSDFS_64MB_STRING) == 0)
		return SSDFS_64MB;
	else if (check_string(str, SSDFS_128MB_STRING) == 0)
		return SSDFS_128MB;
	else if (check_string(str, SSDFS_256MB_STRING) == 0)
		return SSDFS_256MB;
	else if (check_string(str, SSDFS_512MB_STRING) == 0)
		return SSDFS_512MB;
	else if (check_string(str, SSDFS_1GB_STRING) == 0)
		return SSDFS_1GB;
	else if (check_string(str, SSDFS_2GB_STRING) == 0)
		return SSDFS_2GB;
	else if (check_string(str, SSDFS_8GB_STRING) == 0)
		return SSDFS_8GB;
	else if (check_string(str, SSDFS_16GB_STRING) == 0)
		return SSDFS_16GB;
	else if (check_string(str, SSDFS_32GB_STRING) == 0)
		return SSDFS_32GB;
	else if (check_string(str, SSDFS_64GB_STRING) == 0)
		return SSDFS_64GB;

	return U64_MAX;
}

static void check_pagesize(int pagesize)
{
	switch (pagesize) {
	case SSDFS_4KB:
	case SSDFS_8KB:
	case SSDFS_16KB:
	case SSDFS_32KB:
		/* do nothing: proper value */
		break;

	default:
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static void check_segsize(u64 segsize)
{
	switch (segsize) {
	case SSDFS_128KB:
	case SSDFS_256KB:
	case SSDFS_512KB:
	case SSDFS_2MB:
	case SSDFS_8MB:
	case SSDFS_16MB:
	case SSDFS_32MB:
	case SSDFS_64MB:
	case SSDFS_128MB:
	case SSDFS_256MB:
	case SSDFS_512MB:
	case SSDFS_1GB:
	case SSDFS_2GB:
	case SSDFS_8GB:
	case SSDFS_16GB:
	case SSDFS_32GB:
	case SSDFS_64GB:
		/* do nothing: proper value */
		break;

	default:
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static void check_erasesize(u64 erasesize)
{
	switch (erasesize) {
	case SSDFS_128KB:
	case SSDFS_256KB:
	case SSDFS_512KB:
	case SSDFS_2MB:
	case SSDFS_8MB:
		/* do nothing: proper value */
		break;

	default:
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static void check_segbmap_segs_per_chain(int *value)
{
	if (*value <= 0) {
		*value = 1;
		SSDFS_WARN("invalid segbmap option: "
			   "segs_per_chain will equal to %d\n",
			   *value);
	} else if (*value > SSDFS_SEGBMAP_SEGS) {
		*value = SSDFS_SEGBMAP_SEGS;
		SSDFS_WARN("invalid segbmap option: "
			   "segs_per_chain will equal to %d\n",
			   *value);
	}
}

static void check_segbmap_fragments_per_peb(int value)
{
	if (value >= USHRT_MAX) {
		SSDFS_ERR("invalid segbmap option: "
			  "fragments_per_peb %d is huge\n",
			  value);
		print_usage();
		exit(EXIT_FAILURE);
	}
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

static void check_nand_dies_count(int value)
{
	if (value % 2) {
		SSDFS_ERR("invalid nand-dies option: "
			  "nand-dies %d is odd\n",
			  value);
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static void check_maptbl_stripes_per_fragment(int value)
{
	if (value >= USHRT_MAX) {
		SSDFS_ERR("invalid maptbl option: "
			  "stripes_per_fragment %d is huge\n",
			  value);
		print_usage();
		exit(EXIT_FAILURE);
	}
}

static void check_maptbl_fragments_per_peb(int value)
{
	if (value >= USHRT_MAX) {
		SSDFS_ERR("invalid maptbl option: "
			  "fragments_per_peb %d is huge\n",
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

	if (strcmp(value, "none") == 0)
		id = SSDFS_UNCOMPRESSED_BLOB;
	else if (strcmp(value, "zlib") == 0)
		id = SSDFS_ZLIB_BLOB;
	else if (strcmp(value, "lzo") == 0)
		id = SSDFS_LZO_BLOB;
	else {
		print_usage();
		exit(EXIT_FAILURE);
	}

	return id;
}

static void check_btree_node_size(int node_size)
{
	switch (node_size) {
	case SSDFS_4KB:
	case SSDFS_8KB:
	case SSDFS_16KB:
	case SSDFS_32KB:
	case SSDFS_64KB:
		/* do nothing: proper value */
		break;

	default:
		print_usage();
		exit(EXIT_FAILURE);
	}
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

static void check_inode_size(u16 inode_size)
{
	switch (inode_size) {
	case SSDFS_256B:
	case SSDFS_512B:
	case SSDFS_1KB:
	case SSDFS_2KB:
	case SSDFS_4KB:
		/* do nothing: proper value */
		break;

	default:
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
		   struct ssdfs_volume_layout *layout)
{
	int c;
	int oi = 1;
	char *p;
	u64 granularity;
	char sopts[] = "B:C:D:de:fhi:KL:M:m:O:p:qS:s:T:U:V";
	static const struct option lopts[] = {
		{"blkbmap", 1, NULL, 'B'},
		{"compression", 1, NULL, 'C'},
		{"nand-dies", 1, NULL, 'D'},
		{"debug", 0, NULL, 'd'},
		{"erasesize", 1, NULL, 'e'},
		{"force", 0, NULL, 'f'},
		{"help", 0, NULL, 'h'},
		{"inode_size", 1, NULL, 'i'},
		{"not-erase-device", 0, NULL, 'K'},
		{"label", 1, NULL, 'L'},
		{"maptbl", 1, NULL, 'M'},
		{"migration-threshold", 1, NULL, 'm'},
		{"offsets_table", 1, NULL, 'O'},
		{"pagesize", 1, NULL, 'p'},
		{"quiet", 0, NULL, 'q'},
		{"segbmap", 1, NULL, 'S'},
		{"segsize", 1, NULL, 's'},
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
		MAPTBL_STRIPES_PER_FRAG_OPT,
		MAPTBL_FRAGS_PER_PEB_OPT,
		MAPTBL_LOG_PAGES_OPT,
		MAPTBL_MIGRATION_THRESHOLD_OPT,
		MAPTBL_RESERVED_PEBS_PER_FRAGMENT_OPT,
		MAPTBL_COMPRESSION_OPT,
	};
	char *const maptbl_tokens[] = {
		[MAPTBL_HAS_COPY_OPT]		 = "has_copy",
		[MAPTBL_STRIPES_PER_FRAG_OPT]	 = "stripes_per_fragment",
		[MAPTBL_FRAGS_PER_PEB_OPT]	 = "fragments_per_peb",
		[MAPTBL_LOG_PAGES_OPT]		 = "log_pages",
		[MAPTBL_MIGRATION_THRESHOLD_OPT] = "migration_threshold",
		[MAPTBL_RESERVED_PEBS_PER_FRAGMENT_OPT] =
					    "reserved_pebs_per_fragment",
		[MAPTBL_COMPRESSION_OPT]	 = "compression",
		NULL
	};
	enum {
		SEGBMAP_HAS_COPY_OPT = 0,
		SEGBMAP_SEGS_PER_CHAIN_OPT,
		SEGBMAP_FRAGS_PER_PEB_OPT,
		SEGBMAP_LOG_PAGES_OPT,
		SEGBMAP_MIGRATION_THRESHOLD_OPT,
		SEGBMAP_COMPRESSION_OPT,
	};
	char *const segbmap_tokens[] = {
		[SEGBMAP_HAS_COPY_OPT]		  = "has_copy",
		[SEGBMAP_SEGS_PER_CHAIN_OPT]	  = "segs_per_chain",
		[SEGBMAP_FRAGS_PER_PEB_OPT]	  = "fragments_per_peb",
		[SEGBMAP_LOG_PAGES_OPT]		  = "log_pages",
		[SEGBMAP_MIGRATION_THRESHOLD_OPT] = "migration_threshold",
		[SEGBMAP_COMPRESSION_OPT]	  = "compression",
		NULL
	};
	enum {
		BTREE_NODE_SIZE_OPT = 0,
		BTREE_MIN_INDEX_AREA_SIZE_OPT,
		BTREE_LNODE_LOG_PAGES_OPT,
		BTREE_HNODE_LOG_PAGES_OPT,
		BTREE_INODE_LOG_PAGES_OPT,
	};
	char *const btree_tokens[] = {
		[BTREE_NODE_SIZE_OPT]		  = "node_size",
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
				struct ssdfs_blkbmap_layout *blkbmap;
				char *value;

				blkbmap = &layout->blkbmap;
				switch (getsubopt(&p, blkbmap_tokens, &value)) {
				case BLKBMAP_HAS_COPY_OPT:
					blkbmap->has_backup_copy = SSDFS_TRUE;
					break;
				case BLKBMAP_COMPRESSION_OPT:
					blkbmap->compression =
						get_compression_id(value);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'C':
			layout->compression = get_compression_id(optarg);
			break;
		case 'D':
			layout->nand_dies_count = atoi(optarg);
			check_nand_dies_count(layout->nand_dies_count);
			break;
		case 'd':
			layout->env.show_debug = SSDFS_TRUE;
			break;
		case 'e':
			granularity = detect_granularity(optarg);
			if (granularity >= U64_MAX) {
				layout->env.erase_size = atol(optarg);
				check_erasesize(layout->env.erase_size);
			} else {
				check_erasesize(granularity);
				layout->env.erase_size = (long)granularity;
			}
			break;
		case 'f':
			layout->force_overwrite = SSDFS_TRUE;
			break;
		case 'h':
			print_usage();
			exit(EXIT_SUCCESS);
		case 'i':
			granularity = detect_granularity(optarg);
			if (granularity >= U64_MAX) {
				layout->inode_size = (u16)atoi(optarg);
				check_inode_size(layout->inode_size);
			} else {
				check_inode_size(granularity);
				layout->inode_size = (u16)granularity;
			}
			break;
		case 'K':
			layout->need_erase_device = SSDFS_FALSE;
			break;
		case 'L':
			strncpy(layout->volume_label, optarg,
				sizeof(layout->volume_label));
			break;
		case 'M':
			p = optarg;
			while (*p != '\0') {
				struct ssdfs_maptbl_layout *maptbl;
				char *value;
				int count;

				maptbl = &layout->maptbl;
				switch (getsubopt(&p, maptbl_tokens, &value)) {
				case MAPTBL_HAS_COPY_OPT:
					maptbl->has_backup_copy = SSDFS_TRUE;
					break;
				case MAPTBL_STRIPES_PER_FRAG_OPT:
					count = atoi(value);
					check_maptbl_stripes_per_fragment(count);
					maptbl->stripes_per_portion = (u16)count;
					break;
				case MAPTBL_FRAGS_PER_PEB_OPT:
					count = atoi(value);
					check_maptbl_fragments_per_peb(count);
					maptbl->portions_per_fragment =
								    (u16)count;
					break;
				case MAPTBL_LOG_PAGES_OPT:
					count = atoi(value);
					check_maptbl_log_pages(count);
					maptbl->log_pages = (u16)count;
					break;
				case MAPTBL_MIGRATION_THRESHOLD_OPT:
					count = atoi(value);
					check_migration_threshold(count);
					maptbl->migration_threshold =
								    (u16)count;
					break;
				case MAPTBL_RESERVED_PEBS_PER_FRAGMENT_OPT:
					count = atoi(value);
					check_reserved_pebs_per_fragment(count);
					maptbl->reserved_pebs_per_fragment =
								    (u16)count;
					break;
				case MAPTBL_COMPRESSION_OPT:
					maptbl->compression =
						get_compression_id(value);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'm':
			layout->migration_threshold = atoi(optarg);
			check_migration_threshold(layout->migration_threshold);
			break;
		case 'O':
			p = optarg;
			while (*p != '\0') {
				struct ssdfs_blk2off_table_layout *blk2off_tbl;
				char *value;

				blk2off_tbl = &layout->blk2off_tbl;
				switch (getsubopt(&p, blk2off_tbl_tokens,
						  &value)) {
				case BLK2OFF_TABLE_HAS_COPY_OPT:
					blk2off_tbl->has_backup_copy =
								SSDFS_TRUE;
					break;
				case BLK2OFF_TABLE_COMPRESSION_OPT:
					blk2off_tbl->compression =
						get_compression_id(value);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'p':
			granularity = detect_granularity(optarg);
			if (granularity >= U64_MAX) {
				layout->page_size = atoi(optarg);
				check_pagesize(layout->page_size);
			} else {
				check_pagesize(granularity);
				layout->page_size = (u32)granularity;
			}
			break;
		case 'q':
			layout->env.show_info = SSDFS_FALSE;
			break;
		case 'S':
			p = optarg;
			while (*p != '\0') {
				struct ssdfs_segbmap_layout *segbmap;
				char *value;
				int count;

				segbmap = &layout->segbmap;
				switch (getsubopt(&p, segbmap_tokens, &value)) {
				case SEGBMAP_HAS_COPY_OPT:
					segbmap->has_backup_copy = SSDFS_TRUE;
					break;
				case SEGBMAP_SEGS_PER_CHAIN_OPT:
					count = atoi(value);
					check_segbmap_segs_per_chain(&count);
					segbmap->segs_per_chain = (u8)count;
					break;
				case SEGBMAP_FRAGS_PER_PEB_OPT:
					count = atoi(value);
					check_segbmap_fragments_per_peb(count);
					segbmap->fragments_per_peb = (u16)count;
					break;
				case SEGBMAP_LOG_PAGES_OPT:
					count = atoi(value);
					check_segbmap_log_pages(count);
					segbmap->log_pages = (u16)count;
					break;
				case SEGBMAP_MIGRATION_THRESHOLD_OPT:
					count = atoi(value);
					check_migration_threshold(count);
					segbmap->migration_threshold =
								    (u16)count;
					break;
				case SEGBMAP_COMPRESSION_OPT:
					segbmap->compression =
						get_compression_id(value);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 's':
			granularity = detect_granularity(optarg);
			if (granularity >= U64_MAX) {
				layout->seg_size = atoll(optarg);
				check_segsize(layout->seg_size);
			} else {
				check_segsize(granularity);
				layout->seg_size = granularity;
			}
			break;
		case 'T':
			p = optarg;
			while (*p != '\0') {
				struct ssdfs_btree_layout *btree;
				char *value;
				int count;

				btree = &layout->btree;
				switch (getsubopt(&p, btree_tokens, &value)) {
				case BTREE_NODE_SIZE_OPT:
					count = atoi(value);
					check_btree_node_size(count);
					btree->node_size = (u32)count;
					break;
				case BTREE_MIN_INDEX_AREA_SIZE_OPT:
					count = atoi(value);
					check_btree_min_index_area_size(count);
					btree->min_index_area_size = (u16)count;
					break;
				case BTREE_LNODE_LOG_PAGES_OPT:
					count = atoi(value);
					check_btree_node_log_pages(count);
					btree->lnode_log_pages = (u16)count;
					break;
				case BTREE_HNODE_LOG_PAGES_OPT:
					count = atoi(value);
					check_btree_node_log_pages(count);
					btree->hnode_log_pages = (u16)count;
					break;
				case BTREE_INODE_LOG_PAGES_OPT:
					count = atoi(value);
					check_btree_node_log_pages(count);
					btree->inode_log_pages = (u16)count;
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
				struct ssdfs_user_data_layout *data_seg;
				char *value;
				int count;

				data_seg = &layout->user_data_seg;
				switch (getsubopt(&p, dataseg_tokens, &value)) {
				case USER_DATA_LOG_PAGES_OPT:
					count = atoi(value);
					check_user_data_seg_log_pages(count);
					data_seg->log_pages = (u16)count;
					break;
				case USER_DATA_MIGRATION_THRESHOLD_OPT:
					count = atoi(value);
					check_migration_threshold(count);
					data_seg->migration_threshold =
								    (u16)count;
					break;
				case USER_DATA_COMPRESSION_OPT:
					data_seg->compression =
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
