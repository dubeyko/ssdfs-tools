## Makefile.am
## SPDX-License-Identifier: BSD-3-Clause-Clear

AM_CFLAGS = -Wall
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/sbin/fsck.ssdfs

LDADD = -lblkid $(top_builddir)/lib/libssdfs.la

check_PROGRAMS = test_fsck_partial_log_header \
		 test_fsck_explain

TESTS = $(check_PROGRAMS)

test_fsck_partial_log_header_SOURCES = test_common.h \
			test_fsck_partial_log_header.c

test_fsck_explain_SOURCES = test_common.h \
			test_fsck_explain.c
