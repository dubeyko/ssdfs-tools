//SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * sbin/mkfs.ssdfs/initial_snapshot.c - initial snapshot creation functionality.
 *
 * Copyright (c) 2014-2020 HGST, a Western Digital Company.
 *              http://www.hgst.com/
 *
 * HGST Confidential
 * (C) Copyright 2014-2020, HGST, Inc., All rights reserved.
 *
 * Created by HGST, San Jose Research Center, Storage Architecture Group
 * Authors: Vyacheslav Dubeyko <slava@dubeyko.com>
 *
 * Acknowledgement: Cyril Guyot <Cyril.Guyot@wdc.com>
 *                  Zvonimir Bandic <Zvonimir.Bandic@wdc.com>
 */

#include "mkfs.h"

/************************************************************************
 *                 Initial snapshot creation functionality              *
 ************************************************************************/

int snap_mkfs_allocation_policy(struct ssdfs_volume_layout *layout,
				int *segs)
{
	int seg_state = SSDFS_DEDICATED_SEGMENT;

	*segs = 1;
	layout->meta_array[SSDFS_INITIAL_SNAPSHOT].segs_count = *segs;
	layout->meta_array[SSDFS_INITIAL_SNAPSHOT].seg_state = seg_state;

	SSDFS_DBG(layout->env.show_debug, "initial snapshot segs %d\n", *segs);
	return seg_state;
}

int snap_mkfs_prepare(struct ssdfs_volume_layout *layout)
{
	return reserve_segments(layout, SSDFS_INITIAL_SNAPSHOT);
}

int snap_mkfs_define_layout(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_metadata_desc *desc;
	struct ssdfs_peb_content *peb_desc;
	struct ssdfs_extent_desc *extent;
	int seg_index;
	int peb_index;
	int segs_count;
	int err;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	desc = &layout->meta_array[SSDFS_INITIAL_SNAPSHOT];
	segs_count = desc->segs_count;

	if (segs_count <= 0 || segs_count > 1) {
		SSDFS_ERR("invalid segs_count %d\n", segs_count);
		return -ERANGE;
	}

	if (desc->start_seg_index >= layout->segs_capacity) {
		SSDFS_ERR("start_seg_index %d >= segs_capacity %d\n",
			  desc->start_seg_index,
			  layout->segs_capacity);
		return -ERANGE;
	}

	if ((layout->segs_count + segs_count) > layout->segs_capacity) {
		SSDFS_ERR("not enough space for commit: "
			  "segs_count %d, request %d, capacity %d\n",
			  layout->segs_count, segs_count,
			  layout->segs_capacity);
		return -E2BIG;
	}

	seg_index = desc->start_seg_index;
	layout->segs[seg_index].pebs_count = 1;
	BUG_ON(layout->segs[seg_index].pebs_count >
		layout->segs[seg_index].pebs_capacity);
	peb_index = 0;
	peb_desc = &layout->segs[seg_index].pebs[peb_index];

	err = set_extent_start_offset(layout, peb_desc, SSDFS_SEG_HEADER);
	if (err) {
		SSDFS_ERR("fail to define extent's start offset: "
			  "err %d\n", err);
		return err;
	}

	extent = &peb_desc->extents[SSDFS_SEG_HEADER];
	extent->offset += SSDFS_RESERVED_VBR_SIZE;

	err = define_segment_header_layout(layout, seg_index, peb_index);
	if (err) {
		SSDFS_ERR("fail to define segment header's layout: "
			  "err %d\n", err);
		return err;
	}

	/* TODO: define payload's layout */

	err = set_extent_start_offset(layout, peb_desc, SSDFS_LOG_FOOTER);
	if (err) {
		SSDFS_ERR("fail to define extent's start offset: "
			  "err %d\n", err);
		return err;
	}

	err = define_log_footer_layout(layout, seg_index, peb_index);
	if (err) {
		SSDFS_ERR("fail to define segment footer's layout: "
			  "err %d\n", err);
		return err;
	}

	return 0;
}

int snap_mkfs_commit(struct ssdfs_volume_layout *layout)
{
	struct ssdfs_metadata_desc *desc;
	struct ssdfs_peb_content *peb_desc;
	u32 blks;
	int seg_index;
	int peb_index;
	int segs_count;
	int err;

	SSDFS_DBG(layout->env.show_debug, "layout %p\n", layout);

	desc = &layout->meta_array[SSDFS_INITIAL_SNAPSHOT];
	segs_count = desc->segs_count;
	seg_index = desc->start_seg_index;

	if (segs_count <= 0 || segs_count > 1) {
		SSDFS_ERR("invalid segs_count %d\n", segs_count);
		return -ERANGE;
	}

	if (desc->start_seg_index >= layout->segs_capacity) {
		SSDFS_ERR("start_seg_index %d >= segs_capacity %d\n",
			  desc->start_seg_index,
			  layout->segs_capacity);
		return -ERANGE;
	}

	peb_index = 0;
	peb_desc = &layout->segs[seg_index].pebs[peb_index];

	err = pre_commit_segment_header(layout, seg_index, peb_index,
					SSDFS_INITIAL_SNAPSHOT_SEG_TYPE);
	if (err)
		return err;

	/* TODO: commit payload */

	err = pre_commit_log_footer(layout, seg_index, peb_index);
	if (err)
		return err;

	blks = calculate_log_pages(layout, peb_desc);
	commit_log_footer(layout, seg_index, peb_index, blks);
	commit_segment_header(layout, seg_index, peb_index, blks);

	layout->segs_count += segs_count;
	return 0;
}
