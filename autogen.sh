#!/bin/sh
# SPDX-License-Identifier: BSD-3-Clause-Clear
#
# Run this before configure
#

die() {
    echo
    echo "Error: $*"
    exit 1
}

aclocal || die "aclocal failed"

autoheader || die "autoheader failed" 

libtoolize -c --force || die "libtoolize failed"

automake -a -c || die "automake failed"

echo
echo "Creating configure..."

autoconf || die "autoconf failed"

echo
echo "Now run './configure' and 'make' to compile ssdfs-utils."
