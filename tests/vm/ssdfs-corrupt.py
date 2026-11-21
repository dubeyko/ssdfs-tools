#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause-Clear
#
# Corrupt one field of the base snapshot segment's log (PEB 0) of SSDFS
# volume created by mkfs.ssdfs.
#
# Usage: ssdfs-corrupt.py <device> <case>
#        ssdfs-corrupt.py --list
#
# The segment header is located at offset 1024 of PEB 0 (after volume
# boot record). The log footer is located by the footer's descriptor of
# the segment header (offset inside of PEB 0). By default, the checksum
# of the modified structure is recalculated, so only the target item
# should be detected as corrupted.
#

import struct
import sys
import zlib

SEG_HDR_OFFSET = 1024
SEG_HDR_DESC_ARRAY = 0x418
LOG_FOOTER_INDEX = 10
DESC_SIZE = 16
CHECK_OFFSET = 0x8		# struct ssdfs_metadata_check in both structures

SSDFS_SUPER_MAGIC = 0x53734466
SSDFS_SEGMENT_HDR_MAGIC = 0x5348
SSDFS_LOG_FOOTER_MAGIC = 0x4C46

U64_MAX = 0xFFFFFFFFFFFFFFFF

# segment bitmap and mapping table headers in volume header
# (struct ssdfs_volume_header at offset 0 of segment header)
SEGBMAP_HDR = 0x0B0
SEGBMAP_EXTENTS = 0x0C0
MAPTBL_HDR = 0x140
MAPTBL_EXTENTS = 0x170
EXTENT_SIZE = 16		# struct ssdfs_meta_area_extent
EXTENT_LEN = 0x8
EXTENT_TYPE = 0xC
EXTENT_CHAINS = 2		# main and copy chains
SSDFS_PEB_EXTENT_TYPE = 2


def extent_offset(extents, row, chain=0):
	return extents + (row * EXTENT_CHAINS + chain) * EXTENT_SIZE


def copy_extent(extents, src_row, dst_row):
	"""Make the extent of dst_row the same as the extent of src_row."""
	def edit(buf):
		src = extent_offset(extents, src_row)
		dst = extent_offset(extents, dst_row)
		buf[dst:dst + EXTENT_SIZE] = buf[src:src + EXTENT_SIZE]
	return edit


# case: (structure, offset, format, value or callable, fix_csum)
#   value callable: new value = f(old value)
#   format None: value is callable that edits the whole structure
CASES = {
	# log footer (struct ssdfs_log_footer)
	"footer-magic":		("footer", 0x004, "<H", 0, False),
	"footer-csum":		("footer", 0x458, "<B", lambda v: v ^ 0xFF, False),
	"footer-version":	("footer", 0x006, "<B", lambda v: v + 1, True),
	"footer-nsegs":		("footer", 0x010, "<Q", lambda v: v + 1, True),
	"footer-free-pages":	("footer", 0x018, "<Q", 1 << 62, True),
	"footer-timestamp":	("footer", 0x400, "<Q", 1, True),
	"footer-cno":		("footer", 0x408, "<Q", U64_MAX, True),
	"footer-flags":		("footer", 0x030, "<I", lambda v: v | 0x80, True),
	"footer-state":		("footer", 0x034, "<H", 0x8, True),
	"footer-errors":	("footer", 0x036, "<H", 0, True),
	"footer-features":	("footer", 0x048, "<Q", 1, True),
	"footer-uuid":		("footer", 0x050, "<B", lambda v: v ^ 0x11, True),
	"footer-cur-segs":	("footer", 0x070, "<Q", 1 << 40, True),
	"footer-migration-thld": ("footer", 0x098, "<H", 0, True),
	"footer-blkbmap-opts":	("footer", 0x09E, "<B", 0x20, True),
	"footer-blk2off-opts":	("footer", 0x0A0, "<H", lambda v: v | 0x80, True),
	"footer-user-data-opts": ("footer", 0x0A6, "<B", 0x20, True),
	"footer-open-zones":	("footer", 0x0AC, "<I", 0xFFFFFFFF, True),
	"footer-root-folder":	("footer", 0x0B0, "<H", 0, True),
	"footer-inodes-btree":	("footer", 0x200, "<I", 0, True),
	"footer-log-bytes":	("footer", 0x410, "<I", 1, True),
	"footer-log-flags":	("footer", 0x414, "<I", lambda v: v | 0x80000000, True),
	"footer-peb-create-time": ("footer", 0x450, "<Q", lambda v: v - 1, True),
	# segment header (struct ssdfs_segment_header)
	"header-magic":		("header", 0x004, "<H", 0, False),
	"header-csum":		("header", 0x7F0, "<B", lambda v: v ^ 0xFF, False),
	"header-relation-peb":	("header", 0x4F0, "<Q", 5, True),
	# volume geometry (struct ssdfs_volume_header): geometry fields
	# depend on each other, so several items are detected as corrupted;
	# detection takes the volume geometry from PEB #0, if the geometry
	# of PEB #0 looks sane (otherwise, from another PEB)
	"header-log-erasesize":	("header", 0x011, "<B", lambda v: v + 1, True),
	"header-log-segsize":	("header", 0x012, "<B", lambda v: v + 1, True),
	"header-pebs-per-seg":	("header", 0x016, "<H", lambda v: v + 1, True),
	# extreme values: erase block of 2GB (bigger than segment) and
	# segment size that doesn't fit into 32 bits
	"header-log-erasesize-max": ("header", 0x011, "<B", 31, True),
	"header-log-segsize-overflow": ("header", 0x012, "<B", 32, True),
	# segment bitmap header (struct ssdfs_segbmap_sb_header)
	"header-segbmap-flags":	("header", SEGBMAP_HDR + 0xC, "<H",
				 lambda v: v | 0x2, True),
	"header-segbmap-bytes":	("header", SEGBMAP_HDR + 0x8, "<I",
				 lambda v: v + 1, True),
	"header-segbmap-extent-type": ("header",
				 extent_offset(SEGBMAP_EXTENTS, 0) + EXTENT_TYPE,
				 "<H", SSDFS_PEB_EXTENT_TYPE, True),
	"header-segbmap-extent-overlap": ("header", extent_offset(SEGBMAP_EXTENTS, 1),
				 None, copy_extent(SEGBMAP_EXTENTS, 0, 1), True),
	# mapping table header (struct ssdfs_maptbl_sb_header)
	"header-maptbl-flags":	("header", MAPTBL_HDR + 0x24, "<H",
				 lambda v: v | 0x10, True),
	"header-maptbl-lebs":	("header", MAPTBL_HDR + 0x10, "<Q",
				 lambda v: v + 1, True),
	"header-maptbl-extent-len": ("header",
				 extent_offset(MAPTBL_EXTENTS, 0) + EXTENT_LEN,
				 "<I", lambda v: v + 1, True),
}


def read_at(f, offset, size):
	f.seek(offset)
	data = f.read(size)
	if len(data) != size:
		sys.exit("short read at %d" % offset)
	return bytearray(data)


def fix_csum(buf):
	nbytes, flags = struct.unpack_from("<HH", buf, CHECK_OFFSET)
	if nbytes == 0 or nbytes > len(buf):
		sys.exit("unexpected check.bytes %d" % nbytes)
	struct.pack_into("<I", buf, CHECK_OFFSET + 4, 0)
	csum = ~zlib.crc32(bytes(buf[:nbytes])) & 0xFFFFFFFF
	struct.pack_into("<I", buf, CHECK_OFFSET + 4, csum)


def main():
	if len(sys.argv) == 2 and sys.argv[1] == "--list":
		print("\n".join(CASES))
		return

	if len(sys.argv) != 3 or sys.argv[2] not in CASES:
		sys.exit("usage: %s <device> <case> | --list" % sys.argv[0])

	dev, case = sys.argv[1], sys.argv[2]
	what, offset, fmt, value, recalc = CASES[case]

	with open(dev, "r+b", buffering=0) as f:
		hdr = read_at(f, SEG_HDR_OFFSET, 2048)
		common, key = struct.unpack_from("<IH", hdr, 0)
		if common != SSDFS_SUPER_MAGIC or key != SSDFS_SEGMENT_HDR_MAGIC:
			sys.exit("PEB 0 hasn't valid segment header")

		desc = SEG_HDR_DESC_ARRAY + LOG_FOOTER_INDEX * DESC_SIZE
		footer_offset, footer_size = struct.unpack_from("<II", hdr, desc)

		if what == "header":
			base, buf = SEG_HDR_OFFSET, hdr
		else:
			buf = read_at(f, footer_offset, 2048)
			common, key = struct.unpack_from("<IH", buf, 0)
			if common != SSDFS_SUPER_MAGIC or \
			   key != SSDFS_LOG_FOOTER_MAGIC:
				sys.exit("footer at %d isn't struct ssdfs_log_footer" %
					 footer_offset)
			base = footer_offset

		if fmt is None:
			old = bytes(buf[offset:offset + EXTENT_SIZE]).hex()
			value(buf)
			new = bytes(buf[offset:offset + EXTENT_SIZE]).hex()
		else:
			old = struct.unpack_from(fmt, buf, offset)[0]
			new = value(old) if callable(value) else value
			mask = (1 << (8 * struct.calcsize(fmt))) - 1
			new &= mask
			struct.pack_into(fmt, buf, offset, new)
			old, new = "%#x" % old, "%#x" % new

		if recalc:
			fix_csum(buf)

		f.seek(base)
		f.write(bytes(buf))

	print("CORRUPT %s: %s+%#x %s -> %s%s" %
	      (case, what, offset, old, new,
	       " (checksum recalculated)" if recalc else ""))


if __name__ == "__main__":
	main()
