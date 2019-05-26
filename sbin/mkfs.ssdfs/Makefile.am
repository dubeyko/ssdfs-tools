## Makefile.am
## SPDX-License-Identifier: BSD-3-Clause-Clear

AM_CFLAGS = -Wall
AM_CPPFLAGS = -I$(top_srcdir)/include

LDADD = -lblkid -luuid $(top_builddir)/lib/libssdfs.la

sbin_PROGRAMS = mkfs.ssdfs

mkfs_ssdfs_SOURCES = mkfs.h options.c common.c initial_snapshot.c \
			superblock_segment.c segment_bitmap.c \
			mapping_table.c mapping_table_cache.c mkfs.c
