## Makefile.am
## SPDX-License-Identifier: BSD-3-Clause-Clear

noinst_LTLIBRARIES = libssdfs.la

libssdfs_la_SOURCES = ssdfs_common.c segbmap.c blkbmap.c \
			mtd_readwrite.c bdev_readwrite.c \
			zns_readwrite.c compression.c
libssdfs_la_CFLAGS = -Wall -fPIC
libssdfs_la_CPPFLAGS = -I$(top_srcdir)/include
libssdfs_la_LDFLAGS = -static
libssdfs_la_LIBADD = -lz -lrt
