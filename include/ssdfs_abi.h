/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * include/ssdfs_abi.h - SSDFS on-disk structures and common declarations.
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

#ifndef _SSDFS_ABI_H
#define _SSDFS_ABI_H

/* SSDFS magic signatures */
#define SSDFS_SUPER_MAGIC			0x53734466	/* SsDf */
#define SSDFS_SEGMENT_HDR_MAGIC			0x5348		/* SH */
#define SSDFS_LOG_FOOTER_MAGIC			0x4C46		/* LF */
#define SSDFS_PARTIAL_LOG_HDR_MAGIC		0x5048		/* PH */
#define SSDFS_PADDING_HDR_MAGIC			0x5044		/* PD */
#define SSDFS_BLK_BMAP_MAGIC			0x424D		/* BM */
#define SSDFS_FRAGMENT_DESC_MAGIC		0x66		/* f */
#define SSDFS_CHAIN_HDR_MAGIC			0x63		/* c */
#define SSDFS_PHYS_OFF_TABLE_MAGIC		0x504F5448	/* POTH */
#define SSDFS_BLK2OFF_TABLE_HDR_MAGIC		0x5474		/* Tt */
#define SSDFS_SEGBMAP_HDR_MAGIC			0x534D		/* SM */
#define SSDFS_INODE_MAGIC			0x6469		/* di */
#define SSDFS_PEB_TABLE_MAGIC			0x5074		/* Pt */
#define SSDFS_LEB_TABLE_MAGIC			0x4C74		/* Lt */
#define SSDFS_MAPTBL_CACHE_MAGIC		0x4D63		/* Mc */
#define SSDFS_MAPTBL_CACHE_PEB_STATE_MAGIC	0x4D635053	/* McPS */
#define SSDFS_INODES_BTREE_MAGIC		0x496E4274	/* InBt */
#define SSDFS_INODES_BNODE_MAGIC		0x494E		/* IN */
#define SSDFS_DENTRIES_BTREE_MAGIC		0x44654274	/* DeBt */
#define SSDFS_DENTRIES_BNODE_MAGIC		0x444E		/* DN */
#define SSDFS_EXTENTS_BTREE_MAGIC		0x45784274	/* ExBt */
#define SSDFS_SHARED_EXTENTS_BTREE_MAGIC	0x53454274	/* SEBt */
#define SSDFS_EXTENTS_BNODE_MAGIC		0x454E		/* EN */
#define SSDFS_XATTR_BTREE_MAGIC			0x45414274	/* EABt */
#define SSDFS_SHARED_XATTR_BTREE_MAGIC		0x53454174	/* SEAt */
#define SSDFS_XATTR_BNODE_MAGIC			0x414E		/* AN */
#define SSDFS_SHARED_DICT_BTREE_MAGIC		0x53446963	/* SDic */
#define SSDFS_DICTIONARY_BNODE_MAGIC		0x534E		/* SN */
#define SSDFS_SNAPSHOTS_BTREE_MAGIC		0x536E4274	/* SnBt */
#define SSDFS_SNAPSHOTS_BNODE_MAGIC		0x736E		/* sn */
#define SSDFS_SNAPSHOT_RULES_MAGIC		0x536E5275	/* SnRu */
#define SSDFS_SNAPSHOT_RECORD_MAGIC		0x5372		/* Sr */
#define SSDFS_PEB2TIME_RECORD_MAGIC		0x5072		/* Pr */
#define SSDFS_DIFF_BLOB_MAGIC			0x4466		/* Df */
#define SSDFS_INVEXT_BTREE_MAGIC		0x49784274	/* IxBt */
#define SSDFS_INVEXT_BNODE_MAGIC		0x4958		/* IX */

/* SSDFS padding blob */
#define SSDFS_PADDING_BLOB		0x50414444494E4730	/* PADDING0 */

/* SSDFS revision */
#define SSDFS_MAJOR_REVISION		1
#define SSDFS_MINOR_REVISION		19

/* SSDFS constants */
#define SSDFS_MAX_NAME_LEN		255
#define SSDFS_UUID_SIZE			16
#define SSDFS_VOLUME_LABEL_MAX		16
#define SSDFS_MAX_SNAP_RULE_NAME_LEN	16
#define SSDFS_MAX_SNAPSHOT_NAME_LEN	12

#define SSDFS_RESERVED_VBR_SIZE		1024 /* Volume Boot Record size*/
#define SSDFS_DEFAULT_SEG_SIZE		8388608
#define SSDFS_INITIAL_SNAPSHOT_SEG	0
#define SSDFS_START_SEGMENT		1

/*
 * File system states
 */
#define SSDFS_MOUNTED_FS		0x0000  /* Mounted FS state */
#define SSDFS_VALID_FS			0x0001  /* Unmounted cleanly */
#define SSDFS_ERROR_FS			0x0002  /* Errors detected */
#define SSDFS_RESIZE_FS			0x0004	/* Resize required */
#define SSDFS_LAST_KNOWN_FS_STATE	SSDFS_RESIZE_FS

/*
 * Behaviour when detecting errors
 */
#define SSDFS_ERRORS_CONTINUE		1	/* Continue execution */
#define SSDFS_ERRORS_RO			2	/* Remount fs read-only */
#define SSDFS_ERRORS_PANIC		3	/* Panic */
#define SSDFS_ERRORS_DEFAULT		SSDFS_ERRORS_CONTINUE
#define SSDFS_LAST_KNOWN_FS_ERROR	SSDFS_ERRORS_PANIC

/* Reserved inode id */
#define SSDFS_INVALID_EXTENTS_BTREE_INO		5
#define SSDFS_SNAPSHOTS_BTREE_INO		6
#define SSDFS_TESTING_INO			7
#define SSDFS_SHARED_DICT_BTREE_INO		8
#define SSDFS_INODES_BTREE_INO			9
#define SSDFS_SHARED_EXTENTS_BTREE_INO		10
#define SSDFS_SHARED_XATTR_BTREE_INO		11
#define SSDFS_MAPTBL_INO			12
#define SSDFS_SEG_TREE_INO			13
#define SSDFS_SEG_BMAP_INO			14
#define SSDFS_PEB_CACHE_INO			15
#define SSDFS_ROOT_INO				16

/*
 * struct ssdfs_revision - metadata structure version
 * @major: major version number
 * @minor: minor version number
 */
struct ssdfs_revision {
/* 0x0000 */
	__le8 major;
	__le8 minor;

/* 0x0002 */
}  __attribute__((packed));

/*
 * struct ssdfs_signature - metadata structure magic signature
 * @common: common magic value
 * @key: detailed magic value
 */
struct ssdfs_signature {
/* 0x0000 */
	__le32 common;
	__le16 key;
	struct ssdfs_revision version;

/* 0x0008 */
} __attribute__((packed));

/*
 * struct ssdfs_metadata_check - metadata structure checksum
 * @bytes: bytes count of CRC calculation for the structure
 * @flags: flags
 * @csum: checksum
 */
struct ssdfs_metadata_check {
/* 0x0000 */
	__le16 bytes;
#define SSDFS_CRC32			(1 << 0)
#define SSDFS_ZLIB_COMPRESSED		(1 << 1)
#define SSDFS_LZO_COMPRESSED		(1 << 2)
	__le16 flags;
	__le32 csum;

/* 0x0008 */
} __attribute__((packed));

/*
 * struct ssdfs_padding_header - padding block header
 * @magic: magic signature + revision
 * @check: metadata checksum
 * @blob: padding blob
 */
struct ssdfs_padding_header {
/* 0x0000 */
	struct ssdfs_signature magic;

/* 0x0008 */
	struct ssdfs_metadata_check check;

/* 0x0010 */
	__le64 blob;

/* 0x0020 */
} __attribute__((packed));

/*
 * struct ssdfs_raw_extent - raw (on-disk) extent
 * @seg_id: segment number
 * @logical_blk: logical block number
 * @len: count of blocks in extent
 */
struct ssdfs_raw_extent {
/* 0x0000 */
	__le64 seg_id;
	__le32 logical_blk;
	__le32 len;

/* 0x0010 */
} __attribute__((packed));

/*
 * struct ssdfs_meta_area_extent - metadata area extent
 * @start_id: starting identification number
 * @len: count of items in metadata area
 * @type: item's type
 * @flags: flags
 */
struct ssdfs_meta_area_extent {
/* 0x0000 */
	__le64 start_id;
	__le32 len;
	__le16 type;
	__le16 flags;

/* 0x0010 */
} __attribute__((packed));

/* Type of item in metadata area */
enum {
	SSDFS_EMPTY_EXTENT_TYPE,
	SSDFS_SEG_EXTENT_TYPE,
	SSDFS_PEB_EXTENT_TYPE,
	SSDFS_BLK_EXTENT_TYPE,
};

/* Type of segbmap's segments */
enum {
	SSDFS_MAIN_SEGBMAP_SEG,
	SSDFS_COPY_SEGBMAP_SEG,
	SSDFS_SEGBMAP_SEG_COPY_MAX,
};

#define SSDFS_SEGBMAP_SEGS	8

/*
 * struct ssdfs_segbmap_sb_header - superblock's segment bitmap header
 * @fragments_count: fragments count in segment bitmap
 * @fragments_per_seg: segbmap's fragments per segment
 * @fragments_per_peb: segbmap's fragments per PEB
 * @fragment_size: size of fragment in bytes
 * @bytes_count: size of segment bitmap in bytes (payload part)
 * @flags: segment bitmap's flags
 * @segs_count: count of really reserved segments in one chain
 * @segs: array of segbmap's segment numbers
 */
struct ssdfs_segbmap_sb_header {
/* 0x0000 */
	__le16 fragments_count;
	__le16 fragments_per_seg;
	__le16 fragments_per_peb;
	__le16 fragment_size;

/* 0x0008 */
	__le32 bytes_count;
	__le16 flags;
	__le16 segs_count;

/* 0x0010 */
	__le64 segs[SSDFS_SEGBMAP_SEGS][SSDFS_SEGBMAP_SEG_COPY_MAX];

/* 0x0090 */
} __attribute__((packed));

/* Segment bitmap's flags */
#define SSDFS_SEGBMAP_HAS_COPY		(1 << 0)
#define SSDFS_SEGBMAP_ERROR		(1 << 1)
#define SSDFS_SEGBMAP_MAKE_ZLIB_COMPR	(1 << 2)
#define SSDFS_SEGBMAP_MAKE_LZO_COMPR	(1 << 3)
#define SSDFS_SEGBMAP_FLAGS_MASK	(0xF)

enum {
	SSDFS_MAIN_MAPTBL_SEG,
	SSDFS_COPY_MAPTBL_SEG,
	SSDFS_MAPTBL_SEG_COPY_MAX,
};

#define SSDFS_MAPTBL_RESERVED_EXTENTS	(3)

/*
 * struct ssdfs_maptbl_sb_header - superblock's mapping table header
 * @fragments_count: count of fragments in mapping table
 * @fragment_bytes: bytes in one mapping table's fragment
 * @last_peb_recover_cno: checkpoint of last trying to recover PEBs
 * @lebs_count: count of Logical Erase Blocks (LEBs) are described by table
 * @pebs_count: count of Physical Erase Blocks (PEBs) are described by table
 * @fragments_per_seg: count of mapping table's fragments in segment
 * @fragments_per_peb: count of mapping table's fragments in PEB
 * @flags: mapping table's flags
 * @pre_erase_pebs: count of PEBs in pre-erase state
 * @lebs_per_fragment: count of LEBs are described by fragment
 * @pebs_per_fragment: count of PEBs are described by fragment
 * @pebs_per_stripe: count of PEBs are described by stripe
 * @stripes_per_fragment: count of stripes in fragment
 * @extents: metadata extents that describe mapping table location
 */
struct ssdfs_maptbl_sb_header {
/* 0x0000 */
	__le32 fragments_count;
	__le32 fragment_bytes;
	__le64 last_peb_recover_cno;

/* 0x0010 */
	__le64 lebs_count;
	__le64 pebs_count;

/* 0x0020 */
	__le16 fragments_per_seg;
	__le16 fragments_per_peb;
	__le16 flags;
	__le16 pre_erase_pebs;

/* 0x0028 */
	__le16 lebs_per_fragment;
	__le16 pebs_per_fragment;
	__le16 pebs_per_stripe;
	__le16 stripes_per_fragment;

/* 0x0030 */
#define MAPTBL_LIMIT1	(SSDFS_MAPTBL_RESERVED_EXTENTS)
#define MAPTBL_LIMIT2	(SSDFS_MAPTBL_SEG_COPY_MAX)
	struct ssdfs_meta_area_extent extents[MAPTBL_LIMIT1][MAPTBL_LIMIT2];

/* 0x0090 */
} __attribute__((packed));

/* Mapping table's flags */
#define SSDFS_MAPTBL_HAS_COPY		(1 << 0)
#define SSDFS_MAPTBL_ERROR		(1 << 1)
#define SSDFS_MAPTBL_MAKE_ZLIB_COMPR	(1 << 2)
#define SSDFS_MAPTBL_MAKE_LZO_COMPR	(1 << 3)
#define SSDFS_MAPTBL_UNDER_FLUSH	(1 << 4)
#define SSDFS_MAPTBL_FLAGS_MASK		(0x1F)

/*
 * struct ssdfs_btree_descriptor - generic btree descriptor
 * @magic: magic signature
 * @flags: btree flags
 * @type: btree type
 * @log_node_size: log2(node size in bytes)
 * @pages_per_node: physical pages per btree node
 * @node_ptr_size: size in bytes of pointer on btree node
 * @index_size: size in bytes of btree's index
 * @item_size: size in bytes of btree's item
 * @index_area_min_size: minimal size in bytes of index area in btree node
 *
 * The goal of a btree descriptor is to keep
 * the main features of a tree.
 */
struct ssdfs_btree_descriptor {
/* 0x0000 */
	__le32 magic;
#define SSDFS_BTREE_DESC_INDEX_AREA_RESIZABLE		(1 << 0)
#define SSDFS_BTREE_DESC_FLAGS_MASK			0x1
	__le16 flags;
	__le8 type;
	__le8 log_node_size;

/* 0x0008 */
	__le8 pages_per_node;
	__le8 node_ptr_size;
	__le16 index_size;
	__le16 item_size;
	__le16 index_area_min_size;

/* 0x0010 */
} __attribute__((packed));

/* Btree types */
enum {
	SSDFS_BTREE_UNKNOWN_TYPE,
	SSDFS_INODES_BTREE,
	SSDFS_DENTRIES_BTREE,
	SSDFS_EXTENTS_BTREE,
	SSDFS_SHARED_EXTENTS_BTREE,
	SSDFS_XATTR_BTREE,
	SSDFS_SHARED_XATTR_BTREE,
	SSDFS_SHARED_DICTIONARY_BTREE,
	SSDFS_SNAPSHOTS_BTREE,
	SSDFS_INVALIDATED_EXTENTS_BTREE,
	SSDFS_BTREE_TYPE_MAX
};

/*
 * struct ssdfs_dentries_btree_descriptor - dentries btree descriptor
 * @desc: btree descriptor
 */
struct ssdfs_dentries_btree_descriptor {
/* 0x0000 */
	struct ssdfs_btree_descriptor desc;

/* 0x0010 */
	__le8 reserved[0x10];

/* 0x0020 */
} __attribute__((packed));

/*
 * struct ssdfs_extents_btree_descriptor - extents btree descriptor
 * @desc: btree descriptor
 */
struct ssdfs_extents_btree_descriptor {
/* 0x0000 */
	struct ssdfs_btree_descriptor desc;

/* 0x0010 */
	__le8 reserved[0x10];

/* 0x0020 */
} __attribute__((packed));

/*
 * struct ssdfs_xattr_btree_descriptor - extended attr btree descriptor
 * @desc: btree descriptor
 */
struct ssdfs_xattr_btree_descriptor {
/* 0x0000 */
	struct ssdfs_btree_descriptor desc;

/* 0x0010 */
	__le8 reserved[0x10];

/* 0x0020 */
} __attribute__((packed));

/* Type of superblock segments */
enum {
	SSDFS_MAIN_SB_SEG,
	SSDFS_COPY_SB_SEG,
	SSDFS_SB_SEG_COPY_MAX,
};

/* Different phases of superblok segment */
enum {
	SSDFS_CUR_SB_SEG,
	SSDFS_NEXT_SB_SEG,
	SSDFS_RESERVED_SB_SEG,
	SSDFS_PREV_SB_SEG,
	SSDFS_SB_CHAIN_MAX,
};

/*
 * struct ssdfs_leb2peb_pair - LEB/PEB numbers association
 * @leb_id: LEB ID number
 * @peb_id: PEB ID number
 */
struct ssdfs_leb2peb_pair {
/* 0x0000 */
	__le64 leb_id;
	__le64 peb_id;

/* 0x0010 */
} __attribute__((packed));

/*
 * struct ssdfs_btree_index - btree index
 * @hash: hash value
 * @extent: btree node's extent
 *
 * The goal of btree index is to provide the way to search
 * a proper btree node by means of hash value. The hash
 * value could be inode_id, string hash and so on.
 */
struct ssdfs_btree_index {
/* 0x0000 */
	__le64 hash;

/* 0x0008 */
	struct ssdfs_raw_extent extent;

/* 0x0018 */
} __attribute__((packed));

#define SSDFS_BTREE_NODE_INVALID_ID	(U32_MAX)

/*
 * struct ssdfs_btree_index_key - node identification key
 * @node_id: node identification key
 * @node_type: type of the node
 * @height: node's height
 * @flags: index flags
 * @index: node's index
 */
struct ssdfs_btree_index_key {
/* 0x0000 */
	__le32 node_id;
	__le8 node_type;
	__le8 height;
#define SSDFS_BTREE_INDEX_HAS_VALID_EXTENT		(1 << 0)
#define SSDFS_BTREE_INDEX_SHOW_EMPTY_NODE		(1 << 1)
#define SSDFS_BTREE_INDEX_SHOW_FREE_ITEMS		(1 << 2)
#define SSDFS_BTREE_INDEX_HAS_CHILD_WITH_FREE_ITEMS	(1 << 3)
#define SSDFS_BTREE_INDEX_SHOW_PREALLOCATED_CHILD	(1 << 4)
#define SSDFS_BTREE_INDEX_FLAGS_MASK			0x1F
	__le16 flags;

/* 0x0008 */
	struct ssdfs_btree_index index;

/* 0x0020 */
} __attribute__((packed));

#define SSDFS_BTREE_ROOT_NODE_INDEX_COUNT	(2)

/*
 * struct ssdfs_btree_root_node_header - root node header
 * @height: btree height
 * @items_count: count of items in the root node
 * @flags: root node flags
 * @type: root node type
 * @upper_node_id: last allocated the node identification number
 * @node_ids: root node's children IDs
 */
struct ssdfs_btree_root_node_header {
/* 0x0000 */
#define SSDFS_BTREE_LEAF_NODE_HEIGHT	(0)
	__le8 height;
	__le8 items_count;
	__le8 flags;
	__le8 type;

/* 0x0004 */
#define SSDFS_BTREE_ROOT_NODE_ID		(0)
	__le32 upper_node_id;

/* 0x0008 */
	__le32 node_ids[SSDFS_BTREE_ROOT_NODE_INDEX_COUNT];

/* 0x0010 */
} __attribute__((packed));

/*
 * struct ssdfs_btree_inline_root_node - btree root node
 * @header: node header
 * @indexes: root node's index array
 *
 * The goal of root node is to live inside of 0x40 bytes
 * space and to keep the root index node of the tree.
 * The inline root node could be the part of inode
 * structure or the part of btree root. The inode has
 * 0x80 bytes space. But inode needs to store as
 * extent/dentries tree as extended attributes tree.
 * So, 0x80 bytes is used for storing two btrees.
 *
 * The root node's indexes has pre-defined type.
 * If height of the tree equals to 1 - 3 range then
 * root node's indexes define hybrid nodes. Otherwise,
 * if tree's height is greater than 3 then root node's
 * indexes define pure index nodes.
 */
struct ssdfs_btree_inline_root_node {
/* 0x0000 */
	struct ssdfs_btree_root_node_header header;

/* 0x0010 */
#define SSDFS_ROOT_NODE_LEFT_LEAF_NODE		(0)
#define SSDFS_ROOT_NODE_RIGHT_LEAF_NODE		(1)
#define SSDFS_BTREE_ROOT_NODE_INDEX_COUNT	(2)
	struct ssdfs_btree_index indexes[SSDFS_BTREE_ROOT_NODE_INDEX_COUNT];

/* 0x0040 */
} __attribute__((packed));

/*
 * struct ssdfs_inodes_btree - inodes btree
 * @desc: btree descriptor
 * @allocated_inodes: count of allocated inodes
 * @free_inodes: count of free inodes
 * @inodes_capacity: count of inodes in the whole btree
 * @leaf_nodes: count of leaf btree nodes
 * @nodes_count: count of nodes in the whole btree
 * @upper_allocated_ino: maximal allocated inode ID number
 * @root_node: btree's root node
 *
 * The goal of a btree root is to keep
 * the main features of a tree and knowledge
 * about two root indexes. These indexes splits
 * the whole btree on two branches.
 */
struct ssdfs_inodes_btree {
/* 0x0000 */
	struct ssdfs_btree_descriptor desc;

/* 0x0010 */
	__le64 allocated_inodes;
	__le64 free_inodes;

/* 0x0020 */
	__le64 inodes_capacity;
	__le32 leaf_nodes;
	__le32 nodes_count;

/* 0x0030 */
	__le64 upper_allocated_ino;
	__le8 reserved[0x8];

/* 0x0040 */
	struct ssdfs_btree_inline_root_node root_node;

/* 0x0080 */
} __attribute__((packed));

/*
 * struct ssdfs_shared_extents_btree - shared extents btree
 * @desc: btree descriptor
 * @root_node: btree's root node
 *
 * The goal of a btree root is to keep
 * the main features of a tree and knowledge
 * about two root indexes. These indexes splits
 * the whole btree on two branches.
 */
struct ssdfs_shared_extents_btree {
/* 0x0000 */
	struct ssdfs_btree_descriptor desc;

/* 0x0010 */
	__le8 reserved[0x30];

/* 0x0040 */
	struct ssdfs_btree_inline_root_node root_node;

/* 0x0080 */
} __attribute__((packed));

/*
 * ssdfs_shared_dictionary_btree - shared strings dictionary btree
 * @desc: btree descriptor
 * @root_node: btree's root node
 *
 * The goal of a btree root is to keep
 * the main features of a tree and knowledge
 * about two root indexes. These indexes splits
 * the whole btree on two branches.
 */
struct ssdfs_shared_dictionary_btree {
/* 0x0000 */
	struct ssdfs_btree_descriptor desc;

/* 0x0010 */
	__le8 reserved[0x30];

/* 0x0040 */
	struct ssdfs_btree_inline_root_node root_node;

/* 0x0080 */
} __attribute__((packed));

/*
 * struct ssdfs_shared_xattr_btree - shared extended attributes btree
 * @desc: btree descriptor
 * @root_node: btree's root node
 *
 * The goal of a btree root is to keep
 * the main features of a tree and knowledge
 * about two root indexes. These indexes splits
 * the whole btree on two branches.
 */
struct ssdfs_shared_xattr_btree {
/* 0x0000 */
	struct ssdfs_btree_descriptor desc;

/* 0x0010 */
	__le8 reserved[0x30];

/* 0x0040 */
	struct ssdfs_btree_inline_root_node root_node;

/* 0x0080 */
} __attribute__((packed));

/*
 * struct ssdfs_snapshots_btree - snapshots btree
 * @desc: btree descriptor
 * @root_node: btree's root node
 *
 * The goal of a btree root is to keep
 * the main features of a tree and knowledge
 * about two root indexes. These indexes splits
 * the whole btree on two branches.
 */
struct ssdfs_snapshots_btree {
/* 0x0000 */
	struct ssdfs_btree_descriptor desc;

/* 0x0010 */
	__le8 reserved[0x30];

/* 0x0040 */
	struct ssdfs_btree_inline_root_node root_node;

/* 0x0080 */
} __attribute__((packed));

/*
 * struct ssdfs_invalidated_extents_btree - invalidated extents btree
 * @desc: btree descriptor
 * @root_node: btree's root node
 *
 * The goal of a btree root is to keep
 * the main features of a tree and knowledge
 * about two root indexes. These indexes splits
 * the whole btree on two branches.
 */
struct ssdfs_invalidated_extents_btree {
/* 0x0000 */
	struct ssdfs_btree_descriptor desc;

/* 0x0010 */
	__le8 reserved[0x30];

/* 0x0040 */
	struct ssdfs_btree_inline_root_node root_node;

/* 0x0080 */
}  __attribute__((packed));

enum {
	SSDFS_CUR_DATA_SEG,
	SSDFS_CUR_LNODE_SEG,
	SSDFS_CUR_HNODE_SEG,
	SSDFS_CUR_IDXNODE_SEG,
	SSDFS_CUR_DATA_UPDATE_SEG,	/* ZNS SSD case */
	SSDFS_CUR_SEGS_COUNT,
};

/*
 * struct ssdfs_blk_bmap_options - block bitmap options
 * @flags: block bitmap's flags
 * @compression: compression type
 */
struct ssdfs_blk_bmap_options {
/* 0x0000 */
#define SSDFS_BLK_BMAP_CREATE_COPY		(1 << 0)
#define SSDFS_BLK_BMAP_MAKE_COMPRESSION		(1 << 1)
#define SSDFS_BLK_BMAP_OPTIONS_MASK		(0x3)
	__le16 flags;
#define SSDFS_BLK_BMAP_NOCOMPR_TYPE		(0)
#define SSDFS_BLK_BMAP_ZLIB_COMPR_TYPE		(1)
#define SSDFS_BLK_BMAP_LZO_COMPR_TYPE		(2)
	__le8 compression;
	__le8 reserved;

/* 0x0004 */
} __attribute__((packed));

/*
 * struct ssdfs_blk2off_tbl_options - offset translation table options
 * @flags: offset translation table's flags
 * @compression: compression type
 */
struct ssdfs_blk2off_tbl_options {
/* 0x0000 */
#define SSDFS_BLK2OFF_TBL_CREATE_COPY		(1 << 0)
#define SSDFS_BLK2OFF_TBL_MAKE_COMPRESSION	(1 << 1)
#define SSDFS_BLK2OFF_TBL_OPTIONS_MASK		(0x3)
	__le16 flags;
#define SSDFS_BLK2OFF_TBL_NOCOMPR_TYPE		(0)
#define SSDFS_BLK2OFF_TBL_ZLIB_COMPR_TYPE	(1)
#define SSDFS_BLK2OFF_TBL_LZO_COMPR_TYPE	(2)
	__le8 compression;
	__le8 reserved;

/* 0x0004 */
} __attribute__((packed));

/*
 * struct ssdfs_user_data_options - user data options
 * @flags: user data's flags
 * @compression: compression type
 * @migration_threshold: default value of destination PEBs in migration
 */
struct ssdfs_user_data_options {
/* 0x0000 */
#define SSDFS_USER_DATA_MAKE_COMPRESSION	(1 << 0)
#define SSDFS_USER_DATA_OPTIONS_MASK		(0x1)
	__le16 flags;
#define SSDFS_USER_DATA_NOCOMPR_TYPE		(0)
#define SSDFS_USER_DATA_ZLIB_COMPR_TYPE		(1)
#define SSDFS_USER_DATA_LZO_COMPR_TYPE		(2)
	__le8 compression;
	__le8 reserved1;
	__le16 migration_threshold;
	__le16 reserved2;

/* 0x0008 */
} __attribute__((packed));

#define SSDFS_INODE_HASNT_INLINE_FORKS		(0)
#define SSDFS_INLINE_FORKS_COUNT		(2)
#define SSDFS_INLINE_EXTENTS_COUNT		(3)

/*
 * struct ssdfs_raw_fork - contiguous sequence of raw (on-disk) extents
 * @start_offset: start logical offset in pages (blocks) from file's beginning
 * @blks_count: count of logical blocks in the fork (no holes)
 * @extents: sequence of raw (on-disk) extents
 */
struct ssdfs_raw_fork {
/* 0x0000 */
	__le64 start_offset;
	__le64 blks_count;

/* 0x0010 */
	struct ssdfs_raw_extent extents[SSDFS_INLINE_EXTENTS_COUNT];

/* 0x0040 */
} __attribute__((packed));

/*
 * struct ssdfs_name_hash - hash of the name
 * @raw: raw value of the hash64
 *
 * The name's hash is 64 bits wide (8 bytes). But the hash64 has
 * special structure. The first 4 bytes are the low hash (hash32_lo)
 * of the name. The second 4 bytes is the high hash (hash32_hi)
 * of the name. If the name lesser or equal to 12 symbols (inline
 * name's string) then hash32_hi will be equal to zero always.
 * If the name is greater than 12 symbols then the hash32_hi
 * will be the hash of the rest of the name (excluding the
 * first 12 symbols). The hash32_lo will be defined by inline
 * name's length. The inline names (12 symbols long) will be
 * stored into dentries only. The regular names will be stored
 * partially in the dentry (12 symbols) and the whole name string
 * will be stored into shared dictionary.
 */
struct ssdfs_name_hash {
/* 0x0000 */
	__le64 raw;

/* 0x0008 */
} __attribute__((packed));

/* Name hash related macros */
#define SSDFS_NAME_HASH(hash32_lo, hash32_hi)({ \
	u64 hash64 = (u32)hash32_lo; \
	hash64 <<= 32; \
	hash64 |= hash32_hi; \
	hash64; \
})
#define SSDFS_NAME_HASH_LE64(hash32_lo, hash32_hi) \
	(cpu_to_le64(SSDFS_NAME_HASH(hash32_lo, hash32_hi)))
#define LE64_TO_SSDFS_HASH32_LO(hash_le64) \
	((u32)(le64_to_cpu(hash_le64) >> 32))
#define SSDFS_HASH32_LO(hash64) \
	((u32)(hash64 >> 32))
#define LE64_TO_SSDFS_HASH32_HI(hash_le64) \
	((u32)(le64_to_cpu(hash_le64) & 0xFFFFFFFF))
#define SSDFS_HASH32_HI(hash64) \
	((u32)(hash64 & 0xFFFFFFFF))

/*
 * struct ssdfs_dir_entry - directory entry
 * @ino: inode number
 * @hash_code: name string's hash code
 * @name_len: name length in bytes
 * @dentry_type: dentry type
 * @file_type: directory file types
 * @flags: dentry's flags
 * @inline_string: inline copy of the name or exclusive storage of short name
 */
struct ssdfs_dir_entry {
/* 0x0000 */
	__le64 ino;
	__le64 hash_code;

/* 0x0010 */
	__le8 name_len;
	__le8 dentry_type;
	__le8 file_type;
	__le8 flags;
#define SSDFS_DENTRY_INLINE_NAME_MAX_LEN	(12)
	__le8 inline_string[SSDFS_DENTRY_INLINE_NAME_MAX_LEN];

/* 0x0020 */
} __attribute__((packed));

/* Dentry types */
enum {
	SSDFS_DENTRY_UNKNOWN_TYPE,
	SSDFS_INLINE_DENTRY,
	SSDFS_REGULAR_DENTRY,
	SSDFS_DENTRY_TYPE_MAX
};

/*
 * SSDFS directory file types.
 */
enum {
	SSDFS_FT_UNKNOWN,
	SSDFS_FT_REG_FILE,
	SSDFS_FT_DIR,
	SSDFS_FT_CHRDEV,
	SSDFS_FT_BLKDEV,
	SSDFS_FT_FIFO,
	SSDFS_FT_SOCK,
	SSDFS_FT_SYMLINK,
	SSDFS_FT_MAX
};

/* Dentry flags */
#define SSDFS_DENTRY_HAS_EXTERNAL_STRING	(1 << 0)
#define SSDFS_DENTRY_FLAGS_MASK			0x1

/*
 * struct ssdfs_blob_extent - blob's extent descriptor
 * @hash: blob's hash
 * @extent: blob's extent
 */
struct ssdfs_blob_extent {
/* 0x0000 */
	__le64 hash;
	__le64 reserved;
	struct ssdfs_raw_extent extent;

/* 0x0020 */
} __attribute__((packed));

#define SSDFS_XATTR_INLINE_BLOB_MAX_LEN		(32)
#define SSDFS_XATTR_EXTERNAL_BLOB_MAX_LEN	(32768)

/*
 * struct ssdfs_blob_bytes - inline blob's byte stream
 * @bytes: byte stream
 */
struct ssdfs_blob_bytes {
/* 0x0000 */
	__le8 bytes[SSDFS_XATTR_INLINE_BLOB_MAX_LEN];

/* 0x0020 */
} __attribute__((packed));

/*
 * struct ssdfs_xattr_entry - extended attribute entry
 * @name_hash: hash of the name
 * @inline_index: index of the inline xattr
 * @name_len: length of the name
 * @name_type: type of the name
 * @name_flags: flags of the name
 * @blob_len: blob length in bytes
 * @blob_type: type of the blob
 * @blob_flags: flags of the blob
 * @inline_string: inline string of the name
 * @blob.descriptor.hash: hash of the blob
 * @blob.descriptor.extent: extent of the blob
 * @blob.inline_value: inline value of the blob
 *
 * The extended attribute can be described by fixed size
 * descriptor. The name of extended attribute can be inline
 * or to be stored into the shared dictionary. If the name
 * is greater than 16 symbols then it will be stored into shared
 * dictionary. The blob part can be stored inline or,
 * otherwise, the descriptor contains the hash of the blob
 * and blob will be stored as ordinary file inside
 * of logical blocks.
 */
struct ssdfs_xattr_entry {
/* 0x0000 */
	__le64 name_hash;

/* 0x0008 */
	__le8 inline_index;
	__le8 name_len;
	__le8 name_type;
	__le8 name_flags;

/* 0x000C */
	__le16 blob_len;
	__le8 blob_type;
	__le8 blob_flags;

/* 0x0010 */
#define SSDFS_XATTR_INLINE_NAME_MAX_LEN	(16)
	__le8 inline_string[SSDFS_XATTR_INLINE_NAME_MAX_LEN];

/* 0x0020 */
	union {
		struct ssdfs_blob_extent descriptor;
		struct ssdfs_blob_bytes inline_value;
	} blob;

/* 0x0040 */
} __attribute__((packed));

/* registered names' prefixes */
enum {
	SSDFS_USER_NS_INDEX,
	SSDFS_TRUSTED_NS_INDEX,
	SSDFS_SYSTEM_NS_INDEX,
	SSDFS_SECURITY_NS_INDEX,
	SSDFS_REGISTERED_NS_NUMBER
};

static const char * const SSDFS_NS_PREFIX[] = {
	"user.",
	"trusted.",
	"system.",
	"security.",
};

/* xattr name types */
enum {
	SSDFS_XATTR_NAME_UNKNOWN_TYPE,
	SSDFS_XATTR_INLINE_NAME,
	SSDFS_XATTR_USER_INLINE_NAME,
	SSDFS_XATTR_TRUSTED_INLINE_NAME,
	SSDFS_XATTR_SYSTEM_INLINE_NAME,
	SSDFS_XATTR_SECURITY_INLINE_NAME,
	SSDFS_XATTR_REGULAR_NAME,
	SSDFS_XATTR_USER_REGULAR_NAME,
	SSDFS_XATTR_TRUSTED_REGULAR_NAME,
	SSDFS_XATTR_SYSTEM_REGULAR_NAME,
	SSDFS_XATTR_SECURITY_REGULAR_NAME,
	SSDFS_XATTR_NAME_TYPE_MAX
};

/* xattr name flags */
#define SSDFS_XATTR_HAS_EXTERNAL_STRING		(1 << 0)
#define SSDFS_XATTR_NAME_FLAGS_MASK		0x1

/* xattr blob types */
enum {
	SSDFS_XATTR_BLOB_UNKNOWN_TYPE,
	SSDFS_XATTR_INLINE_BLOB,
	SSDFS_XATTR_REGULAR_BLOB,
	SSDFS_XATTR_BLOB_TYPE_MAX
};

/* xattr blob flags */
#define SSDFS_XATTR_HAS_EXTERNAL_BLOB		(1 << 0)
#define SSDFS_XATTR_BLOB_FLAGS_MASK		0x1

#define SSDFS_INLINE_DENTRIES_PER_AREA		(2)
#define SSDFS_INLINE_STREAM_SIZE_PER_AREA	(64)
#define SSDFS_DEFAULT_INLINE_XATTR_COUNT	(1)

/*
 * struct ssdfs_inode_inline_stream - inode's inline stream
 * @bytes: bytes array
 */
struct ssdfs_inode_inline_stream {
/* 0x0000 */
	__le8 bytes[SSDFS_INLINE_STREAM_SIZE_PER_AREA];

/* 0x0040 */
} __attribute__((packed));

/*
 * struct ssdfs_inode_inline_dentries - inline dentries array
 * @array: dentries array
 */
struct ssdfs_inode_inline_dentries {
/* 0x0000 */
	struct ssdfs_dir_entry array[SSDFS_INLINE_DENTRIES_PER_AREA];

/* 0x0040 */
} __attribute__((packed));

/*
 * struct ssdfs_inode_private_area - inode's private area
 * @area1.inline_stream: inline file's content
 * @area1.extents_root: extents btree root node
 * @area1.fork: inline fork
 * @area1.dentries_root: dentries btree root node
 * @area1.dentries: inline dentries
 * @area2.inline_stream: inline file's content
 * @area2.inline_xattr: inline extended attribute
 * @area2.xattr_root: extended attributes btree root node
 * @area2.fork: inline fork
 * @area2.dentries: inline dentries
 */
struct ssdfs_inode_private_area {
/* 0x0000 */
	union {
		struct ssdfs_inode_inline_stream inline_stream;
		struct ssdfs_btree_inline_root_node extents_root;
		struct ssdfs_raw_fork fork;
		struct ssdfs_btree_inline_root_node dentries_root;
		struct ssdfs_inode_inline_dentries dentries;
	} area1;

/* 0x0040 */
	union {
		struct ssdfs_inode_inline_stream inline_stream;
		struct ssdfs_xattr_entry inline_xattr;
		struct ssdfs_btree_inline_root_node xattr_root;
		struct ssdfs_raw_fork fork;
		struct ssdfs_inode_inline_dentries dentries;
	} area2;

/* 0x0080 */
} __attribute__((packed));

/*
 * struct ssdfs_inode - raw (on-disk) inode
 * @magic: inode magic
 * @mode: file mode
 * @flags: file attributes
 * @uid: owner user ID
 * @gid: owner group ID
 * @atime: access time (seconds)
 * @ctime: change time (seconds)
 * @mtime: modification time (seconds)
 * @birthtime: inode creation time (seconds)
 * @atime_nsec: access time in nano scale
 * @ctime_nsec: change time in nano scale
 * @mtime_nsec: modification time in nano scale
 * @birthtime_nsec: creation time in nano scale
 * @generation: file version (for NFS)
 * @size: file size in bytes
 * @blocks: file size in blocks
 * @parent_ino: parent inode number
 * @refcount: links count
 * @checksum: inode checksum
 * @ino: inode number
 * @hash_code: hash code of file name
 * @name_len: lengh of file name
 * @forks_count: count of forks
 * @internal: array of inline private areas of inode
 */
struct ssdfs_inode {
/* 0x0000 */
	__le16 magic;			/* Inode magic */
	__le16 mode;			/* File mode */
	__le32 flags;			/* file attributes */

/* 0x0008 */
	__le32 uid;			/* user ID */
	__le32 gid;			/* group ID */

/* 0x0010 */
	__le64 atime;			/* access time */
	__le64 ctime;			/* change time */
	__le64 mtime;			/* modification time */
	__le64 birthtime;		/* inode creation time */

/* 0x0030 */
	__le32 atime_nsec;		/* access time in nano scale */
	__le32 ctime_nsec;		/* change time in nano scale */
	__le32 mtime_nsec;		/* modification time in nano scale */
	__le32 birthtime_nsec;		/* creation time in nano scale */

/* 0x0040 */
	__le64 generation;		/* file version (for NFS) */
	__le64 size;			/* file size in bytes */
	__le64 blocks;			/* file size in blocks */
	__le64 parent_ino;		/* parent inode number */

/* 0x0060 */
	__le32 refcount;		/* links count */
	__le32 checksum;		/* inode checksum */

/* 0x0068 */
	__le64 ino;			/* Inode number */
	__le64 hash_code;		/* hash code of file name */
	__le16 name_len;		/* lengh of file name */
#define SSDFS_INODE_HAS_INLINE_EXTENTS		(1 << 0)
#define SSDFS_INODE_HAS_EXTENTS_BTREE		(1 << 1)
#define SSDFS_INODE_HAS_INLINE_DENTRIES		(1 << 2)
#define SSDFS_INODE_HAS_DENTRIES_BTREE		(1 << 3)
#define SSDFS_INODE_HAS_INLINE_XATTR		(1 << 4)
#define SSDFS_INODE_HAS_XATTR_BTREE		(1 << 5)
#define SSDFS_INODE_HAS_INLINE_FILE		(1 << 6)
#define SSDFS_INODE_PRIVATE_FLAGS_MASK		0x7F
	__le16 private_flags;

	union {
		__le32 forks;
		__le32 dentries;
	} count_of __attribute__((packed));

/* 0x0080 */
	struct ssdfs_inode_private_area internal[1];

/* 0x0100 */
} __attribute__((packed));

#define SSDFS_IFREG_PRIVATE_FLAG_MASK \
	(SSDFS_INODE_HAS_INLINE_EXTENTS | \
	 SSDFS_INODE_HAS_EXTENTS_BTREE | \
	 SSDFS_INODE_HAS_INLINE_XATTR | \
	 SSDFS_INODE_HAS_XATTR_BTREE | \
	 SSDFS_INODE_HAS_INLINE_FILE)

#define SSDFS_IFDIR_PRIVATE_FLAG_MASK \
	(SSDFS_INODE_HAS_INLINE_DENTRIES | \
	 SSDFS_INODE_HAS_DENTRIES_BTREE | \
	 SSDFS_INODE_HAS_INLINE_XATTR | \
	 SSDFS_INODE_HAS_XATTR_BTREE)

/*
 * struct ssdfs_volume_header - static part of superblock
 * @magic: magic signature + revision
 * @check: metadata checksum
 * @log_pagesize: log2(page size)
 * @log_erasesize: log2(erase block size)
 * @log_segsize: log2(segment size)
 * @log_pebs_per_seg: log2(erase blocks per segment)
 * @megabytes_per_peb: MBs in one PEB
 * @pebs_per_seg: number of PEBs per segment
 * @create_time: volume create timestamp (mkfs phase)
 * @create_cno: volume create checkpoint
 * @flags: volume creation flags
 * @lebs_per_peb_index: difference of LEB IDs between PEB indexes in segment
 * @sb_pebs: array of prev, cur and next superblock's PEB numbers
 * @segbmap: superblock's segment bitmap header
 * @maptbl: superblock's mapping table header
 * @sb_seg_log_pages: full log size in sb segment (pages count)
 * @segbmap_log_pages: full log size in segbmap segment (pages count)
 * @maptbl_log_pages: full log size in maptbl segment (pages count)
 * @lnodes_seg_log_pages: full log size in leaf nodes segment (pages count)
 * @hnodes_seg_log_pages: full log size in hybrid nodes segment (pages count)
 * @inodes_seg_log_pages: full log size in index nodes segment (pages count)
 * @user_data_log_pages: full log size in user data segment (pages count)
 * @create_threads_per_seg: number of creation threads per segment
 * @dentries_btree: descriptor of all dentries btrees
 * @extents_btree: descriptor of all extents btrees
 * @xattr_btree: descriptor of all extended attributes btrees
 * @invextree: b-tree of invalidated extents (ZNS SSD)
 * @uuid: 128-bit uuid for volume
 */
struct ssdfs_volume_header {
/* 0x0000 */
	struct ssdfs_signature magic;

/* 0x0008 */
	struct ssdfs_metadata_check check;

/* 0x0010 */
	__le8 log_pagesize;
	__le8 log_erasesize;
	__le8 log_segsize;
	__le8 log_pebs_per_seg;
	__le16 megabytes_per_peb;
	__le16 pebs_per_seg;

/* 0x0018 */
	__le64 create_time;
	__le64 create_cno;
#define SSDFS_VH_ZNS_BASED_VOLUME	(1 << 0)
#define SSDFS_VH_UNALIGNED_ZONE		(1 << 1)
#define SSDFS_VH_FLAGS_MASK		(0x3)
	__le32 flags;
	__le32 lebs_per_peb_index;

/* 0x0030 */
#define VH_LIMIT1	SSDFS_SB_CHAIN_MAX
#define VH_LIMIT2	SSDFS_SB_SEG_COPY_MAX
	struct ssdfs_leb2peb_pair sb_pebs[VH_LIMIT1][VH_LIMIT2];

/* 0x00B0 */
	struct ssdfs_segbmap_sb_header segbmap;

/* 0x0140 */
	struct ssdfs_maptbl_sb_header maptbl;

/* 0x01D0 */
	__le16 sb_seg_log_pages;
	__le16 segbmap_log_pages;
	__le16 maptbl_log_pages;
	__le16 lnodes_seg_log_pages;
	__le16 hnodes_seg_log_pages;
	__le16 inodes_seg_log_pages;
	__le16 user_data_log_pages;
	__le16 create_threads_per_seg;

/* 0x01E0 */
	struct ssdfs_dentries_btree_descriptor dentries_btree;

/* 0x0200 */
	struct ssdfs_extents_btree_descriptor extents_btree;

/* 0x0220 */
	struct ssdfs_xattr_btree_descriptor xattr_btree;

/* 0x0240 */
	struct ssdfs_invalidated_extents_btree invextree;

/* 0x02C0 */
	__le8 uuid[SSDFS_UUID_SIZE];

/* 0x02D0 */
	__le8 reserved4[0x130];

/* 0x0400 */
} __attribute__((packed));

#define SSDFS_LEBS_PER_PEB_INDEX_DEFAULT	(1)

/*
 * struct ssdfs_volume_state - changeable part of superblock
 * @magic: magic signature + revision
 * @check: metadata checksum
 * @nsegs: segments count
 * @free_pages: free pages count
 * @timestamp: write timestamp
 * @cno: write checkpoint
 * @flags: volume flags
 * @state: file system state
 * @errors: behaviour when detecting errors
 * @feature_compat: compatible feature set
 * @feature_compat_ro: read-only compatible feature set
 * @feature_incompat: incompatible feature set
 * @uuid: 128-bit uuid for volume
 * @label: volume name
 * @cur_segs: array of current segment numbers
 * @migration_threshold: default value of destination PEBs in migration
 * @blkbmap: block bitmap options
 * @blk2off_tbl: offset translation table options
 * @user_data: user data options
 * @open_zones: number of open/active zones
 * @root_folder: copy of root folder's inode
 * @inodes_btree: inodes btree root
 * @shared_extents_btree: shared extents btree root
 * @shared_dict_btree: shared dictionary btree root
 * @snapshots_btree: snapshots btree root
 */
struct ssdfs_volume_state {
/* 0x0000 */
	struct ssdfs_signature magic;

/* 0x0008 */
	struct ssdfs_metadata_check check;

/* 0x0010 */
	__le64 nsegs;
	__le64 free_pages;

/* 0x0020 */
	__le64 timestamp;
	__le64 cno;

/* 0x0030 */
#define SSDFS_HAS_INLINE_INODES_TREE		(1 << 0)
#define SSDFS_VOLUME_STATE_FLAGS_MASK		0x1
	__le32 flags;
	__le16 state;
	__le16 errors;

/* 0x0038 */
	__le64 feature_compat;
	__le64 feature_compat_ro;
	__le64 feature_incompat;

/* 0x0050 */
	__le8 uuid[SSDFS_UUID_SIZE];
	char label[SSDFS_VOLUME_LABEL_MAX];

/* 0x0070 */
	__le64 cur_segs[SSDFS_CUR_SEGS_COUNT];

/* 0x0098 */
	__le16 migration_threshold;
	__le16 reserved1;

/* 0x009C */
	struct ssdfs_blk_bmap_options blkbmap;
	struct ssdfs_blk2off_tbl_options blk2off_tbl;

/* 0x00A4 */
	struct ssdfs_user_data_options user_data;

/* 0x00AC */
	__le32 open_zones;

/* 0x00B0 */
	struct ssdfs_inode root_folder;

/* 0x01B0 */
	__le8 reserved3[0x50];

/* 0x0200 */
	struct ssdfs_inodes_btree inodes_btree;

/* 0x0280 */
	struct ssdfs_shared_extents_btree shared_extents_btree;

/* 0x0300 */
	struct ssdfs_shared_dictionary_btree shared_dict_btree;

/* 0x0380 */
	struct ssdfs_snapshots_btree snapshots_btree;

/* 0x0400 */
} __attribute__((packed));

/* Compatible feature flags */
#define SSDFS_HAS_SEGBMAP_COMPAT_FLAG			(1 << 0)
#define SSDFS_HAS_MAPTBL_COMPAT_FLAG			(1 << 1)
#define SSDFS_HAS_SHARED_EXTENTS_COMPAT_FLAG		(1 << 2)
#define SSDFS_HAS_SHARED_XATTRS_COMPAT_FLAG		(1 << 3)
#define SSDFS_HAS_SHARED_DICT_COMPAT_FLAG		(1 << 4)
#define SSDFS_HAS_INODES_TREE_COMPAT_FLAG		(1 << 5)
#define SSDFS_HAS_SNAPSHOTS_TREE_COMPAT_FLAG		(1 << 6)
#define SSDFS_HAS_INVALID_EXTENTS_TREE_COMPAT_FLAG	(1 << 7)

/* Read-Only compatible feature flags */
#define SSDFS_ZLIB_COMPAT_RO_FLAG	(1 << 0)
#define SSDFS_LZO_COMPAT_RO_FLAG	(1 << 1)

#define SSDFS_FEATURE_COMPAT_SUPP \
	(SSDFS_HAS_SEGBMAP_COMPAT_FLAG | SSDFS_HAS_MAPTBL_COMPAT_FLAG | \
	 SSDFS_HAS_SHARED_EXTENTS_COMPAT_FLAG | \
	 SSDFS_HAS_SHARED_XATTRS_COMPAT_FLAG | \
	 SSDFS_HAS_SHARED_DICT_COMPAT_FLAG | \
	 SSDFS_HAS_INODES_TREE_COMPAT_FLAG | \
	 SSDFS_HAS_SNAPSHOTS_TREE_COMPAT_FLAG | \
	 SSDFS_HAS_INVALID_EXTENTS_TREE_COMPAT_FLAG)

#define SSDFS_FEATURE_COMPAT_RO_SUPP \
	(SSDFS_ZLIB_COMPAT_RO_FLAG | SSDFS_LZO_COMPAT_RO_FLAG)

#define SSDFS_FEATURE_INCOMPAT_SUPP	0ULL

/*
 * struct ssdfs_metadata_descriptor - metadata descriptor
 * @offset: offset in bytes
 * @size: size in bytes
 * @check: metadata checksum
 */
struct ssdfs_metadata_descriptor {
/* 0x0000 */
	__le32 offset;
	__le32 size;
	struct ssdfs_metadata_check check;

/* 0x0010 */
} __attribute__((packed));

enum {
	SSDFS_BLK_BMAP_INDEX,
	SSDFS_SNAPSHOT_RULES_AREA_INDEX,
	SSDFS_OFF_TABLE_INDEX,
	SSDFS_COLD_PAYLOAD_AREA_INDEX,
	SSDFS_WARM_PAYLOAD_AREA_INDEX,
	SSDFS_HOT_PAYLOAD_AREA_INDEX,
	SSDFS_BLK_DESC_AREA_INDEX,
	SSDFS_MAPTBL_CACHE_INDEX,
	SSDFS_LOG_FOOTER_INDEX,
	SSDFS_SEG_HDR_DESC_MAX = SSDFS_LOG_FOOTER_INDEX + 1,
	SSDFS_LOG_FOOTER_DESC_MAX = SSDFS_OFF_TABLE_INDEX + 1,
};

enum {
	SSDFS_PREV_MIGRATING_PEB,
	SSDFS_CUR_MIGRATING_PEB,
	SSDFS_MIGRATING_PEBS_CHAIN
};

/*
 * struct ssdfs_segment_header - header of segment
 * @volume_hdr: copy of static part of superblock
 * @timestamp: log creation timestamp
 * @cno: log checkpoint
 * @log_pages: size of log (partial segment) in pages count
 * @seg_type: type of segment
 * @seg_flags: flags of segment
 * @desc_array: array of segment's metadata descriptors
 * @peb_migration_id: identification number of PEB in migration sequence
 * @peb_create_time: PEB creation timestamp
 * @seg_id: segment ID that contains this PEB
 * @leb_id: LEB ID that mapped with this PEB
 * @peb_id: PEB ID
 * @relation_peb_id: source PEB ID during migration
 * @payload: space for segment header's payload
 */
struct ssdfs_segment_header {
/* 0x0000 */
	struct ssdfs_volume_header volume_hdr;

/* 0x0400 */
	__le64 timestamp;
	__le64 cno;

/* 0x0410 */
	__le16 log_pages;
	__le16 seg_type;
	__le32 seg_flags;

/* 0x0418 */
	struct ssdfs_metadata_descriptor desc_array[SSDFS_SEG_HDR_DESC_MAX];

/* 0x04A8 */
#define SSDFS_PEB_UNKNOWN_MIGRATION_ID		(0)
#define SSDFS_PEB_MIGRATION_ID_START		(1)
#define SSDFS_PEB_MIGRATION_ID_MAX		(U8_MAX)
	__le8 peb_migration_id[SSDFS_MIGRATING_PEBS_CHAIN];
	__le8 reserved[0x6];

/* 0x4B0 */
	__le64 peb_create_time;

/* 0x4B8 */
	__le64 seg_id;
	__le64 leb_id;
	__le64 peb_id;
	__le64 relation_peb_id;

/* 0x4D8 */
	__le8 payload[0x328];

/* 0x0800 */
} __attribute__((packed));

/* Possible segment types */
#define SSDFS_UNKNOWN_SEG_TYPE			(0)
#define SSDFS_SB_SEG_TYPE			(1)
#define SSDFS_INITIAL_SNAPSHOT_SEG_TYPE		(2)
#define SSDFS_SEGBMAP_SEG_TYPE			(3)
#define SSDFS_MAPTBL_SEG_TYPE			(4)
#define SSDFS_LEAF_NODE_SEG_TYPE		(5)
#define SSDFS_HYBRID_NODE_SEG_TYPE		(6)
#define SSDFS_INDEX_NODE_SEG_TYPE		(7)
#define SSDFS_USER_DATA_SEG_TYPE		(8)
#define SSDFS_LAST_KNOWN_SEG_TYPE		SSDFS_USER_DATA_SEG_TYPE

/* Segment flags' bits */
#define SSDFS_BLK_BMAP_BIT			(0)
#define SSDFS_OFFSET_TABLE_BIT			(1)
#define SSDFS_COLD_PAYLOAD_BIT			(2)
#define SSDFS_WARM_PAYLOAD_BIT			(3)
#define SSDFS_HOT_PAYLOAD_BIT			(4)
#define SSDFS_BLK_DESC_CHAIN_BIT		(5)
#define SSDFS_MAPTBL_CACHE_BIT			(6)
#define SSDFS_FOOTER_BIT			(7)
#define SSDFS_PARTIAL_LOG_BIT			(8)
#define SSDFS_PARTIAL_LOG_HEADER_BIT		(9)
#define SSDFS_PLH_INSTEAD_FOOTER_BIT		(10)

/* Segment flags */
#define SSDFS_SEG_HDR_HAS_BLK_BMAP		(1 << SSDFS_BLK_BMAP_BIT)
#define SSDFS_SEG_HDR_HAS_OFFSET_TABLE		(1 << SSDFS_OFFSET_TABLE_BIT)
#define SSDFS_LOG_HAS_COLD_PAYLOAD		(1 << SSDFS_COLD_PAYLOAD_BIT)
#define SSDFS_LOG_HAS_WARM_PAYLOAD		(1 << SSDFS_WARM_PAYLOAD_BIT)
#define SSDFS_LOG_HAS_HOT_PAYLOAD		(1 << SSDFS_HOT_PAYLOAD_BIT)
#define SSDFS_LOG_HAS_BLK_DESC_CHAIN		(1 << SSDFS_BLK_DESC_CHAIN_BIT)
#define SSDFS_LOG_HAS_MAPTBL_CACHE		(1 << SSDFS_MAPTBL_CACHE_BIT)
#define SSDFS_LOG_HAS_FOOTER			(1 << SSDFS_FOOTER_BIT)
#define SSDFS_LOG_IS_PARTIAL			(1 << SSDFS_PARTIAL_LOG_BIT)
#define SSDFS_LOG_HAS_PARTIAL_HEADER		(1 << SSDFS_PARTIAL_LOG_HEADER_BIT)
#define SSDFS_PARTIAL_HEADER_INSTEAD_FOOTER	(1 << SSDFS_PLH_INSTEAD_FOOTER_BIT)
#define SSDFS_SEG_HDR_FLAG_MASK			0x7FF

/*
 * struct ssdfs_log_footer - footer of partial log
 * @volume_state: changeable part of superblock
 * @timestamp: writing timestamp
 * @cno: writing checkpoint
 * @log_bytes: payload size in bytes
 * @log_flags: flags of log
 * @reserved1: reserved field
 * @desc_array: array of footer's metadata descriptors
 * @peb_create_time: PEB creation timestamp
 * @payload: space for log footer's payload
 */
struct ssdfs_log_footer {
/* 0x0000 */
	struct ssdfs_volume_state volume_state;

/* 0x0400 */
	__le64 timestamp;
	__le64 cno;

/* 0x0410 */
	__le32 log_bytes;
	__le32 log_flags;
	__le64 reserved1;

/* 0x0420 */
	struct ssdfs_metadata_descriptor desc_array[SSDFS_LOG_FOOTER_DESC_MAX];

/* 0x0450 */
	__le64 peb_create_time;

/* 0x0458 */
	__le8 payload[0x3A8];

/* 0x0800 */
} __attribute__((packed));

/* Log footer flags' bits */
#define __SSDFS_BLK_BMAP_BIT			(0)
#define __SSDFS_OFFSET_TABLE_BIT		(1)
#define __SSDFS_PARTIAL_LOG_BIT			(2)
#define __SSDFS_ENDING_LOG_BIT			(3)
#define __SSDFS_SNAPSHOT_RULE_AREA_BIT		(4)

/* Log footer flags */
#define SSDFS_LOG_FOOTER_HAS_BLK_BMAP		(1 << __SSDFS_BLK_BMAP_BIT)
#define SSDFS_LOG_FOOTER_HAS_OFFSET_TABLE	(1 << __SSDFS_OFFSET_TABLE_BIT)
#define SSDFS_PARTIAL_LOG_FOOTER		(1 << __SSDFS_PARTIAL_LOG_BIT)
#define SSDFS_ENDING_LOG_FOOTER			(1 << __SSDFS_ENDING_LOG_BIT)
#define SSDFS_LOG_FOOTER_HAS_SNAPSHOT_RULES	(1 << __SSDFS_SNAPSHOT_RULE_AREA_BIT)
#define SSDFS_LOG_FOOTER_FLAG_MASK		0x1F

/*
 * struct ssdfs_partial_log_header - header of partial log
 * @magic: magic signature + revision
 * @check: metadata checksum
 * @timestamp: writing timestamp
 * @cno: writing checkpoint
 * @log_pages: size of log in pages count
 * @seg_type: type of segment
 * @pl_flags: flags of log
 * @log_bytes: payload size in bytes
 * @flags: volume flags
 * @desc_array: array of log's metadata descriptors
 * @nsegs: segments count
 * @free_pages: free pages count
 * @root_folder: copy of root folder's inode
 * @inodes_btree: inodes btree root
 * @shared_extents_btree: shared extents btree root
 * @shared_dict_btree: shared dictionary btree root
 * @sequence_id: index of partial log in the sequence
 * @log_pagesize: log2(page size)
 * @log_erasesize: log2(erase block size)
 * @log_segsize: log2(segment size)
 * @log_pebs_per_seg: log2(erase blocks per segment)
 * @lebs_per_peb_index: difference of LEB IDs between PEB indexes in segment
 * @create_threads_per_seg: number of creation threads per segment
 * @snapshots_btree: snapshots btree root
 * @open_zones: number of open/active zones
 * @peb_create_time: PEB creation timestamp
 * @invextree: invalidated extents btree root
 * @seg_id: segment ID that contains this PEB
 * @leb_id: LEB ID that mapped with this PEB
 * @peb_id: PEB ID
 * @relation_peb_id: source PEB ID during migration
 * @uuid: 128-bit uuid for volume
 * @volume_create_time: volume create timestamp (mkfs phase)
 *
 * This header is used when the full log needs to be built from several
 * partial logs. The header represents the combination of the most
 * essential fields of segment header and log footer. The first partial
 * log starts from the segment header and partial log header. The next
 * every partial log starts from the partial log header. Only the latest
 * log ends with the log footer.
 */
struct ssdfs_partial_log_header {
/* 0x0000 */
	struct ssdfs_signature magic;

/* 0x0008 */
	struct ssdfs_metadata_check check;

/* 0x0010 */
	__le64 timestamp;
	__le64 cno;

/* 0x0020 */
	__le16 log_pages;
	__le16 seg_type;
	__le32 pl_flags;

/* 0x0028 */
	__le32 log_bytes;
	__le32 flags;

/* 0x0030 */
	struct ssdfs_metadata_descriptor desc_array[SSDFS_SEG_HDR_DESC_MAX];

/* 0x00C0 */
	__le64 nsegs;
	__le64 free_pages;

/* 0x00D0 */
	struct ssdfs_inode root_folder;

/* 0x01D0 */
	struct ssdfs_inodes_btree inodes_btree;

/* 0x0250 */
	struct ssdfs_shared_extents_btree shared_extents_btree;

/* 0x02D0 */
	struct ssdfs_shared_dictionary_btree shared_dict_btree;

/* 0x0350 */
	__le32 sequence_id;
	__le8 log_pagesize;
	__le8 log_erasesize;
	__le8 log_segsize;
	__le8 log_pebs_per_seg;
	__le32 lebs_per_peb_index;
	__le16 create_threads_per_seg;
	__le8 reserved1[0x2];

/* 0x0360 */
	struct ssdfs_snapshots_btree snapshots_btree;

/* 0x03E0 */
	__le32 open_zones;
	__le8 reserved2[0x4];
	__le64 peb_create_time;
	__le8 reserved3[0x10];

/* 0x0400 */
	struct ssdfs_invalidated_extents_btree invextree;

/* 0x0480 */
	__le64 seg_id;
	__le64 leb_id;
	__le64 peb_id;
	__le64 relation_peb_id;

/* 0x04A0 */
	__le8 uuid[SSDFS_UUID_SIZE];

/* 0x04B0 */
	__le64 volume_create_time;

/* 0x04B8 */
	__le8 payload[0x348];

/* 0x0800 */
} __attribute__((packed));

/*
 * struct ssdfs_diff_blob_header - diff blob header
 * @magic: diff blob's magic
 * @type: diff blob's type
 * @desc_size: size of diff blob's descriptor in bytes
 * @blob_size: size of diff blob in bytes
 * @flags: diff blob's flags
 */
struct ssdfs_diff_blob_header {
/* 0x0000 */
	__le16 magic;
	__le8 type;
	__le8 desc_size;
	__le16 blob_size;
	__le16 flags;

/* 0x0008 */
} __attribute__((packed));

/* Diff blob flags */
#define SSDFS_DIFF_BLOB_HAS_BTREE_NODE_HEADER	(1 << 0)
#define SSDFS_DIFF_CHAIN_CONTAINS_NEXT_BLOB	(1 << 1)
#define SSDFS_DIFF_BLOB_FLAGS_MASK		(0x3)

/*
 * struct ssdfs_metadata_diff_blob_header - metadata diff blob header
 * @diff: generic diff blob header
 * @bits_count: count of bits in bitmap
 * @item_start_bit: item starting bit in bitmap
 * @index_start_bit: index starting bit in bitmap
 * @item_size: size of item in bytes
 */
struct ssdfs_metadata_diff_blob_header {
/* 0x0000 */
	struct ssdfs_diff_blob_header diff;

/* 0x0008 */
	__le16 bits_count;
	__le16 item_start_bit;
	__le16 index_start_bit;
	__le16 item_size;

/* 0x0010 */
} __attribute__((packed));

/* Diff blob types */
enum {
	SSDFS_UNKNOWN_DIFF_BLOB_TYPE,
	SSDFS_BTREE_NODE_DIFF_BLOB,
	SSDFS_USER_DATA_DIFF_BLOB,
	SSDFS_DIFF_BLOB_TYPE_MAX
};

/*
 * struct ssdfs_fragments_chain_header - header of fragments' chain
 * @compr_bytes: size of the whole fragments' chain in compressed state
 * @uncompr_bytes: size of the whole fragments' chain in decompressed state
 * @fragments_count: count of fragments in the chain
 * @desc_size: size of one descriptor item
 * @magic: fragments chain header magic
 * @type: fragments chain header type
 * @flags: flags of fragments' chain
 */
struct ssdfs_fragments_chain_header {
/* 0x0000 */
	__le32 compr_bytes;
	__le32 uncompr_bytes;

/* 0x0008 */
	__le16 fragments_count;
	__le16 desc_size;

/* 0x000C */
	__le8 magic;
	__le8 type;
	__le16 flags;

/* 0x0010 */
} __attribute__((packed));

/* Fragments chain types */
#define SSDFS_UNKNOWN_CHAIN_HDR		0x0
#define SSDFS_LOG_AREA_CHAIN_HDR	0x1
#define SSDFS_BLK_STATE_CHAIN_HDR	0x2
#define SSDFS_BLK_DESC_CHAIN_HDR	0x3
#define SSDFS_BLK_DESC_ZLIB_CHAIN_HDR	0x4
#define SSDFS_BLK_DESC_LZO_CHAIN_HDR	0x5
#define SSDFS_BLK2OFF_CHAIN_HDR		0x6
#define SSDFS_BLK2OFF_ZLIB_CHAIN_HDR	0x7
#define SSDFS_BLK2OFF_LZO_CHAIN_HDR	0x8
#define SSDFS_BLK_BMAP_CHAIN_HDR	0x9
#define SSDFS_CHAIN_HDR_TYPE_MAX	(SSDFS_BLK_BMAP_CHAIN_HDR + 1)

/* Fragments chain flags */
#define SSDFS_MULTIPLE_HDR_CHAIN	(1 << 0)
#define SSDFS_CHAIN_HDR_FLAG_MASK	0x1

/* Fragments chain constants */
#define SSDFS_FRAGMENTS_CHAIN_MAX		14
#define SSDFS_BLK_BMAP_FRAGMENTS_CHAIN_MAX	64

/*
 * struct ssdfs_fragment_desc - fragment descriptor
 * @offset: fragment's offset
 * @compr_size: size of fragment in compressed state
 * @uncompr_size: size of fragment after decompression
 * @checksum: fragment checksum
 * @sequence_id: fragment's sequential id number
 * @magic: fragment descriptor's magic
 * @type: fragment descriptor's type
 * @flags: fragment descriptor's flags
 */
struct ssdfs_fragment_desc {
/* 0x0000 */
	__le32 offset;
	__le16 compr_size;
	__le16 uncompr_size;

/* 0x0008 */
	__le32 checksum;
	__le8 sequence_id;
	__le8 magic;
	__le8 type;
	__le8 flags;

/* 0x0010 */
} __attribute__((packed));

/* Fragment descriptor types */
#define SSDFS_UNKNOWN_FRAGMENT_TYPE	0
#define SSDFS_FRAGMENT_UNCOMPR_BLOB	1
#define SSDFS_FRAGMENT_ZLIB_BLOB	2
#define SSDFS_FRAGMENT_LZO_BLOB		3
#define SSDFS_DATA_BLK_STATE_DESC	4
#define SSDFS_DATA_BLK_DESC		5
#define SSDFS_DATA_BLK_DESC_ZLIB	6
#define SSDFS_DATA_BLK_DESC_LZO		7
#define SSDFS_BLK2OFF_EXTENT_DESC	8
#define SSDFS_BLK2OFF_EXTENT_DESC_ZLIB	9
#define SSDFS_BLK2OFF_EXTENT_DESC_LZO	10
#define SSDFS_BLK2OFF_DESC		11
#define SSDFS_BLK2OFF_DESC_ZLIB		12
#define SSDFS_BLK2OFF_DESC_LZO		13
#define SSDFS_NEXT_TABLE_DESC		14
#define SSDFS_FRAGMENT_DESC_MAX_TYPE	(SSDFS_NEXT_TABLE_DESC + 1)

/* Fragment descriptor flags */
#define SSDFS_FRAGMENT_HAS_CSUM		(1 << 0)
#define SSDFS_FRAGMENT_DESC_FLAGS_MASK	0x1

/*
 * struct ssdfs_block_bitmap_header - header of segment's block bitmap
 * @magic: magic signature and flags
 * @fragments_count: count of block bitmap's fragments
 * @bytes_count: count of bytes in fragments' sequence
 * @flags: block bitmap's flags
 * @type: type of block bitmap
 */
struct ssdfs_block_bitmap_header {
/* 0x0000 */
	struct ssdfs_signature magic;

/* 0x0008 */
	__le16 fragments_count;
	__le32 bytes_count;

#define SSDFS_BLK_BMAP_BACKUP		(1 << 0)
#define SSDFS_BLK_BMAP_COMPRESSED	(1 << 1)
#define SSDFS_BLK_BMAP_FLAG_MASK	0x3
	__le8 flags;

#define SSDFS_BLK_BMAP_UNCOMPRESSED_BLOB	(0)
#define SSDFS_BLK_BMAP_ZLIB_BLOB		(1)
#define SSDFS_BLK_BMAP_LZO_BLOB			(2)
#define SSDFS_BLK_BMAP_TYPE_MAX			(SSDFS_BLK_BMAP_LZO_BLOB + 1)
	__le8 type;

/* 0x0010 */
} __attribute__((packed));

/*
 * struct ssdfs_block_bitmap_fragment - block bitmap's fragment header
 * @peb_index: PEB's index
 * @sequence_id: ID of block bitmap's fragment in the sequence
 * @flags: fragment's flags
 * @type: fragment type
 * @last_free_blk: last logical free block
 * @metadata_blks: count of physical pages are used by metadata
 * @invalid_blks: count of invalid blocks
 * @chain_hdr: descriptor of block bitmap's fragments' chain
 */
struct ssdfs_block_bitmap_fragment {
/* 0x0000 */
	__le16 peb_index;
	__le8 sequence_id;

#define SSDFS_MIGRATING_BLK_BMAP	(1 << 0)
#define SSDFS_PEB_HAS_EXT_PTR		(1 << 1)
#define SSDFS_PEB_HAS_RELATION		(1 << 2)
#define SSDFS_INFLATED_BLK_BMAP		(1 << 3)
#define SSDFS_FRAG_BLK_BMAP_FLAG_MASK	0xF
	__le8 flags : 6;

#define SSDFS_SRC_BLK_BMAP		(0)
#define SSDFS_DST_BLK_BMAP		(1)
#define SSDFS_FRAG_BLK_BMAP_TYPE_MAX	(SSDFS_DST_BLK_BMAP + 1)
	__le8 type : 2;

	__le32 last_free_blk;

/* 0x0008 */
	__le32 metadata_blks;
	__le32 invalid_blks;

/* 0x0010 */
	struct ssdfs_fragments_chain_header chain_hdr;

/* 0x0020 */
} __attribute__((packed));

/*
 * The block to offset table has structure:
 *
 * ----------------------------
 * |                          |
 * |  Blk2Off table Header    |
 * |                          |
 * ----------------------------
 * |                          |
 * |   Translation extents    |
 * |        sequence          |
 * |                          |
 * ----------------------------
 * |                          |
 * |  Physical offsets table  |
 * |         header           |
 * |                          |
 * ----------------------------
 * |                          |
 * |    Physical offset       |
 * |  descriptors sequence    |
 * |                          |
 * ----------------------------
 */

/* Possible log's area types */
enum {
	SSDFS_LOG_BLK_DESC_AREA,
	SSDFS_LOG_MAIN_AREA,
	SSDFS_LOG_DIFFS_AREA,
	SSDFS_LOG_JOURNAL_AREA,
	SSDFS_LOG_AREA_MAX,
};

/*
 * struct ssdfs_peb_page_descriptor - PEB's page descriptor
 * @logical_offset: logical offset from file's begin in pages
 * @logical_blk: logical number of the block in segment
 * @peb_page: PEB's page index
 */
struct ssdfs_peb_page_descriptor {
/* 0x0000 */
	__le32 logical_offset;
	__le16 logical_blk;
	__le16 peb_page;

/* 0x0008 */
} __attribute__((packed));

/*
 * struct ssdfs_blk_state_offset - block's state offset
 * @log_start_page: start page of the log
 * @log_area: identification number of log area
 * @peb_migration_id: identification number of PEB in migration sequence
 * @byte_offset: offset in bytes from area's beginning
 */
struct ssdfs_blk_state_offset {
/* 0x0000 */
	__le16 log_start_page;
	__le8 log_area;
	__le8 peb_migration_id;
	__le32 byte_offset;

/* 0x0008 */
} __attribute__((packed));

/*
 * struct ssdfs_phys_offset_descriptor - descriptor of physical offset
 * @page_desc: PEB's page descriptor
 * @blk_state: logical block's state offset
 */
struct ssdfs_phys_offset_descriptor {
/* 0x0000 */
	struct ssdfs_peb_page_descriptor page_desc;
	struct ssdfs_blk_state_offset blk_state;

/* 0x0010 */
} __attribute__((packed));

/*
 * struct ssdfs_phys_offset_table_header - physical offset table header
 * @start_id: start id in the table's fragment
 * @id_count: number of unique physical offsets in log's fragments chain
 * @byte_size: size in bytes of table's fragment
 * @peb_index: PEB index
 * @sequence_id: table's fragment's sequential id number
 * @type: table's type
 * @flags: table's flags
 * @magic: table's magic
 * @checksum: table checksum
 * @used_logical_blks: count of allocated logical blocks
 * @free_logical_blks: count of free logical blocks
 * @last_allocated_blk: last allocated block (hint for allocation)
 * @next_fragment_off: offset till next table's fragment
 *
 * This table contains offsets of block descriptors in a segment.
 * Generally speaking, table can be represented as array of
 * ssdfs_phys_offset_descriptor structures are ordered by id
 * numbers. The whole table can be split on several fragments.
 * Every table's fragment begins from header.
 */
struct ssdfs_phys_offset_table_header {
/* 0x0000 */
	__le16 start_id;
	__le16 id_count;
	__le32 byte_size;

/* 0x0008 */
	__le16 peb_index;
	__le16 sequence_id;
	__le16 type;
	__le16 flags;

/* 0x0010 */
	__le32 magic;
	__le32 checksum;

/* 0x0018 */
	__le16 used_logical_blks;
	__le16 free_logical_blks;
	__le16 last_allocated_blk;
	__le16 next_fragment_off;

/* 0x0020 */
} __attribute__((packed));

/* Physical offset table types */
#define SSDFS_UNKNOWN_OFF_TABLE_TYPE	0
#define SSDFS_SEG_OFF_TABLE		1
#define SSDFS_OFF_TABLE_MAX_TYPE	(SSDFS_SEG_OFF_TABLE + 1)

/* Physical offset table flags */
#define SSDFS_OFF_TABLE_HAS_CSUM		(1 << 0)
#define SSDFS_OFF_TABLE_HAS_NEXT_FRAGMENT	(1 << 1)
#define SSDFS_BLK_DESC_TBL_COMPRESSED		(1 << 2)
#define SSDFS_OFF_TABLE_HAS_OLD_LOG_FRAGMENT	(1 << 3)
#define SSDFS_INFLATED_OFF_TABLE		(1 << 4)
#define SSDFS_OFF_TABLE_FLAGS_MASK		0x1F

/*
 * struct ssdfs_translation_extent - logical block to offset id translation
 * @logical_blk: starting logical block
 * @offset_id: starting offset id
 * @len: count of items in extent
 * @sequence_id: id in sequence of extents
 * @state: logical blocks' sequence state
 */
struct ssdfs_translation_extent {
/* 0x0000 */
	__le16 logical_blk;
#define SSDFS_INVALID_OFFSET_ID		(U16_MAX)
	__le16 offset_id;
	__le16 len;
	__le8 sequence_id;
	__le8 state;

/* 0x0008 */
} __attribute__((packed));

enum {
	SSDFS_LOGICAL_BLK_UNKNOWN_STATE,
	SSDFS_LOGICAL_BLK_FREE,
	SSDFS_LOGICAL_BLK_USED,
	SSDFS_LOGICAL_BLK_STATE_MAX,
};

/*
 * struct ssdfs_blk2off_table_header - translation table header
 * @magic: magic signature
 * @check: metadata checksum + flags
 * @extents_off: offset in bytes from header begin till extents sequence
 * @extents_count: count of extents in the sequence
 * @offset_table_off: offset in bytes from header begin till phys offsets table
 * @fragments_count: count of table's fragments for the whole PEB
 * @sequence: first translation extent in the sequence
 */
struct ssdfs_blk2off_table_header {
/* 0x0000 */
	struct ssdfs_signature magic;

/* 0x0008 */
#define SSDFS_BLK2OFF_TBL_ZLIB_COMPR	(1 << 1)
#define SSDFS_BLK2OFF_TBL_LZO_COMPR	(1 << 2)
	struct ssdfs_metadata_check check;

/* 0x0010 */
	struct ssdfs_fragments_chain_header chain_hdr;

/* 0x0020 */
#define SSDFS_BLK2OFF_FRAG_CHAIN_MAX	(5)
#define SSDFS_NEXT_BLK2OFF_TBL_INDEX	SSDFS_BLK2OFF_FRAG_CHAIN_MAX
#define SSDFS_BLK2OFF_TBL_MAX		(SSDFS_BLK2OFF_FRAG_CHAIN_MAX + 1)
	struct ssdfs_fragment_desc blk[SSDFS_BLK2OFF_TBL_MAX];

/* 0x0080 */
} __attribute__((packed));

/*
 * The block's descriptor table has structure:
 *
 * ----------------------------
 * |                          |
 * | Area block table #0      |
 * |  Fragment descriptor #0  |
 * |          ***             |
 * |  Fragment descriptor #14 |
 * |  Next area block table   |
 * |        descriptor        |
 * |                          |
 * ----------------------------
 * |                          |
 * |    Block descriptor #0   |
 * |           ***            |
 * |    Block descriptor #N   |
 * |                          |
 * ----------------------------
 * |                          |
 * |          ***             |
 * |                          |
 * ----------------------------
 * |                          |
 * |    Block descriptor #0   |
 * |           ***            |
 * |    Block descriptor #N   |
 * |                          |
 * ----------------------------
 * |                          |
 * |          ***             |
 * |                          |
 * ----------------------------
 * |                          |
 * | Area block table #N      |
 * |  Fragment descriptor #0  |
 * |          ***             |
 * |  Fragment descriptor #14 |
 * |  Next area block table   |
 * |        descriptor        |
 * |                          |
 * ----------------------------
 * |                          |
 * |    Block descriptor #0   |
 * |           ***            |
 * |    Block descriptor #N   |
 * |                          |
 * ----------------------------
 * |                          |
 * |          ***             |
 * |                          |
 * ----------------------------
 * |                          |
 * |    Block descriptor #0   |
 * |           ***            |
 * |    Block descriptor #N   |
 * |                          |
 * ----------------------------
 */

#define SSDFS_BLK_STATE_OFF_MAX		6

/*
 * struct ssdfs_block_descriptor - block descriptor
 * @ino: inode identification number
 * @logical_offset: logical offset from file's begin in pages
 * @peb_index: PEB's index
 * @peb_page: PEB's page index
 * @state: array of fragment's offsets
 */
struct ssdfs_block_descriptor {
/* 0x0000 */
	__le64 ino;
	__le32 logical_offset;
	__le16 peb_index;
	__le16 peb_page;

/* 0x0010 */
	struct ssdfs_blk_state_offset state[SSDFS_BLK_STATE_OFF_MAX];

/* 0x0040 */
} __attribute__((packed));

/*
 * struct ssdfs_area_block_table - descriptor of block state sequence in area
 * @chain_hdr: descriptor of block states' chain
 * @blk: table of fragment descriptors
 *
 * This table describes block state sequence in PEB's area. This table
 * can consists from several parts. Every part can describe 14 blocks
 * in partial sequence. If sequence contains more block descriptors
 * then last fragment descriptor describes placement of next part of
 * block table and so on.
 */
struct ssdfs_area_block_table {
/* 0x0000 */
	struct ssdfs_fragments_chain_header chain_hdr;

/* 0x0010 */
#define SSDFS_NEXT_BLK_TABLE_INDEX	SSDFS_FRAGMENTS_CHAIN_MAX
#define SSDFS_BLK_TABLE_MAX		(SSDFS_FRAGMENTS_CHAIN_MAX + 1)
	struct ssdfs_fragment_desc blk[SSDFS_BLK_TABLE_MAX];

/* 0x0100 */
} __attribute__((packed));

/*
 * The data (diff, journaling) area has structure:
 * -----------------------------
 * |                           |
 * | Block state descriptor #0 |
 * |  Fragment descriptor #0   |
 * |          ***              |
 * |  Fragment descriptor #N   |
 * |                           |
 * -----------------------------
 * |                           |
 * |   Data portion #0         |
 * |          ***              |
 * |   Data portion #N         |
 * |                           |
 * -----------------------------
 * |                           |
 * |          ***              |
 * |                           |
 * -----------------------------
 * |                           |
 * | Block state descriptor #N |
 * |  Fragment descriptor #0   |
 * |          ***              |
 * |  Fragment descriptor #N   |
 * |                           |
 * -----------------------------
 * |                           |
 * |   Data portion #0         |
 * |          ***              |
 * |   Data portion #N         |
 * |                           |
 * -----------------------------
 */

/*
 * ssdfs_block_state_descriptor - block's state descriptor
 * @cno: checkpoint
 * @parent_snapshot: parent snapshot
 * @chain_hdr: descriptor of data fragments' chain
 */
struct ssdfs_block_state_descriptor {
/* 0x0000 */
	__le64 cno;
	__le64 parent_snapshot;

/* 0x0010 */
	struct ssdfs_fragments_chain_header chain_hdr;

/* 0x0020 */
} __attribute__((packed));

/*
 * struct ssdfs_segbmap_fragment_header - segment bitmap fragment header
 * @magic: magic signature
 * @seg_index: segment index in segment bitmap fragments' chain
 * @peb_index: PEB's index in segment
 * @flags: fragment's flags
 * @seg_type: segment type (main/backup)
 * @start_item: fragment's start item number
 * @sequence_id: fragment identification number
 * @fragment_bytes: bytes count in fragment
 * @checksum: fragment checksum
 * @total_segs: count of total segments in fragment
 * @clean_or_using_segs: count of clean or using segments in fragment
 * @used_or_dirty_segs: count of used or dirty segments in fragment
 * @bad_segs: count of bad segments in fragment
 */
struct ssdfs_segbmap_fragment_header {
/* 0x0000 */
	__le16 magic;
	__le16 seg_index;
	__le16 peb_index;
#define SSDFS_SEGBMAP_FRAG_ZLIB_COMPR	(1 << 0)
#define SSDFS_SEGBMAP_FRAG_LZO_COMPR	(1 << 1)
	__le8 flags;
	__le8 seg_type;

/* 0x0008 */
	__le64 start_item;

/* 0x0010 */
	__le16 sequence_id;
	__le16 fragment_bytes;
	__le32 checksum;

/* 0x0018 */
	__le16 total_segs;
	__le16 clean_or_using_segs;
	__le16 used_or_dirty_segs;
	__le16 bad_segs;

/* 0x0020 */
} __attribute__((packed));

/*
 * struct ssdfs_peb_descriptor - descriptor of PEB
 * @erase_cycles: count of P/E cycles of PEB
 * @type: PEB's type
 * @state: PEB's state
 * @flags: PEB's flags
 * @shared_peb_index: index of external shared destination PEB
 */
struct ssdfs_peb_descriptor {
/* 0x0000 */
	__le32 erase_cycles;
	__le8 type;
	__le8 state;
	__le8 flags;
	__le8 shared_peb_index;

/* 0x0008 */
} __attribute__((packed));

/* PEB's types */
enum {
	SSDFS_MAPTBL_UNKNOWN_PEB_TYPE,				/* 0x00 */
	SSDFS_MAPTBL_DATA_PEB_TYPE,				/* 0x01 */
	SSDFS_MAPTBL_LNODE_PEB_TYPE,				/* 0x02 */
	SSDFS_MAPTBL_HNODE_PEB_TYPE,				/* 0x03 */
	SSDFS_MAPTBL_IDXNODE_PEB_TYPE,				/* 0x04 */
	SSDFS_MAPTBL_INIT_SNAP_PEB_TYPE,			/* 0x05 */
	SSDFS_MAPTBL_SBSEG_PEB_TYPE,				/* 0x06 */
	SSDFS_MAPTBL_SEGBMAP_PEB_TYPE,				/* 0x07 */
	SSDFS_MAPTBL_MAPTBL_PEB_TYPE,				/* 0x08 */
	SSDFS_MAPTBL_PEB_TYPE_MAX				/* 0x09 */
};

/* PEB's states */
enum {
	SSDFS_MAPTBL_UNKNOWN_PEB_STATE,				/* 0x00 */
	SSDFS_MAPTBL_BAD_PEB_STATE,				/* 0x01 */
	SSDFS_MAPTBL_CLEAN_PEB_STATE,				/* 0x02 */
	SSDFS_MAPTBL_USING_PEB_STATE,				/* 0x03 */
	SSDFS_MAPTBL_USED_PEB_STATE,				/* 0x04 */
	SSDFS_MAPTBL_PRE_DIRTY_PEB_STATE,			/* 0x05 */
	SSDFS_MAPTBL_DIRTY_PEB_STATE,				/* 0x06 */
	SSDFS_MAPTBL_MIGRATION_SRC_USING_STATE,			/* 0x07 */
	SSDFS_MAPTBL_MIGRATION_SRC_USED_STATE,			/* 0x08 */
	SSDFS_MAPTBL_MIGRATION_SRC_PRE_DIRTY_STATE,		/* 0x09 */
	SSDFS_MAPTBL_MIGRATION_SRC_DIRTY_STATE,			/* 0x0A */
	SSDFS_MAPTBL_MIGRATION_DST_CLEAN_STATE,			/* 0x0B */
	SSDFS_MAPTBL_MIGRATION_DST_USING_STATE,			/* 0x0C */
	SSDFS_MAPTBL_MIGRATION_DST_USED_STATE,			/* 0x0D */
	SSDFS_MAPTBL_MIGRATION_DST_PRE_DIRTY_STATE,		/* 0x0E */
	SSDFS_MAPTBL_MIGRATION_DST_DIRTY_STATE,			/* 0x0F */
	SSDFS_MAPTBL_PRE_ERASE_STATE,				/* 0x10 */
	SSDFS_MAPTBL_UNDER_ERASE_STATE,				/* 0x11 */
	SSDFS_MAPTBL_SNAPSHOT_STATE,				/* 0x12 */
	SSDFS_MAPTBL_RECOVERING_STATE,				/* 0x13 */
	SSDFS_MAPTBL_PEB_STATE_MAX				/* 0x14 */
};

/* PEB's flags */
#define SSDFS_MAPTBL_SHARED_DESTINATION_PEB		(1 << 0)
#define SSDFS_MAPTBL_SOURCE_PEB_HAS_EXT_PTR		(1 << 1)
#define SSDFS_MAPTBL_SOURCE_PEB_HAS_ZONE_PTR		(1 << 2)

#define SSDFS_PEBTBL_BMAP_SIZE \
	((PAGE_CACHE_SIZE / sizeof(struct ssdfs_peb_descriptor)) / \
	 BITS_PER_BYTE)

/* PEB table's bitmap types */
enum {
	SSDFS_PEBTBL_USED_BMAP,
	SSDFS_PEBTBL_DIRTY_BMAP,
	SSDFS_PEBTBL_RECOVER_BMAP,
	SSDFS_PEBTBL_BADBLK_BMAP,
	SSDFS_PEBTBL_BMAP_MAX
};

/*
 * struct ssdfs_peb_table_fragment_header - header of PEB table fragment
 * @magic: signature of PEB table's fragment
 * @flags: flags of PEB table's fragment
 * @recover_months: recovering duration in months
 * @recover_threshold: recover threshold
 * @checksum: checksum of PEB table's fragment
 * @start_peb: starting PEB number
 * @pebs_count: count of PEB's descriptors in table's fragment
 * @last_selected_peb: index of last selected unused PEB
 * @reserved_pebs: count of reserved PEBs in table's fragment
 * @stripe_id: stripe identification number
 * @portion_id: sequential ID of mapping table fragment
 * @fragment_id: sequential ID of PEB table fragment in the portion
 * @bytes_count: table's fragment size in bytes
 * @bmap: PEB table fragment's bitmap
 */
struct ssdfs_peb_table_fragment_header {
/* 0x0000 */
	__le16 magic;
	__le8 flags;
	__le8 recover_months : 4;
	__le8 recover_threshold : 4;
	__le32 checksum;

/* 0x0008 */
	__le64 start_peb;

/* 0x0010 */
	__le16 pebs_count;
	__le16 last_selected_peb;
	__le16 reserved_pebs;
	__le16 stripe_id;

/* 0x0018 */
	__le16 portion_id;
	__le16 fragment_id;
	__le32 bytes_count;

/* 0x0020 */
	__le8 bmaps[SSDFS_PEBTBL_BMAP_MAX][SSDFS_PEBTBL_BMAP_SIZE];

/* 0x0120 */
} __attribute__((packed));

/* PEB table fragment's flags */
#define SSDFS_PEBTBL_FRAG_ZLIB_COMPR		(1 << 0)
#define SSDFS_PEBTBL_FRAG_LZO_COMPR		(1 << 1)
#define SSDFS_PEBTBL_UNDER_RECOVERING		(1 << 2)
#define SSDFS_PEBTBL_BADBLK_EXIST		(1 << 3)
#define SSDFS_PEBTBL_TRY_CORRECT_PEBS_AGAIN	(1 << 4)
#define SSDFS_PEBTBL_FIND_RECOVERING_PEBS \
	(SSDFS_PEBTBL_UNDER_RECOVERING | SSDFS_PEBTBL_BADBLK_EXIST)
#define SSDFS_PEBTBL_FLAGS_MASK			0x1F

/* PEB table recover thresholds */
#define SSDFS_PEBTBL_FIRST_RECOVER_TRY		(0)
#define SSDFS_PEBTBL_SECOND_RECOVER_TRY		(1)
#define SSDFS_PEBTBL_THIRD_RECOVER_TRY		(2)
#define SSDFS_PEBTBL_FOURTH_RECOVER_TRY		(3)
#define SSDFS_PEBTBL_FIFTH_RECOVER_TRY		(4)
#define SSDFS_PEBTBL_SIX_RECOVER_TRY		(5)
#define SSDFS_PEBTBL_BADBLK_THRESHOLD		(6)

#define SSDFS_PEBTBL_FRAGMENT_HDR_SIZE \
	(sizeof(struct ssdfs_peb_table_fragment_header))

#define SSDFS_PEB_DESC_PER_FRAGMENT(fragment_size) \
	((fragment_size - SSDFS_PEBTBL_FRAGMENT_HDR_SIZE) / \
	 sizeof(struct ssdfs_peb_descriptor))

/*
 * struct ssdfs_leb_descriptor - logical descriptor of erase block
 * @physical_index: PEB table's offset till PEB's descriptor
 * @relation_index: PEB table's offset till associated PEB's descriptor
 */
struct ssdfs_leb_descriptor {
/* 0x0000 */
	__le16 physical_index;
	__le16 relation_index;

/* 0x0004 */
} __attribute__((packed));

/*
 * struct ssdfs_leb_table_fragment_header - header of LEB table fragment
 * @magic: signature of LEB table's fragment
 * @flags: flags of LEB table's fragment
 * @checksum: checksum of LEB table's fragment
 * @start_leb: starting LEB number
 * @lebs_count: count of LEB's descriptors in table's fragment
 * @mapped_lebs: count of LEBs are mapped on PEBs
 * @migrating_lebs: count of LEBs under migration
 * @portion_id: sequential ID of mapping table fragment
 * @fragment_id: sequential ID of LEB table fragment in the portion
 * @bytes_count: table's fragment size in bytes
 */
struct ssdfs_leb_table_fragment_header {
/* 0x0000 */
	__le16 magic;
#define SSDFS_LEBTBL_FRAG_ZLIB_COMPR	(1 << 0)
#define SSDFS_LEBTBL_FRAG_LZO_COMPR	(1 << 1)
	__le16 flags;
	__le32 checksum;

/* 0x0008 */
	__le64 start_leb;

/* 0x0010 */
	__le16 lebs_count;
	__le16 mapped_lebs;
	__le16 migrating_lebs;
	__le16 reserved1;

/* 0x0018 */
	__le16 portion_id;
	__le16 fragment_id;
	__le32 bytes_count;

/* 0x0020 */
} __attribute__((packed));

#define SSDFS_LEBTBL_FRAGMENT_HDR_SIZE \
	(sizeof(struct ssdfs_leb_table_fragment_header))

#define SSDFS_LEB_DESC_PER_FRAGMENT(fragment_size) \
	((fragment_size - SSDFS_LEBTBL_FRAGMENT_HDR_SIZE) / \
	 sizeof(struct ssdfs_leb_descriptor))

/*
 * The mapping table cache is the copy of content of mapping
 * table for some type of PEBs. The goal of cache is to provide
 * the space for storing the copy of LEB_ID/PEB_ID pairs with
 * PEB state record. The cache is using for conversion LEB ID
 * to PEB ID and retrieving the PEB state record in the case
 * when the fragment of mapping table is not initialized yet.
 * Also the cache needs for storing modified PEB state during
 * the mapping table destruction. The fragment of mapping table
 * cache has structure:
 *
 * ----------------------------
 * |                          |
 * |         Header           |
 * |                          |
 * ----------------------------
 * |                          |
 * |   LEB_ID/PEB_ID pairs    |
 * |                          |
 * ----------------------------
 * |                          |
 * |    PEB state records     |
 * |                          |
 * ----------------------------
 */

/*
 * struct ssdfs_maptbl_cache_header - maptbl cache header
 * @magic: magic signature
 * @sequence_id: ID of fragment in the sequence
 * @flags: maptbl cache header's flags
 * @items_count: count of items in maptbl cache's fragment
 * @bytes_count: size of fragment in bytes
 * @start_leb: start LEB ID in fragment
 * @end_leb: ending LEB ID in fragment
 */
struct ssdfs_maptbl_cache_header {
/* 0x0000 */
	struct ssdfs_signature magic;

/* 0x0008 */
	__le16 sequence_id;
#define SSDFS_MAPTBL_CACHE_ZLIB_COMPR	(1 << 0)
#define SSDFS_MAPTBL_CACHE_LZO_COMPR	(1 << 1)
	__le16 flags;
	__le16 items_count;
	__le16 bytes_count;

/* 0x0010 */
	__le64 start_leb;
	__le64 end_leb;

/* 0x0020 */
} __attribute__((packed));

/*
 * struct ssdfs_maptbl_cache_peb_state - PEB state descriptor
 * @consistency: PEB state consistency type
 * @state: PEB's state
 * @flags: PEB's flags
 * @shared_peb_index: index of external shared destination PEB
 *
 * The mapping table cache is the copy of content of mapping
 * table for some type of PEBs. If the mapping table cache and
 * the mapping table contain the same content for the PEB then
 * the PEB state record is consistent. Otherwise, the PEB state
 * record is inconsistent. For example, the inconsistency takes
 * place if a PEB state record was modified in the mapping table
 * cache during the destruction of the mapping table.
 */
struct ssdfs_maptbl_cache_peb_state {
/* 0x0000 */
	__le8 consistency;
	__le8 state;
	__le8 flags;
	__le8 shared_peb_index;

/* 0x0004 */
} __attribute__((packed));

/* PEB state consistency type */
enum {
	SSDFS_PEB_STATE_UNKNOWN,
	SSDFS_PEB_STATE_CONSISTENT,
	SSDFS_PEB_STATE_INCONSISTENT,
	SSDFS_PEB_STATE_PRE_DELETED,
	SSDFS_PEB_STATE_MAX
};

#define SSDFS_MAPTBL_CACHE_HDR_SIZE \
	(sizeof(struct ssdfs_maptbl_cache_header))
#define SSDFS_LEB2PEB_PAIR_SIZE \
	(sizeof(struct ssdfs_leb2peb_pair))
#define SSDFS_PEB_STATE_SIZE \
	(sizeof(struct ssdfs_maptbl_cache_peb_state))

#define SSDFS_LEB2PEB_PAIR_PER_FRAGMENT(fragment_size) \
	((fragment_size - SSDFS_MAPTBL_CACHE_HDR_SIZE - \
				SSDFS_PEB_STATE_SIZE) / \
	 (SSDFS_LEB2PEB_PAIR_SIZE + SSDFS_PEB_STATE_SIZE))

/*
 * struct ssdfs_btree_node_header - btree's node header
 * @magic: magic signature + revision
 * @check: metadata checksum
 * @height: btree node's height
 * @log_node_size: log2(node size)
 * @log_index_area_size: log2(index area size)
 * @type: btree node type
 * @flags: btree node flags
 * @index_area_offset: offset of index area in bytes
 * @index_count: count of indexes in index area
 * @index_size: size of index in bytes
 * @min_item_size: min size of item in bytes
 * @max_item_size: max possible size of item in bytes
 * @items_capacity: capacity of items in the node
 * @start_hash: start hash value
 * @end_hash: end hash value
 * @create_cno: create checkpoint
 * @node_id: node identification number
 * @item_area_offset: offset of items area in bytes
 */
struct ssdfs_btree_node_header {
/* 0x0000 */
	struct ssdfs_signature magic;

/* 0x0008 */
	struct ssdfs_metadata_check check;

/* 0x0010 */
	__le8 height;
	__le8 log_node_size;
	__le8 log_index_area_size;
	__le8 type;

/* 0x0014 */
#define SSDFS_BTREE_NODE_HAS_INDEX_AREA		(1 << 0)
#define SSDFS_BTREE_NODE_HAS_ITEMS_AREA		(1 << 1)
#define SSDFS_BTREE_NODE_HAS_L1TBL		(1 << 2)
#define SSDFS_BTREE_NODE_HAS_L2TBL		(1 << 3)
#define SSDFS_BTREE_NODE_HAS_HASH_TBL		(1 << 4)
#define SSDFS_BTREE_NODE_PRE_ALLOCATED		(1 << 5)
#define SSDFS_BTREE_NODE_FLAGS_MASK		0x3F
	__le16 flags;
	__le16 index_area_offset;

/* 0x0018 */
	__le16 index_count;
	__le8 index_size;
	__le8 min_item_size;
	__le16 max_item_size;
	__le16 items_capacity;

/* 0x0020 */
	__le64 start_hash;
	__le64 end_hash;

/* 0x0030 */
	__le64 create_cno;
	__le32 node_id;
	__le32 item_area_offset;

/* 0x0040 */
} __attribute__((packed));

/* Index of btree node in node's items sequence */
#define SSDFS_BTREE_NODE_HEADER_INDEX	(0)

/* Btree node types */
enum {
	SSDFS_BTREE_NODE_UNKNOWN_TYPE,
	SSDFS_BTREE_ROOT_NODE,
	SSDFS_BTREE_INDEX_NODE,
	SSDFS_BTREE_HYBRID_NODE,
	SSDFS_BTREE_LEAF_NODE,
	SSDFS_BTREE_NODE_TYPE_MAX
};

#define SSDFS_DENTRIES_PAGES_PER_NODE_MAX		(32)
#define SSDFS_DENTRIES_INDEX_BMAP_SIZE \
	((((SSDFS_DENTRIES_PAGES_PER_NODE_MAX * PAGE_SIZE) / \
	  sizeof(struct ssdfs_btree_index_key)) + BITS_PER_LONG) / BITS_PER_BYTE)
#define SSDFS_RAW_DENTRIES_BMAP_SIZE \
	((((SSDFS_DENTRIES_PAGES_PER_NODE_MAX * PAGE_SIZE) / \
	  sizeof(struct ssdfs_dir_entry)) + BITS_PER_LONG) / BITS_PER_BYTE)
#define SSDFS_DENTRIES_BMAP_SIZE \
	(SSDFS_DENTRIES_INDEX_BMAP_SIZE + SSDFS_RAW_DENTRIES_BMAP_SIZE)

/*
 * struct ssdfs_dentries_btree_node_header - directory entries node's header
 * @node: generic btree node's header
 * @parent_ino: parent inode number
 * @dentries_count: count of allocated dentries in the node
 * @inline_names: count of dentries with inline names
 * @flags: dentries node's flags
 * @free_space: free space of the node in bytes
 * @lookup_table: table for clustering search in the node
 *
 * The @lookup_table has goal to provide the way of clustering
 * the dentries in the node with the goal to speed-up the search.
 */
struct ssdfs_dentries_btree_node_header {
/* 0x0000 */
	struct ssdfs_btree_node_header node;

/* 0x0040 */
	__le64 parent_ino;

/* 0x0048 */
	__le16 dentries_count;
	__le16 inline_names;
	__le16 flags;
	__le16 free_space;

/* 0x0050 */
#define SSDFS_DENTRIES_BTREE_LOOKUP_TABLE_SIZE		(22)
	__le64 lookup_table[SSDFS_DENTRIES_BTREE_LOOKUP_TABLE_SIZE];

/* 0x0100 */
} __attribute__((packed));

#define SSDFS_SHARED_DICT_PAGES_PER_NODE_MAX		(32)
#define SSDFS_SHARED_DICT_INDEX_BMAP_SIZE \
	((((SSDFS_SHARED_DICT_PAGES_PER_NODE_MAX * PAGE_SIZE) / \
	  sizeof(struct ssdfs_btree_index_key)) + BITS_PER_LONG) / BITS_PER_BYTE)
#define SSDFS_RAW_SHARED_DICT_BMAP_SIZE \
	(((SSDFS_SHARED_DICT_PAGES_PER_NODE_MAX * PAGE_SIZE) / \
	  SSDFS_DENTRY_INLINE_NAME_MAX_LEN) / BITS_PER_BYTE)
#define SSDFS_SHARED_DICT_BMAP_SIZE \
	(SSDFS_SHARED_DICT_INDEX_BMAP_SIZE + SSDFS_RAW_SHARED_DICT_BMAP_SIZE)

/*
 * struct ssdfs_shdict_search_key - generalized search key
 * @name.hash_lo: low hash32 value
 * @name.hash_hi: tail hash of the name
 * @range.prefix_len: prefix length in bytes
 * @range.start_index: starting index into lookup table2
 * @range.reserved: private part of concrete structure
 *
 * This key is generalized version of the first part of any
 * item in lookup1, lookup2 and hash tables. This structure
 * is needed for the generic way of making search in all
 * tables.
 */
struct ssdfs_shdict_search_key {
/* 0x0000 */
	union {
		__le32 hash_lo;
		__le32 hash_hi;
	} name __attribute__((packed));

/* 0x0004 */
	union {
		__le8 prefix_len;
		__le16 start_index;
		__le32 reserved;
	} range __attribute__((packed));

/* 0x0008 */
} __attribute__((packed));

/*
 * struct ssdfs_shdict_ltbl1_item - shared dictionary lookup table1 item
 * @hash_lo: low hash32 value
 * @start_index: starting index into lookup table2
 * @range_len: number of items in the range of lookup table2
 *
 * The header of shared dictionary node contains the lookup table1.
 * This table is responsible for clustering the items in lookup
 * table2. The @hash_lo is hash32 of the first part of the name.
 * The length of the first part is the inline name length.
 */
struct ssdfs_shdict_ltbl1_item {
/* 0x0000 */
	__le32 hash_lo;
	__le16 start_index;
	__le16 range_len;

/* 0x0008 */
} __attribute__((packed));

/*
 * struct ssdfs_shdict_ltbl2_item - shared dictionary lookup table2 item
 * @hash_lo: low hash32 value
 * @prefix_len: prefix length in bytes
 * @str_count: count of strings in the range
 * @hash_index: index of the hash in the hash table
 *
 * The lookup table2 is located at the end of the node. It begins from
 * the bottom and is growing in the node's beginning direction.
 * Every item of the lookup table2 describes a position of the starting
 * keyword of a name. The goal of such descriptor is to describe
 * the starting position of the deduplicated keyword that is shared by
 * several following names. But the keyword is used only in the beginning
 * of the sequence because the rest of the names are represented by
 * suffixes only (for example, the sequence of names "absurd, abcissa,
 * abacus" can be reprensented by "abacuscissasurd" deduplicated range
 * of names).
 */
struct ssdfs_shdict_ltbl2_item {
/* 0x0000 */
	__le32 hash_lo;
	__le8 prefix_len;
	__le8 str_count;
	__le16 hash_index;

/* 0x0008 */
} __attribute__((packed));

/*
 * struct ssdfs_shdict_htbl_item - shared dictionary hash table item
 * @hash_hi: tail hash of the name
 * @str_offset: offset in bytes to string
 * @str_len: string length
 * @type: string type
 *
 * The hash table contains descriptors of all strings in
 * string area. The @str_offset is the offset in bytes from
 * the items (strings) area's beginning.
 */
struct ssdfs_shdict_htbl_item {
/* 0x0000 */
	__le32 hash_hi;
	__le16 str_offset;
	__le8 str_len;
	__le8 type;

/* 0x0008 */
} __attribute__((packed));

/* Name string types */
enum {
	SSDFS_UNKNOWN_NAME_TYPE,
	SSDFS_NAME_PREFIX,
	SSDFS_NAME_SUFFIX,
	SSDFS_FULL_NAME,
	SSDFS_NAME_TYPE_MAX
};

/*
 * struct ssdfs_shared_dict_area - area descriptor
 * @offset: area offset in bytes
 * @size: area size in bytes
 * @free_space: free space in bytes
 * @items_count: count of items in area
 */
struct ssdfs_shared_dict_area {
/* 0x0000 */
	__le16 offset;
	__le16 size;
	__le16 free_space;
	__le16 items_count;

/* 0x0008 */
} __attribute__((packed));

/*
 * struct ssdfs_shared_dictionary_node_header - shared dictionary node header
 * @node: generic btree node's header
 * @str_area: string area descriptor
 * @hash_table: hash table descriptor
 * @lookup_table2: lookup2 table descriptor
 * @flags: private flags
 * @lookup_table1_items: number of valid items in the lookup1 table
 * @lookup_table1: lookup1 table
 */
struct ssdfs_shared_dictionary_node_header {
/* 0x0000 */
	struct ssdfs_btree_node_header node;

/* 0x0040 */
	struct ssdfs_shared_dict_area str_area;

/* 0x0048 */
	struct ssdfs_shared_dict_area hash_table;

/* 0x0050 */
	struct ssdfs_shared_dict_area lookup_table2;

/* 0x0058 */
	__le16 flags;
	__le16 lookup_table1_items;
	__le32 reserved2;

/* 0x0060 */
#define SSDFS_SHDIC_LTBL1_SIZE		(20)
	struct ssdfs_shdict_ltbl1_item lookup_table1[SSDFS_SHDIC_LTBL1_SIZE];

/* 0x0100 */
} __attribute__((packed));

#define SSDFS_EXTENT_PAGES_PER_NODE_MAX		(32)
#define SSDFS_EXTENT_INDEX_BMAP_SIZE \
	((((SSDFS_EXTENT_PAGES_PER_NODE_MAX * PAGE_SIZE) / \
	  sizeof(struct ssdfs_btree_index_key)) + BITS_PER_LONG) / BITS_PER_BYTE)
#define SSDFS_RAW_EXTENT_BMAP_SIZE \
	((((SSDFS_EXTENT_PAGES_PER_NODE_MAX * PAGE_SIZE) / \
	  sizeof(struct ssdfs_raw_fork)) + BITS_PER_LONG) / BITS_PER_BYTE)
#define SSDFS_EXTENT_MAX_BMAP_SIZE \
	(SSDFS_EXTENT_INDEX_BMAP_SIZE + SSDFS_RAW_EXTENT_BMAP_SIZE)

/*
 * ssdfs_extents_btree_node_header - extents btree node's header
 * @node: generic btree node's header
 * @parent_ino: parent inode number
 * @blks_count: count of blocks in all valid extents
 * @forks_count: count of forks in the leaf node or sub-tree (hybrid/index node)
 * @allocated_extents: count of allocated extents in all forks
 * @valid_extents: count of valid extents
 * @max_extent_blks: maximal number of blocks in one extent
 * @lookup_table: table for clustering search in the node
 *
 * The @lookup_table has goal to provide the way of clustering
 * the forks in the node with the goal to speed-up the search.
 */
struct ssdfs_extents_btree_node_header {
/* 0x0000 */
	struct ssdfs_btree_node_header node;

/* 0x0040 */
	__le64 parent_ino;
	__le64 blks_count;

/* 0x0050 */
	__le32 forks_count;
	__le32 allocated_extents;
	__le32 valid_extents;
	__le32 max_extent_blks;

/* 0x0060 */
#define SSDFS_EXTENTS_BTREE_LOOKUP_TABLE_SIZE		(20)
	__le64 lookup_table[SSDFS_EXTENTS_BTREE_LOOKUP_TABLE_SIZE];

/* 0x0100 */
} __attribute__((packed));

#define SSDFS_XATTRS_PAGES_PER_NODE_MAX		(32)
#define SSDFS_XATTRS_INDEX_BMAP_SIZE \
	((((SSDFS_XATTRS_PAGES_PER_NODE_MAX * PAGE_SIZE) / \
	  sizeof(struct ssdfs_btree_index_key)) + BITS_PER_LONG) / BITS_PER_BYTE)
#define SSDFS_RAW_XATTRS_BMAP_SIZE \
	((((SSDFS_XATTRS_PAGES_PER_NODE_MAX * PAGE_SIZE) / \
	  sizeof(struct ssdfs_xattr_entry)) + BITS_PER_LONG) / BITS_PER_BYTE)
#define SSDFS_XATTRS_BMAP_SIZE \
	(SSDFS_XATTRS_INDEX_BMAP_SIZE + SSDFS_RAW_XATTRS_BMAP_SIZE)

/*
 * struct ssdfs_xattrs_btree_node_header - xattrs node's header
 * @node: generic btree node's header
 * @parent_ino: parent inode number
 * @xattrs_count: count of allocated xattrs in the node
 * @flags: xattrs node's flags
 * @free_space: free space of the node in bytes
 * @lookup_table: table for clustering search in the node
 *
 * The @lookup_table has goal to provide the way of clustering
 * the xattrs in the node with the goal to speed-up the search.
 */
struct ssdfs_xattrs_btree_node_header {
/* 0x0000 */
	struct ssdfs_btree_node_header node;

/* 0x0040 */
	__le64 parent_ino;

/* 0x0048 */
	__le16 xattrs_count;
	__le16 reserved;
	__le16 flags;
	__le16 free_space;

/* 0x0050 */
#define SSDFS_XATTRS_BTREE_LOOKUP_TABLE_SIZE		(22)
	__le64 lookup_table[SSDFS_XATTRS_BTREE_LOOKUP_TABLE_SIZE];

/* 0x0100 */
} __attribute__((packed));

/*
 * struct ssdfs_index_area - index area info
 * @start_hash: start hash value
 * @end_hash: end hash value
 */
struct ssdfs_index_area {
/* 0x0000 */
	__le64 start_hash;
	__le64 end_hash;

/* 0x0010 */
} __attribute__((packed));

#define SSDFS_INODE_BMAP_SIZE		(0xA0)

/*
 * struct ssdfs_inodes_btree_node_header -inodes btree node's header
 * @node: generic btree node's header
 * @inodes_count: count of inodes in the node
 * @valid_inodes: count of valid inodes in the node
 * @index_area: index area info (hybrid node)
 * @bmap: bitmap of valid/invalid inodes in the node
 */
struct ssdfs_inodes_btree_node_header {
/* 0x0000 */
	struct ssdfs_btree_node_header node;

/* 0x0040 */
	__le16 inodes_count;
	__le16 valid_inodes;
	__le8 reserved1[0xC];

/* 0x0050 */
	struct ssdfs_index_area index_area;

/* 0x0060 */
	__le8 bmap[SSDFS_INODE_BMAP_SIZE];

/* 0x0100 */
} __attribute__((packed));

/*
 * struct ssdfs_snapshot_rule_info - snapshot rule info
 * @mode: snapshot mode (READ-ONLY|READ-WRITE)
 * @type: snapshot type (PERIODIC|ONE-TIME)
 * @expiration: snapshot expiration time (WEEK|MONTH|YEAR|NEVER)
 * @frequency: taking snapshot frequency (SYNCFS|HOUR|DAY|WEEK)
 * @snapshots_threshold max number of simultaneously available snapshots
 * @snapshots_number: current number of created snapshots
 * @ino: root object inode ID
 * @uuid: snapshot UUID
 * @name: snapshot rule name
 * @name_hash: name hash
 * @last_snapshot_cno: latest snapshot checkpoint
 */
struct ssdfs_snapshot_rule_info {
/* 0x0000 */
	__le8 mode;
	__le8 type;
	__le8 expiration;
	__le8 frequency;
	__le16 snapshots_threshold;
	__le16 snapshots_number;

/* 0x0008 */
	__le64 ino;

/* 0x0010 */
	__le8 uuid[SSDFS_UUID_SIZE];

/* 0x0020 */
	char name[SSDFS_MAX_SNAP_RULE_NAME_LEN];

/* 0x0030 */
	__le64 name_hash;
	__le64 last_snapshot_cno;

/* 0x0040 */
} __attribute__((packed));

/* Snapshot mode */
enum {
	SSDFS_UNKNOWN_SNAPSHOT_MODE,
	SSDFS_READ_ONLY_SNAPSHOT,
	SSDFS_READ_WRITE_SNAPSHOT,
	SSDFS_SNAPSHOT_MODE_MAX
};

#define SSDFS_READ_ONLY_MODE_STR	"READ_ONLY"
#define SSDFS_READ_WRITE_MODE_STR	"READ_WRITE"

/* Snapshot type */
enum {
	SSDFS_UNKNOWN_SNAPSHOT_TYPE,
	SSDFS_ONE_TIME_SNAPSHOT,
	SSDFS_PERIODIC_SNAPSHOT,
	SSDFS_SNAPSHOT_TYPE_MAX
};

#define SSDFS_ONE_TIME_TYPE_STR		"ONE-TIME"
#define SSDFS_PERIODIC_TYPE_STR		"PERIODIC"

/* Snapshot expiration */
enum {
	SSDFS_UNKNOWN_EXPIRATION_POINT,
	SSDFS_EXPIRATION_IN_WEEK,
	SSDFS_EXPIRATION_IN_MONTH,
	SSDFS_EXPIRATION_IN_YEAR,
	SSDFS_NEVER_EXPIRED,
	SSDFS_EXPIRATION_POINT_MAX
};

#define SSDFS_WEEK_EXPIRATION_POINT_STR		"WEEK"
#define SSDFS_MONTH_EXPIRATION_POINT_STR	"MONTH"
#define SSDFS_YEAR_EXPIRATION_POINT_STR		"YEAR"
#define SSDFS_NEVER_EXPIRED_STR			"NEVER"

/* Snapshot creation frequency */
enum {
	SSDFS_UNKNOWN_FREQUENCY,
	SSDFS_SYNCFS_FREQUENCY,
	SSDFS_HOUR_FREQUENCY,
	SSDFS_DAY_FREQUENCY,
	SSDFS_WEEK_FREQUENCY,
	SSDFS_MONTH_FREQUENCY,
	SSDFS_CREATION_FREQUENCY_MAX
};

#define SSDFS_SYNCFS_FREQUENCY_STR		"SYNCFS"
#define SSDFS_HOUR_FREQUENCY_STR		"HOUR"
#define SSDFS_DAY_FREQUENCY_STR			"DAY"
#define SSDFS_WEEK_FREQUENCY_STR		"WEEK"
#define SSDFS_MONTH_FREQUENCY_STR		"MONTH"

#define SSDFS_INFINITE_SNAPSHOTS_NUMBER		U16_MAX
#define SSDFS_UNDEFINED_SNAPSHOTS_NUMBER	(0)

/*
 * struct ssdfs_snapshot_rules_header - snapshot rules table's header
 * @magic: magic signature
 * @item_size: snapshot rule's size in bytes
 * @flags: various flags
 * @items_count: number of snapshot rules in table
 * @items_capacity: capacity of the snaphot rules table
 * @area_size: size of table in bytes
 */
struct ssdfs_snapshot_rules_header {
/* 0x0000 */
	__le32 magic;
	__le16 item_size;
	__le16 flags;

/* 0x0008 */
	__le16 items_count;
	__le16 items_capacity;
	__le32 area_size;

/* 0x0010 */
	__le8 padding[0x10];

/* 0x0020 */
} __attribute__((packed));

/*
 * struct ssdfs_snapshot - snapshot info
 * @magic: magic signature of snapshot
 * @mode: snapshot mode (READ-ONLY|READ-WRITE)
 * @expiration: snapshot expiration time (WEEK|MONTH|YEAR|NEVER)
 * @flags: snapshot's flags
 * @name: snapshot name
 * @uuid: snapshot UUID
 * @create_time: snapshot's timestamp
 * @create_cno: snapshot's checkpoint
 * @ino: root object inode ID
 * @name_hash: name hash
 */
struct ssdfs_snapshot {
/* 0x0000 */
	__le16 magic;
	__le8 mode : 4;
	__le8 expiration : 4;
	__le8 flags;
	char name[SSDFS_MAX_SNAPSHOT_NAME_LEN];

/* 0x0010 */
	__le8 uuid[SSDFS_UUID_SIZE];

/* 0x0020 */
	__le64 create_time;
	__le64 create_cno;

/* 0x0030 */
	__le64 ino;
	__le64 name_hash;

/* 0x0040 */
} __attribute__((packed));

/* snapshot flags */
#define SSDFS_SNAPSHOT_HAS_EXTERNAL_STRING	(1 << 0)
#define SSDFS_SNAPSHOT_FLAGS_MASK		0x1

/*
 * struct ssdfs_peb2time_pair - PEB to timestamp pair
 * @peb_id: PEB ID
 * @last_log_time: last log creation time
 */
struct ssdfs_peb2time_pair {
/* 0x0000 */
	__le64 peb_id;
	__le64 last_log_time;

/* 0x0010 */
} __attribute__((packed));

/*
 * struct ssdfs_peb2time_set - PEB to timestamp set
 * @magic: magic signature of set
 * @pairs_count: number of valid pairs in the set
 * @create_time: create time of the first PEB in pair set
 * @array: array of PEB to timestamp pairs
 */
struct ssdfs_peb2time_set {
/* 0x0000 */
	__le16 magic;
	__le8 pairs_count;
	__le8 padding[0x5];

/* 0x0008 */
	__le64 create_time;

/* 0x0010 */
#define SSDFS_PEB2TIME_ARRAY_CAPACITY		(3)
	struct ssdfs_peb2time_pair array[SSDFS_PEB2TIME_ARRAY_CAPACITY];

/* 0x0040 */
} __attribute__((packed));

/*
 * union ssdfs_snapshot_item - snapshot item
 * @magic: magic signature
 * @snapshot: snapshot info
 * @peb2time: PEB to timestamp set
 */
union ssdfs_snapshot_item {
/* 0x0000 */
	__le16 magic;
	struct ssdfs_snapshot snapshot;
	struct ssdfs_peb2time_set peb2time;

/* 0x0040 */
} __attribute__((packed));

#define SSDFS_SNAPSHOTS_PAGES_PER_NODE_MAX		(32)
#define SSDFS_SNAPSHOTS_INDEX_BMAP_SIZE \
	((((SSDFS_SNAPSHOTS_PAGES_PER_NODE_MAX * PAGE_SIZE) / \
	  sizeof(struct ssdfs_btree_index_key)) + BITS_PER_LONG) / BITS_PER_BYTE)
#define SSDFS_RAW_SNAPSHOTS_BMAP_SIZE \
	((((SSDFS_SNAPSHOTS_PAGES_PER_NODE_MAX * PAGE_SIZE) / \
	  sizeof(struct ssdfs_snapshot_info)) + BITS_PER_LONG) / BITS_PER_BYTE)
#define SSDFS_SNAPSHOTS_BMAP_SIZE \
	(SSDFS_SNAPSHOTS_INDEX_BMAP_SIZE + SSDFS_RAW_SNAPSHOTS_BMAP_SIZE)

/*
 * struct ssdfs_snapshots_btree_node_header - snapshots node's header
 * @node: generic btree node's header
 * @snapshots_count: snapshots count in the node
 * @lookup_table: table for clustering search in the node
 *
 * The @lookup_table has goal to provide the way of clustering
 * the snapshots in the node with the goal to speed-up the search.
 */
struct ssdfs_snapshots_btree_node_header {
/* 0x0000 */
	struct ssdfs_btree_node_header node;

/* 0x0040 */
	__le32 snapshots_count;
	__le8 padding[0x0C];

/* 0x0050 */
#define SSDFS_SNAPSHOTS_BTREE_LOOKUP_TABLE_SIZE		(22)
	__le64 lookup_table[SSDFS_SNAPSHOTS_BTREE_LOOKUP_TABLE_SIZE];

/* 0x0100 */
} __attribute__((packed));

/*
 * struct ssdfs_shared_extent - shared extent
 * @fingerprint: fingerprint of shared extent
 * @extent: position of the extent on volume
 * @fingerprint_len: length of fingerprint
 * @fingerprint_type: type of fingerprint
 * @flags: various flags
 * @ref_count: reference counter of shared extent
 */
struct ssdfs_shared_extent {
/* 0x0000 */
#define SSDFS_FINGERPRINT_LENGTH_MAX	(32)
	__le8 fingerprint[SSDFS_FINGERPRINT_LENGTH_MAX];

/* 0x0020 */
	struct ssdfs_raw_extent extent;

/* 0x0030 */
	__le8 fingerprint_len;
	__le8 fingerprint_type;
	__le16 flags;
	__le8 padding[0x4];

/* 0x0038 */
	__le64 ref_count;

/* 0x0040 */
} __attribute__((packed));

#define SSDFS_SHEXTREE_PAGES_PER_NODE_MAX		(32)
#define SSDFS_SHEXTREE_INDEX_BMAP_SIZE \
	((((SSDFS_SHEXTREE_PAGES_PER_NODE_MAX * PAGE_SIZE) / \
	  sizeof(struct ssdfs_btree_index_key)) + BITS_PER_LONG) / BITS_PER_BYTE)
#define SSDFS_RAW_SHEXTREE_BMAP_SIZE \
	((((SSDFS_SHEXTREE_PAGES_PER_NODE_MAX * PAGE_SIZE) / \
	  sizeof(struct ssdfs_shared_extent)) + BITS_PER_LONG) / BITS_PER_BYTE)
#define SSDFS_SHEXTREE_BMAP_SIZE \
	(SSDFS_SHEXTREE_INDEX_BMAP_SIZE + SSDFS_RAW_SHEXTREE_BMAP_SIZE)

/*
 * struct ssdfs_shextree_node_header - shared extents btree node's header
 * @node: generic btree node's header
 * @shared_extents: number of shared extents in the node
 * @lookup_table: table for clustering search in the node
 *
 * The @lookup_table has goal to provide the way of clustering
 * the shared extents in the node with the goal to speed-up the search.
 */
struct ssdfs_shextree_node_header {
/* 0x0000 */
	struct ssdfs_btree_node_header node;

/* 0x0040 */
	__le32 shared_extents;
	__le8 padding[0x0C];

/* 0x0050 */
#define SSDFS_SHEXTREE_LOOKUP_TABLE_SIZE		(22)
	__le64 lookup_table[SSDFS_SHEXTREE_LOOKUP_TABLE_SIZE];

/* 0x0100 */
} __attribute__((packed));

#define SSDFS_INVEXTREE_PAGES_PER_NODE_MAX		(32)
#define SSDFS_INVEXTREE_INDEX_BMAP_SIZE \
	((((SSDFS_INVEXTREE_PAGES_PER_NODE_MAX * PAGE_SIZE) / \
	  sizeof(struct ssdfs_btree_index_key)) + BITS_PER_LONG) / BITS_PER_BYTE)
#define SSDFS_RAW_INVEXTREE_BMAP_SIZE \
	((((SSDFS_INVEXTREE_PAGES_PER_NODE_MAX * PAGE_SIZE) / \
	  sizeof(struct ssdfs_raw_extent)) + BITS_PER_LONG) / BITS_PER_BYTE)
#define SSDFS_INVEXTREE_BMAP_SIZE \
	(SSDFS_INVEXTREE_INDEX_BMAP_SIZE + SSDFS_RAW_INVEXTREE_BMAP_SIZE)

/*
 * struct ssdfs_invextree_node_header - invalidated extents btree node's header
 * @node: generic btree node's header
 * @extents_count: number of invalidated extents in the node
 * @lookup_table: table for clustering search in the node
 *
 * The @lookup_table has goal to provide the way of clustering
 * the invalidated extents in the node with the goal to speed-up the search.
 */
struct ssdfs_invextree_node_header {
/* 0x0000 */
	struct ssdfs_btree_node_header node;

/* 0x0040 */
	__le32 extents_count;
	__le8 padding[0x0C];

/* 0x0050 */
#define SSDFS_INVEXTREE_LOOKUP_TABLE_SIZE		(22)
	__le64 lookup_table[SSDFS_INVEXTREE_LOOKUP_TABLE_SIZE];

/* 0x0100 */
}  __attribute__((packed));

#endif /* _SSDFS_ABI_H */
