## Makefile.am
## SPDX-License-Identifier: BSD-3-Clause-Clear

AM_CFLAGS = -Wall
AM_CPPFLAGS = -I$(top_srcdir)/include

LDADD = -luuid $(top_builddir)/lib/libssdfs.la

sbin_PROGRAMS = snapshotfs.ssdfs

snapshotfs_ssdfs_SOURCES = options.c snapshotfs.c snapshotfs.h
