## Makefile.am
## SPDX-License-Identifier: BSD-3-Clause-Clear

AM_CFLAGS = -Wall
AM_CPPFLAGS = -I$(top_srcdir)/include

LDADD = $(top_builddir)/lib/libssdfs.la

sbin_PROGRAMS = fsck.ssdfs

fsck_ssdfs_SOURCES = fsck.h options.c detect_file_system.c \
			check_file_system.c fsck.c
