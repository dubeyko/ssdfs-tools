#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause-Clear
#
# Executed inside the VM (as root): check fsck.ssdfs on
#   (1) freshly created volume (mkfs -> fsck);
#   (2) volume after mount -> I/O -> umount (fsck).
#
# Usage: guest-fsck.sh <device> <log dir>
#
# Environment:
#   MKFS_OPTS - options of mkfs.ssdfs
#   FSCK_OPTS - options of fsck.ssdfs (default: -n -f -v)
#   TOOLS_DIR - ssdfs-tools build (default: this source tree)
#
# fsck debug output: <log dir>/fsck-{fresh,after-io}-debug.log
#

DEV="$1"
LOG_DIR="$2"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TOOLS="${TOOLS_DIR:-$(cd "$SCRIPT_DIR/../.." && pwd)}/sbin"
MKFS="$TOOLS/mkfs.ssdfs/mkfs.ssdfs"
FSCK="$TOOLS/fsck.ssdfs/fsck.ssdfs"
RECOVERFS="$TOOLS/recoverfs.ssdfs/recoverfs.ssdfs"
MNT=/tmp/ssdfs-mnt
MKFS_OPTS="${MKFS_OPTS:-}"
FSCK_OPTS="${FSCK_OPTS:--n -f -v}"
FAILED=0

step() {
	echo
	echo "===== STEP: $1 ====="
}

run_fsck() {
	local tag="$1"
	local rc

	echo "+ $FSCK $FSCK_OPTS $DEV"
	timeout 300 "$FSCK" $FSCK_OPTS "$DEV"
	rc=$?
	echo "FSCK [$tag]: rc $rc"
	[ $rc -ne 0 ] && FAILED=1

	timeout 600 "$FSCK" $FSCK_OPTS -d "$DEV" \
		> "$LOG_DIR/fsck-$tag-debug.log" 2>&1
	echo "FSCK-DEBUG [$tag]: rc $?, $(wc -l < "$LOG_DIR/fsck-$tag-debug.log") lines"
}

echo "kernel: $(uname -r), device: $DEV"
test -b "$DEV" || { echo "SSDFS-TEST: FAIL (no device)"; exit 1; }
mkdir -p "$MNT"

step "mkfs"
"$MKFS" -f $MKFS_OPTS "$DEV" || { echo "SSDFS-TEST: FAIL at mkfs"; exit 1; }

step "fsck (fresh volume)"
run_fsck fresh

step "mount, I/O, umount"
if mount -t ssdfs "$DEV" "$MNT"; then
	mkdir -p "$MNT/dir1/dir2"
	for size in 1 4096 65536 1048576; do
		dd if=/dev/urandom of="$MNT/file-$size" bs="$size" count=1 \
			status=none
	done
	for i in $(seq 1 50); do
		echo "small file $i" > "$MNT/dir1/small-$i"
	done
	sync
	umount "$MNT" || echo "MOUNT-IO: umount failed"
	echo "MOUNT-IO: done"

	step "fsck (after I/O)"
	run_fsck after-io

	step "recoverfs (after I/O)"
	rm -rf /tmp/ssdfs-recoverfs-out
	mkdir -p /tmp/ssdfs-recoverfs-out
	timeout 600 "$RECOVERFS" -d "$DEV" /tmp/ssdfs-recoverfs-out \
		> "$LOG_DIR/recoverfs-after-io-debug.log" 2>&1
	rc=$?
	echo "RECOVERFS [after-io]: rc $rc"
	grep -E "FILE CREATED|\[00[0-9]\]" \
		"$LOG_DIR/recoverfs-after-io-debug.log" | \
		sed 's/.*v\.[0-9.]*: //'
	ls -la /tmp/ssdfs-recoverfs-out
	[ $rc -ne 0 ] && FAILED=1
else
	echo "MOUNT-IO: mount failed, skip fsck after I/O"
fi

dmesg > "$LOG_DIR/dmesg.log" 2>&1

if [ $FAILED -eq 0 ]; then
	echo "SSDFS-TEST: PASS"
else
	echo "SSDFS-TEST: FAIL (fsck reported problems)"
fi
