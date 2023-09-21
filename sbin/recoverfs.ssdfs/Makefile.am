## Makefile.am
## SPDX-License-Identifier: BSD-3-Clause-Clear

AM_CFLAGS = -Wall
AM_CPPFLAGS = -I$(top_srcdir)/include

LDADD = -lpthread $(top_builddir)/lib/libssdfs.la

sbin_PROGRAMS = recoverfs.ssdfs

recoverfs_ssdfs_SOURCES = options.c recoverfs.c recoverfs.h \
			  peb_processing.c file_synthesis.c \
			  snapshot.h delete_folder.c \
			  inline_files.c
