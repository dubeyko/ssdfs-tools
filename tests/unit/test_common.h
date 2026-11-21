/*
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * ssdfs-utils -- SSDFS file system utilities.
 *
 * tests/unit/test_common.h - common declarations of unit-tests.
 *
 * Copyright (c) 2026 Viacheslav Dubeyko <slava@dubeyko.com>
 *              http://www.ssdfs.org/
 * All rights reserved.
 *
 * Authors: Viacheslav Dubeyko <slava@dubeyko.com>
 */

#ifndef _SSDFS_UTILS_TEST_COMMON_H
#define _SSDFS_UTILS_TEST_COMMON_H

#include <stdio.h>

/*
 * Every unit-test is a standalone program executed by "make check".
 * Exit code: 0 - all cases passed, 1 - some case failed.
 */

static int ssdfs_test_passed;
static int ssdfs_test_failed;

#define SSDFS_TEST_CHECK(name, cond)					\
	do {								\
		if (cond) {						\
			ssdfs_test_passed++;				\
			printf("PASS: %s\n", name);			\
		} else {						\
			ssdfs_test_failed++;				\
			printf("FAIL: %s (%s:%d: %s)\n",		\
			       name, __FILE__, __LINE__, #cond);	\
		}							\
	} while (0)

static inline
int ssdfs_test_summary(const char *test_name)
{
	printf("%s: passed %d, failed %d\n",
	       test_name, ssdfs_test_passed, ssdfs_test_failed);
	return ssdfs_test_failed ? 1 : 0;
}

#endif /* _SSDFS_UTILS_TEST_COMMON_H */
