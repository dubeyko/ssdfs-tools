## Makefile.am
## SPDX-License-Identifier: BSD-3-Clause-Clear

AM_CFLAGS = -Wall
AM_CPPFLAGS = -I$(top_srcdir)/include

LDADD = -lblkid $(top_builddir)/lib/libssdfs.la

sbin_PROGRAMS = fsck.ssdfs

fsck_ssdfs_SOURCES = fsck.h options.c detect_file_system.h detect_file_system.c \
			check_segment_header.c check_partial_log_header.c \
			check_log_footer.c check_file_system.h \
			check_file_system.c explain_corruption.c \
			recover_file_system.c \
			fsck.c
