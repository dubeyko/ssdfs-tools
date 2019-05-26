## Makefile.am
## SPDX-License-Identifier: BSD-3-Clause-Clear

AM_CFLAGS = -Wall
AM_CPPFLAGS = -I$(top_srcdir)/include

LDADD = $(top_builddir)/lib/libssdfs.la

sbin_PROGRAMS = dump.ssdfs

dump_ssdfs_SOURCES = dumpfs.h options.c common.c show_granularity.c \
			show_peb_dump.c show_raw_dump.c dumpfs.c
