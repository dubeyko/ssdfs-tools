## Makefile.am
## SPDX-License-Identifier: BSD-3-Clause-Clear

AM_CFLAGS = -Wall
AM_CPPFLAGS = -I$(top_srcdir)/include

LDADD = -luuid $(top_builddir)/lib/libssdfs.la

sbin_PROGRAMS = tune.ssdfs

tune_ssdfs_SOURCES = tunefs.h options.c tunefs.c
