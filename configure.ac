#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# SPDX-License-Identifier: BSD-3-Clause-Clear

AC_PREREQ([2.69])
AC_INIT(ssdfs-utils, 4.68, slava@dubeyko.com)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([include/ssdfs_abi.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
LT_INIT
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB([blkid], [blkid_new_probe_from_filename],
		[AC_DEFINE(HAVE_LIBBLKID, 1,
			   [Define to 1 if you have the 'blkid' library (-lblkid).])],
		AC_MSG_ERROR([BLKID library not found]))
AC_CHECK_HEADERS([blkid/blkid.h])

AC_CHECK_LIB([uuid], [uuid_generate],
		[AC_DEFINE([HAVE_LIBUUID], 1,
			   [Define to 1 if you have the 'uuid' library (-luuid).])],
		[AC_MSG_ERROR([UUID library not found])])
AC_CHECK_HEADERS([uuid/uuid.h])

AC_CHECK_LIB([rt], [clock_gettime],
		[AC_DEFINE([HAVE_LIBRT], 1,
			   [Define to 1 if you have the 'rt' library (-lrt).])],
		[AC_MSG_ERROR([Realtime Extensions (RT) library not found])])
AC_CHECK_HEADERS([time.h])

AC_CHECK_LIB([z], [crc32],
		[AC_DEFINE([HAVE_LIBZ], 1,
			   [Define to 1 if you have the 'zlib' library (-lz).])],
		[AC_MSG_ERROR([ZLIB library not found])])
AC_CHECK_HEADERS([zlib.h])

AC_CHECK_LIB([lzo2], [lzo1x_1_compress],
		[AC_DEFINE([HAVE_LIBLZO2], 1,
			   [Define to 1 if you have the 'lzo2' library (-llzo2).])
		 HAVE_LZO2=yes],
		[AC_MSG_WARN([LZO2 library not found - LZO compression will be disabled])
		 HAVE_LZO2=no])
		 
if test "x$HAVE_LZO2" = "xyes"; then
	AC_CHECK_HEADERS([lzo/lzo1x.h], [],
		[AC_MSG_WARN([LZO2 headers not found - LZO compression will be disabled])
		 HAVE_LZO2=no])
fi

AM_CONDITIONAL([HAVE_LZO2], [test "x$HAVE_LZO2" = "xyes"])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h paths.h stdlib.h string.h sys/ioctl.h unistd.h])
AC_CHECK_HEADER([linux/blkzoned.h], [], [AC_MSG_ERROR([linux/blkzoned.h not found])])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_rdev])

# Checks for library functions.
AC_HEADER_MAJOR
AC_FUNC_MALLOC
AC_CHECK_FUNCS([clock_gettime memmove memset strerror])

AC_CONFIG_FILES([Makefile
                 include/Makefile
                 lib/Makefile
                 sbin/Makefile
                 sbin/dump.ssdfs/Makefile
                 sbin/fsck.ssdfs/Makefile
                 sbin/mkfs.ssdfs/Makefile
                 sbin/resize.ssdfs/Makefile
                 sbin/recoverfs.ssdfs/Makefile
                 sbin/snapshotfs.ssdfs/Makefile
                 sbin/tune.ssdfs/Makefile
                 sbin/test.ssdfs/Makefile
                 man/Makefile])
AC_OUTPUT
