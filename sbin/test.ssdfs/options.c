/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/test.ssdfs/options.c - parsing command line options functionality.
 *
 * Copyright (c) 2021-2024 Viacheslav Dubeyko <slava@dubeyko.com>
 * All rights reserved.
 *              http://www.ssdfs.org/
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#include <sys/types.h>
#include <getopt.h>

#include "testing.h"

/************************************************************************
 *                    Options parsing functionality                     *
 ************************************************************************/

static void print_version(void)
{
	SSDFS_INFO("test.ssdfs, part of %s\n", SSDFS_UTILS_VERSION);
}

void print_usage(void)
{
	SSDFS_TESTFS_INFO(SSDFS_TRUE, "test SSDFS file system\n\n");
	SSDFS_INFO("Usage: test.ssdfs <options> [<device> | <image-file>]\n");
	SSDFS_INFO("Options:\n");
	SSDFS_INFO("\t [-a|--all]\t\t  test all subsystems.\n");
	SSDFS_INFO("\t [-b|--block-bitmap capacity=value,"
		   "pre-alloc=value,alloc=value,invalidate=value,"
		   "reserve=value]\t  define block bitmap testing options.\n");
	SSDFS_INFO("\t [-d|--shared-dictionary names_number=value,"
		   "name_len=value,step_factor=value]\t  "
		   "define shared dictionary testing options.\n");
	SSDFS_INFO("\t [-D|--shared-extents-tree extents_number=value,"
		   "extent_len=value,ref_count_max=value]\t  "
		   "define shared extents tree testing options.\n");
	SSDFS_INFO("\t [-e|--extent max_len=value]\t  "
		   "define extent related thresholds.\n");
	SSDFS_INFO("\t [-f|--file max_count=value,max_size=value]\t  "
		   "define file related thresholds.\n");
	SSDFS_INFO("\t [-h|--help]\t\t  display help message and exit.\n");
	SSDFS_INFO("\t [-m|--mapping-table iterations=value,"
		   "mappings_per_iter=value,"
		   "add_migrations_per_iter=value,"
		   "exclude_migrations_per_iter=value]\t  "
		   "define PEB mapping table testing options.\n");
	SSDFS_INFO("\t [-M|--memory-primitives iterations=value,"
		   "capacity=value,count=value,item_size=value,"
		   "test_folio_vector,test_folio_array,"
		   "test_dynamic_array,test_all]\t  "
		   "define memory primitives testing options.\n");
	SSDFS_INFO("\t [-n|--snapshots-tree snapshots_number=value]\t  "
		   "define snapshots tree testing options.\n");
	SSDFS_INFO("\t [-o|--offset-table capacity=value]\t\t  "
		   "define offsets table testing options.\n");
	SSDFS_INFO("\t [-p|--pagesize size]\t  page size of target device "
		   "(4096|8192|16384|32768 bytes).\n");
	SSDFS_INFO("\t [-s|--subsystem dentries_tree,extents_tree,"
		   "block_bitmap,offset_table,mapping_table,memory_primitives,"
		   "segment_bitmap,shared_dictionary,xattr_tree,"
		   "shared_extents_tree,snapshots_tree]\t  "
		   "define testing subsystems.\n");
	SSDFS_INFO("\t [-S|--segment-bitmap iterations=value,"
		   "using_segs_per_iter=value,"
		   "used_segs_per_iter=value,"
		   "pre_dirty_segs_per_iter=value,"
		   "dirty_segs_per_iter=value,"
		   "cleaned_segs_per_iter=value]\t  "
		   "define segment bitmap testing options.\n");
	SSDFS_INFO("\t [-V|--version]\t\t  print version and exit.\n");
	SSDFS_INFO("\t [-x|--xattr-tree xattrs_number=value,"
		   "name_len=value,step_factor=value,"
		   "blob_len=value,blob_pattern=value]\t  "
		   "define xattrs tree testing options.\n");
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

#define BLOCK_BMAP_CAPACITY(env) \
	(env->block_bitmap.capacity)
#define PRE_ALLOC_PER_ITER(env) \
	(env->block_bitmap.pre_alloc_blks_per_iteration)
#define ALLOC_PER_ITER(env) \
	(env->block_bitmap.alloc_blks_per_iteration)
#define INVALIDATE_PER_ITER(env) \
	(env->block_bitmap.invalidate_blks_per_iteration)
#define RESERVE_PER_ITER(env) \
	(env->block_bitmap.reserved_metadata_blks_per_iteration)

#define SHDICT_NAMES_NUMBER(env) \
	(env->shared_dictionary.names_number)
#define SHDICT_NAME_LEN(env) \
	(env->shared_dictionary.name_len)
#define SHDICT_STEP_FACTOR(env) \
	(env->shared_dictionary.step_factor)

#define FILE_SIZE(env) \
	(env->extents_tree.file_size_threshold)
#define EXTENT_LEN(env) \
	(env->extents_tree.extent_len_threshold)

#define FILES_NUMBER(env) \
	(env->dentries_tree.files_number_threshold)

#define PEBTBL_ITERATIONS(env) \
	(env->mapping_table.iterations_number)
#define PEBTBL_MAPPINGS(env) \
	(env->mapping_table.peb_mappings_per_iteration)
#define PEBTBL_ADD_MIGRATIONS(env) \
	(env->mapping_table.add_migrations_per_iteration)
#define PEBTBL_DEL_MIGRATIONS(env) \
	(env->mapping_table.exclude_migrations_per_iteration)

#define MEM_PRIMITIVES_ITERATIONS(env) \
	(env->memory_primitives.iterations_number)
#define MEM_PRIMITIVES_CAPACITY(env) \
	(env->memory_primitives.capacity)
#define MEM_PRIMITIVES_COUNT(env) \
	(env->memory_primitives.count)
#define MEM_PRIMITIVES_ITEM_SIZE(env) \
	(env->memory_primitives.item_size)
#define MEM_PRIMITIVES_TEST_TYPES(env) \
	(env->memory_primitives.test_types)

#define BLK2OFF_CAPACITY(env) \
	(env->blk2off_table.capacity)

#define SEGBMAP_ITERATIONS(env) \
	(env->segment_bitmap.iterations_number)
#define SEGBMAP_USING_SEGS(env) \
	(env->segment_bitmap.using_segs_per_iteration)
#define SEGBMAP_USED_SEGS(env) \
	(env->segment_bitmap.used_segs_per_iteration)
#define SEGBMAP_PRE_DIRTY_SEGS(env) \
	(env->segment_bitmap.pre_dirty_segs_per_iteration)
#define SEGBMAP_DIRTY_SEGS(env) \
	(env->segment_bitmap.dirty_segs_per_iteration)
#define SEGBMAP_CLEANED_SEGS(env) \
	(env->segment_bitmap.cleaned_segs_per_iteration)

#define XATTRS_NUMBER(env) \
	(env->xattr_tree.xattrs_number)
#define XATTR_NAME_LEN(env) \
	(env->xattr_tree.name_len)
#define XATTR_STEP_FACTOR(env) \
	(env->xattr_tree.step_factor)
#define XATTR_BLOB_LEN(env) \
	(env->xattr_tree.blob_len)
#define XATTR_BLOB_PATTERN(env) \
	(env->xattr_tree.blob_pattern)

#define SHARED_EXTENTS_NUMBER(env) \
	(env->shextree.extents_number_threshold)
#define SHARED_EXTENT_LEN(env) \
	(env->shextree.extent_len)
#define SHARED_EXTENT_REF_COUNT_MAX(env) \
	(env->shextree.ref_count_threshold)

#define SNAPSHOTS_NUMBER(env) \
	(env->snapshots_tree.snapshots_number_threshold)

void parse_options(int argc, char *argv[],
		   struct ssdfs_testing_environment *env)
{
	int c;
	int oi = 1;
	char *p;
	char sopts[] = "ab:d:D:e:f:hm:M:n:o:p:s:S:Vx:";
	static const struct option lopts[] = {
		{"all", 0, NULL, 'a'},
		{"block-bitmap", 1, NULL, 'b'},
		{"shared-dictionary", 1, NULL, 'd'},
		{"shared-extents-tree", 1, NULL, 'D'},
		{"extent", 1, NULL, 'e'},
		{"file", 1, NULL, 'f'},
		{"help", 0, NULL, 'h'},
		{"mapping-table", 1, NULL, 'm'},
		{"memory-primitives", 1, NULL, 'M'},
		{"snapshots-tree", 1, NULL, 'n'},
		{"offset-table", 1, NULL, 'o'},
		{"pagesize", 1, NULL, 'p'},
		{"subsystem", 1, NULL, 's'},
		{"segment-bitmap", 1, NULL, 'S'},
		{"version", 0, NULL, 'V'},
		{"xattr-tree", 1, NULL, 'x'},
		{ }
	};
	enum {
		BLOCK_BMAP_CAPACITY_OPT = 0,
		BLOCK_BMAP_PRE_ALLOC_OPT,
		BLOCK_BMAP_ALLOC_OPT,
		BLOCK_BMAP_INVALIDATE_OPT,
		BLOCK_BMAP_RESERVE_OPT,
	};
	char *const block_bmap_tokens[] = {
		[BLOCK_BMAP_CAPACITY_OPT]	= "capacity",
		[BLOCK_BMAP_PRE_ALLOC_OPT]	= "pre-alloc",
		[BLOCK_BMAP_ALLOC_OPT]		= "alloc",
		[BLOCK_BMAP_INVALIDATE_OPT]	= "invalidate",
		[BLOCK_BMAP_RESERVE_OPT]	= "reserve",
		NULL
	};
	enum {
		SHARED_DICT_NAMES_NUMBER_OPT = 0,
		SHARED_DICT_NAME_LEN_OPT,
		SHARED_DICT_STEP_FACTOR_OPT,
	};
	char *const shared_dict_tokens[] = {
		[SHARED_DICT_NAMES_NUMBER_OPT]	= "names_number",
		[SHARED_DICT_NAME_LEN_OPT]	= "name_len",
		[SHARED_DICT_STEP_FACTOR_OPT]	= "step_factor",
		NULL
	};
	enum {
		SHEXTREE_EXTENTS_NUMBER_OPT = 0,
		SHEXTREE_EXTENT_LEN_OPT,
		SHEXTREE_REF_COUNT_MAX_OPT,
	};
	char *const shextree_tokens[] = {
		[SHEXTREE_EXTENTS_NUMBER_OPT]	= "extents_number",
		[SHEXTREE_EXTENT_LEN_OPT]	= "extent_len",
		[SHEXTREE_REF_COUNT_MAX_OPT]	= "ref_count_max",
		NULL
	};
	enum {
		EXTENT_MAX_LEN_OPT = 0,
	};
	char *const extent_tokens[] = {
		[EXTENT_MAX_LEN_OPT]		= "max_len",
		NULL
	};
	enum {
		FILE_MAX_COUNT_OPT = 0,
		FILE_MAX_SIZE_OPT,
	};
	char *const file_tokens[] = {
		[FILE_MAX_COUNT_OPT]		= "max_count",
		[FILE_MAX_SIZE_OPT]		= "max_size",
		NULL
	};
	enum {
		MAPPING_TABLE_ITERATIONS_OPT = 0,
		MAPPING_TABLE_MAPPINGS_PER_ITER_OPT,
		MAPPING_TABLE_ADD_MIGRATIONS_PER_ITER_OPT,
		MAPPING_TABLE_DEL_MIGRATIONS_PER_ITER_OPT,
	};
	char *const mapping_table_tokens[] = {
		[MAPPING_TABLE_ITERATIONS_OPT]			=
							"iterations",
		[MAPPING_TABLE_MAPPINGS_PER_ITER_OPT]		=
							"mappings_per_iter",
		[MAPPING_TABLE_ADD_MIGRATIONS_PER_ITER_OPT]	=
						"add_migrations_per_iter",
		[MAPPING_TABLE_DEL_MIGRATIONS_PER_ITER_OPT]	=
						"exclude_migrations_per_iter",
		NULL
	};
	enum {
		MEMORY_PRIMITIVES_ITERATIONS_OPT = 0,
		MEMORY_PRIMITIVES_CAPACITY_OPT,
		MEMORY_PRIMITIVES_COUNT_OPT,
		MEMORY_PRIMITIVES_ITEM_SIZE_OPT,
		MEMORY_PRIMITIVES_TEST_FOLIO_VECTOR_OPT,
		MEMORY_PRIMITIVES_TEST_FOLIO_ARRAY_OPT,
		MEMORY_PRIMITIVES_TEST_DYNAMIC_ARRAY_OPT,
		MEMORY_PRIMITIVES_TEST_ALL_OPT,
	};
	char *const memory_primitives_tokens[] = {
		[MEMORY_PRIMITIVES_ITERATIONS_OPT]	= "iterations",
		[MEMORY_PRIMITIVES_CAPACITY_OPT]	= "capacity",
		[MEMORY_PRIMITIVES_COUNT_OPT]		= "count",
		[MEMORY_PRIMITIVES_ITEM_SIZE_OPT]	= "item_size",
		[MEMORY_PRIMITIVES_TEST_FOLIO_VECTOR_OPT] =
							  "test_folio_vector",
		[MEMORY_PRIMITIVES_TEST_FOLIO_ARRAY_OPT] =
							  "test_folio_array",
		[MEMORY_PRIMITIVES_TEST_DYNAMIC_ARRAY_OPT] =
							  "test_dynamic_array",
		[MEMORY_PRIMITIVES_TEST_ALL_OPT] 	= "test_all",
		NULL
	};
	enum {
		OFFSET_TABLE_CAPACITY_OPT = 0,
	};
	char *const offset_table_tokens[] = {
		[OFFSET_TABLE_CAPACITY_OPT]	= "capacity",
		NULL
	};
	enum {
		SEGBMAP_ITERATIONS_OPT = 0,
		SEGBMAP_USING_SEGS_PER_ITER_OPT,
		SEGBMAP_USED_SEGS_PER_ITER_OPT,
		SEGBMAP_PRE_DIRTY_SEGS_PER_ITER_OPT,
		SEGBMAP_DIRTY_SEGS_PER_ITER_OPT,
		SEGBMAP_CLEANED_SEGS_PER_ITER_OPT,
	};
	char *const segbmap_tokens[] = {
		[SEGBMAP_ITERATIONS_OPT]		=
						"iterations",
		[SEGBMAP_USING_SEGS_PER_ITER_OPT]	=
						"using_segs_per_iter",
		[SEGBMAP_USED_SEGS_PER_ITER_OPT]	=
						"used_segs_per_iter",
		[SEGBMAP_PRE_DIRTY_SEGS_PER_ITER_OPT]	=
						"pre_dirty_segs_per_iter",
		[SEGBMAP_DIRTY_SEGS_PER_ITER_OPT]	=
						"dirty_segs_per_iter",
		[SEGBMAP_CLEANED_SEGS_PER_ITER_OPT]	=
						"cleaned_segs_per_iter",
		NULL
	};
	enum {
		XATTR_TREE_XATTRS_NUMBER_OPT = 0,
		XATTR_TREE_NAME_LEN_OPT,
		XATTR_TREE_STEP_FACTOR_OPT,
		XATTR_TREE_BLOB_LEN_OPT,
		XATTR_TREE_BLOB_PATTERN_OPT,
	};
	char *const xattr_tree_tokens[] = {
		[XATTR_TREE_XATTRS_NUMBER_OPT]	= "xattrs_number",
		[XATTR_TREE_NAME_LEN_OPT]	= "name_len",
		[XATTR_TREE_STEP_FACTOR_OPT]	= "step_factor",
		[XATTR_TREE_BLOB_LEN_OPT]	= "blob_len",
		[XATTR_TREE_BLOB_PATTERN_OPT]	= "blob_pattern",
		NULL
	};
	enum {
		SNAPSHOTS_NUMBER_OPT = 0,
	};
	char *const snapshots_tree_tokens[] = {
		[SNAPSHOTS_NUMBER_OPT]		= "snapshots_number",
		NULL
	};
	enum {
		DENTRIES_TREE_SUBSYSTEM_OPT = 0,
		EXTENTS_TREE_SUBSYSTEM_OPT,
		BLOCK_BMAP_SUBSYSTEM_OPT,
		BLK2OFF_TABLE_SUBSYSTEM_OPT,
		PEB_MAPPING_TABLE_SUBSYSTEM_OPT,
		MEMORY_PRIMITIVES_SUBSYSTEM_OPT,
		SEGMENT_BITMAP_SUBSYSTEM_OPT,
		SHARED_DICTIONARY_SUBSYSTEM_OPT,
		XATTR_TREE_SUBSYSTEM_OPT,
		SHEXTREE_SUBSYSTEM_OPT,
		SNAPSHOTS_TREE_SUBSYSTEM_OPT,
	};
	char *const subsystem_tokens[] = {
		[DENTRIES_TREE_SUBSYSTEM_OPT]		= "dentries_tree",
		[EXTENTS_TREE_SUBSYSTEM_OPT]		= "extents_tree",
		[BLOCK_BMAP_SUBSYSTEM_OPT]		= "block_bitmap",
		[BLK2OFF_TABLE_SUBSYSTEM_OPT]		= "offset_table",
		[PEB_MAPPING_TABLE_SUBSYSTEM_OPT]	= "mapping_table",
		[MEMORY_PRIMITIVES_SUBSYSTEM_OPT]	= "memory_primitives",
		[SEGMENT_BITMAP_SUBSYSTEM_OPT]		= "segment_bitmap",
		[SHARED_DICTIONARY_SUBSYSTEM_OPT]	= "shared_dictionary",
		[XATTR_TREE_SUBSYSTEM_OPT]		= "xattr_tree",
		[SHEXTREE_SUBSYSTEM_OPT]		= "shared_extents_tree",
		[SNAPSHOTS_TREE_SUBSYSTEM_OPT]		= "snapshots_tree",
		NULL
	};

	while ((c = getopt_long(argc, argv, sopts, lopts, &oi)) != EOF) {
		switch (c) {
		case 'a':
			env->subsystems |= SSDFS_ENABLE_EXTENTS_TREE_TESTING;
			env->subsystems |= SSDFS_ENABLE_DENTRIES_TREE_TESTING;
			env->subsystems |= SSDFS_ENABLE_BLOCK_BMAP_TESTING;
			env->subsystems |= SSDFS_ENABLE_BLK2OFF_TABLE_TESTING;
			env->subsystems |=
					SSDFS_ENABLE_PEB_MAPPING_TABLE_TESTING;
			env->subsystems |= SSDFS_ENABLE_MEMORY_PRIMITIVES_TESTING;
			env->subsystems |= SSDFS_ENABLE_SEGMENT_BITMAP_TESTING;
			env->subsystems |=
					SSDFS_ENABLE_SHARED_DICTIONARY_TESTING;
			env->subsystems |= SSDFS_ENABLE_XATTR_TREE_TESTING;
			env->subsystems |= SSDFS_ENABLE_SHEXTREE_TESTING;
			env->subsystems |= SSDFS_ENABLE_SNAPSHOTS_TREE_TESTING;
			break;
		case 'b':
			p = optarg;
			while (*p != '\0') {
				char *value;

				switch (getsubopt(&p, block_bmap_tokens,
						  &value)) {
				case BLOCK_BMAP_CAPACITY_OPT:
					BLOCK_BMAP_CAPACITY(env) = atoi(value);
					break;
				case BLOCK_BMAP_PRE_ALLOC_OPT:
					PRE_ALLOC_PER_ITER(env) = atoi(value);
					break;
				case BLOCK_BMAP_ALLOC_OPT:
					ALLOC_PER_ITER(env) = atoi(value);
					break;
				case BLOCK_BMAP_INVALIDATE_OPT:
					INVALIDATE_PER_ITER(env) = atoi(value);
					break;
				case BLOCK_BMAP_RESERVE_OPT:
					RESERVE_PER_ITER(env) = atoi(value);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'd':
			p = optarg;
			while (*p != '\0') {
				char *value;

				switch (getsubopt(&p, shared_dict_tokens,
						  &value)) {
				case SHARED_DICT_NAMES_NUMBER_OPT:
					SHDICT_NAMES_NUMBER(env) = atoi(value);
					break;
				case SHARED_DICT_NAME_LEN_OPT:
					SHDICT_NAME_LEN(env) = atoi(value);
					break;
				case SHARED_DICT_STEP_FACTOR_OPT:
					SHDICT_STEP_FACTOR(env) = atoi(value);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'D':
			p = optarg;
			while (*p != '\0') {
				char *value;

				switch (getsubopt(&p, shextree_tokens,
						  &value)) {
				case SHEXTREE_EXTENTS_NUMBER_OPT:
					SHARED_EXTENTS_NUMBER(env) =
								atoll(value);
					break;
				case SHEXTREE_EXTENT_LEN_OPT:
					SHARED_EXTENT_LEN(env) = atoi(value);
					break;
				case SHEXTREE_REF_COUNT_MAX_OPT:
					SHARED_EXTENT_REF_COUNT_MAX(env) =
								atoi(value);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'e':
			p = optarg;
			while (*p != '\0') {
				char *value;
				int len;

				switch (getsubopt(&p, extent_tokens, &value)) {
				case EXTENT_MAX_LEN_OPT:
					len = atoi(value);

					if (len >= U16_MAX) {
						print_usage();
						exit(EXIT_FAILURE);
					}

					EXTENT_LEN(env) = (u16)len;
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'f':
			p = optarg;
			while (*p != '\0') {
				char *value;

				switch (getsubopt(&p, file_tokens, &value)) {
				case FILE_MAX_COUNT_OPT:
					FILES_NUMBER(env) = atoll(value);
					break;
				case FILE_MAX_SIZE_OPT:
					FILE_SIZE(env) = atoll(value);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'h':
			print_usage();
			exit(EXIT_SUCCESS);
		case 'm':
			p = optarg;
			while (*p != '\0') {
				char *value;

				switch (getsubopt(&p, mapping_table_tokens,
						  &value)) {
				case MAPPING_TABLE_ITERATIONS_OPT:
					PEBTBL_ITERATIONS(env) = atoi(value);
					break;
				case MAPPING_TABLE_MAPPINGS_PER_ITER_OPT:
					PEBTBL_MAPPINGS(env) = atoi(value);
					break;
				case MAPPING_TABLE_ADD_MIGRATIONS_PER_ITER_OPT:
					PEBTBL_ADD_MIGRATIONS(env) =
								atoi(value);
					break;
				case MAPPING_TABLE_DEL_MIGRATIONS_PER_ITER_OPT:
					PEBTBL_DEL_MIGRATIONS(env) =
								atoi(value);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'M':
			p = optarg;
			while (*p != '\0') {
				char *value;

				switch (getsubopt(&p, memory_primitives_tokens,
						  &value)) {
				case MEMORY_PRIMITIVES_ITERATIONS_OPT:
					MEM_PRIMITIVES_ITERATIONS(env) =
								atoi(value);
					break;
				case MEMORY_PRIMITIVES_CAPACITY_OPT:
					MEM_PRIMITIVES_CAPACITY(env) =
								atoll(value);
					break;
				case MEMORY_PRIMITIVES_COUNT_OPT:
					MEM_PRIMITIVES_COUNT(env) =
								atoll(value);
					break;
				case MEMORY_PRIMITIVES_ITEM_SIZE_OPT:
					MEM_PRIMITIVES_ITEM_SIZE(env) =
								atoi(value);
					break;
				case MEMORY_PRIMITIVES_TEST_FOLIO_VECTOR_OPT:
					MEM_PRIMITIVES_TEST_TYPES(env) |=
						SSDFS_ENABLE_FOLIO_VECTOR_TESTING;
					break;
				case MEMORY_PRIMITIVES_TEST_FOLIO_ARRAY_OPT:
					MEM_PRIMITIVES_TEST_TYPES(env) |=
						SSDFS_ENABLE_FOLIO_ARRAY_TESTING;
					break;
				case MEMORY_PRIMITIVES_TEST_DYNAMIC_ARRAY_OPT:
					MEM_PRIMITIVES_TEST_TYPES(env) |=
						SSDFS_ENABLE_DYNAMIC_ARRAY_TESTING;
					break;
				case MEMORY_PRIMITIVES_TEST_ALL_OPT:
					MEM_PRIMITIVES_TEST_TYPES(env) |=
						SSDFS_ENABLE_FOLIO_VECTOR_TESTING |
						SSDFS_ENABLE_FOLIO_ARRAY_TESTING |
						SSDFS_ENABLE_DYNAMIC_ARRAY_TESTING;
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'n':
			p = optarg;
			while (*p != '\0') {
				char *value;

				switch (getsubopt(&p, snapshots_tree_tokens,
						  &value)) {
				case SNAPSHOTS_NUMBER_OPT:
					SNAPSHOTS_NUMBER(env) = atoll(value);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'o':
			p = optarg;
			while (*p != '\0') {
				char *value;

				switch (getsubopt(&p, offset_table_tokens,
						  &value)) {
				case OFFSET_TABLE_CAPACITY_OPT:
					BLK2OFF_CAPACITY(env) = atoi(value);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
		case 'p':
			env->page_size = atoi(optarg);
			check_pagesize(env->page_size);
			break;
		case 's':
			p = optarg;
			while (*p != '\0') {
				char *value;

				switch (getsubopt(&p, subsystem_tokens,
						  &value)) {
				case DENTRIES_TREE_SUBSYSTEM_OPT:
					env->subsystems |=
					    SSDFS_ENABLE_DENTRIES_TREE_TESTING;
					break;
				case EXTENTS_TREE_SUBSYSTEM_OPT:
					env->subsystems |=
					    SSDFS_ENABLE_EXTENTS_TREE_TESTING;
					break;
				case BLOCK_BMAP_SUBSYSTEM_OPT:
					env->subsystems |=
					    SSDFS_ENABLE_BLOCK_BMAP_TESTING;
					break;
				case BLK2OFF_TABLE_SUBSYSTEM_OPT:
					env->subsystems |=
					    SSDFS_ENABLE_BLK2OFF_TABLE_TESTING;
					break;
				case MEMORY_PRIMITIVES_SUBSYSTEM_OPT:
					env->subsystems |=
					    SSDFS_ENABLE_MEMORY_PRIMITIVES_TESTING;
					break;
				case PEB_MAPPING_TABLE_SUBSYSTEM_OPT:
					env->subsystems |=
					 SSDFS_ENABLE_PEB_MAPPING_TABLE_TESTING;
					break;
				case SEGMENT_BITMAP_SUBSYSTEM_OPT:
					env->subsystems |=
					    SSDFS_ENABLE_SEGMENT_BITMAP_TESTING;
					break;
				case SHARED_DICTIONARY_SUBSYSTEM_OPT:
					env->subsystems |=
					 SSDFS_ENABLE_SHARED_DICTIONARY_TESTING;
					break;
				case XATTR_TREE_SUBSYSTEM_OPT:
					env->subsystems |=
					    SSDFS_ENABLE_XATTR_TREE_TESTING;
					break;
				case SHEXTREE_SUBSYSTEM_OPT:
					env->subsystems |=
					    SSDFS_ENABLE_SHEXTREE_TESTING;
					break;
				case SNAPSHOTS_TREE_SUBSYSTEM_OPT:
					env->subsystems |=
					    SSDFS_ENABLE_SNAPSHOTS_TREE_TESTING;
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
				char *value;

				switch (getsubopt(&p, segbmap_tokens,
						  &value)) {
				case SEGBMAP_ITERATIONS_OPT:
					SEGBMAP_ITERATIONS(env) = atoi(value);
					break;
				case SEGBMAP_USING_SEGS_PER_ITER_OPT:
					SEGBMAP_USING_SEGS(env) = atoi(value);
					break;
				case SEGBMAP_USED_SEGS_PER_ITER_OPT:
					SEGBMAP_USED_SEGS(env) = atoi(value);
					break;
				case SEGBMAP_PRE_DIRTY_SEGS_PER_ITER_OPT:
					SEGBMAP_PRE_DIRTY_SEGS(env) =
								atoi(value);
					break;
				case SEGBMAP_DIRTY_SEGS_PER_ITER_OPT:
					SEGBMAP_DIRTY_SEGS(env) = atoi(value);
					break;
				case SEGBMAP_CLEANED_SEGS_PER_ITER_OPT:
					SEGBMAP_CLEANED_SEGS(env) = atoi(value);
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
		case 'x':
			p = optarg;
			while (*p != '\0') {
				char *value;

				switch (getsubopt(&p, xattr_tree_tokens,
						  &value)) {
				case XATTR_TREE_XATTRS_NUMBER_OPT:
					XATTRS_NUMBER(env) = atoi(value);
					break;
				case XATTR_TREE_NAME_LEN_OPT:
					XATTR_NAME_LEN(env) = atoi(value);
					break;
				case XATTR_TREE_STEP_FACTOR_OPT:
					XATTR_STEP_FACTOR(env) = atoi(value);
					break;
				case XATTR_TREE_BLOB_LEN_OPT:
					XATTR_BLOB_LEN(env) = atoi(value);
					break;
				case XATTR_TREE_BLOB_PATTERN_OPT:
					XATTR_BLOB_PATTERN(env) = atoll(value);
					break;
				default:
					print_usage();
					exit(EXIT_FAILURE);
				};
			};
			break;
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
