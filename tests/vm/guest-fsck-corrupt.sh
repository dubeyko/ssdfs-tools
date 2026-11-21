#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause-Clear
#
# Executed inside the VM (as root): fsck.ssdfs corruption tests.
#
# The volume is created once by mkfs.ssdfs. Every test case restores
# the pristine metadata, corrupts one field of the base snapshot
# segment's log (see ssdfs-corrupt.py) and checks:
#   (1) exit code of fsck.ssdfs;
#   (2) the diagnostic message of the corrupted item (debug output);
#   (3) the number of corrupted items of base snapshot segment;
#   (4) the explanation of the corruption (fsck output, report level 3).
#
# Usage: guest-fsck-corrupt.sh <device> <log dir>
#
# Environment:
#   MKFS_OPTS - options of mkfs.ssdfs
#   TOOLS_DIR - ssdfs-tools build (default: this source tree)
#   CASES     - space separated list of cases (default: all)
#
# fsck debug output of every case: <log dir>/corrupt/<case>.log
#

DEV="$1"
LOG_DIR="$2"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TOOLS="${TOOLS_DIR:-$(cd "$SCRIPT_DIR/../.." && pwd)}/sbin"
MKFS="$TOOLS/mkfs.ssdfs/mkfs.ssdfs"
FSCK="$TOOLS/fsck.ssdfs/fsck.ssdfs"
RECOVERFS="$TOOLS/recoverfs.ssdfs/recoverfs.ssdfs"
DUMP="$TOOLS/dump.ssdfs/dump.ssdfs"
CORRUPT="python3 -I $SCRIPT_DIR/ssdfs-corrupt.py"
MKFS_OPTS="${MKFS_OPTS:-}"
FSCK_OPTS="-n -f -R 3"
BACKUP=/tmp/ssdfs-pristine.img
BACKUP_MB=128
OUT="$LOG_DIR/corrupt"

PASSED=0
FAILED=0
FAILED_CASES=""

EXIT_OK=0
EXIT_UNCORRECTED=4
EXIT_OPERATIONAL=8
EXIT_USAGE=16

# case | expected exit code | expected message | expected corrupted items |
#   expected lines of corruption explanation, separated by ';'
#   ("-" means: don't check)
TESTS=(
	"footer-magic|4|corrupted footer's signature|1|log footer: 1 of 27 items corrupted;signature: corrupted"
	"footer-csum|4|incorrect footer's checksum|1|log footer: 1 of 27 items corrupted;checksum: corrupted"
	"footer-version|4|unexpected on-disk layout version|1|log footer: 1 of 27 items corrupted;on-disk layout version: corrupted"
	"footer-nsegs|4|incorrect segment number|1|log footer: 1 of 27 items corrupted;segments count: corrupted"
	"footer-free-pages|4|incorrect free pages|1|log footer: 1 of 27 items corrupted;free pages: corrupted"
	"footer-timestamp|4|corrupted create_time|-|log footer: 2 of 27 items corrupted;log timestamp: corrupted;PEB creation time: corrupted"
	"footer-cno|4|corrupted cno|1|log footer: 1 of 27 items corrupted;log checkpoint: corrupted"
	"footer-flags|4|corrupted volume state flags|1|log footer: 1 of 27 items corrupted;volume state flags: corrupted"
	"footer-state|4|corrupted file system state|1|log footer: 1 of 27 items corrupted;file system state: corrupted"
	"footer-errors|4|corrupted errors behavior|1|log footer: 1 of 27 items corrupted;errors behavior: corrupted"
	"footer-features|4|corrupted features|1|log footer: 1 of 27 items corrupted;feature flags: corrupted"
	"footer-uuid|4|footer's UUID is different from header's one|1|log footer: 1 of 27 items corrupted;volume UUID: corrupted"
	"footer-cur-segs|4|corrupted current segment|1|log footer: 1 of 27 items corrupted;current segments: corrupted"
	"footer-migration-thld|4|corrupted migration threshold|1|log footer: 1 of 27 items corrupted;migration threshold: corrupted"
	"footer-blkbmap-opts|4|corrupted block bitmap options|1|log footer: 1 of 27 items corrupted;block bitmap options: corrupted"
	"footer-blk2off-opts|4|corrupted offset translation table options|1|log footer: 1 of 27 items corrupted;offset translation table options: corrupted"
	"footer-user-data-opts|4|corrupted user data options|1|log footer: 1 of 27 items corrupted;user data options: corrupted"
	"footer-open-zones|4|corrupted open zones|1|log footer: 1 of 27 items corrupted;open zones: corrupted"
	"footer-root-folder|4|corrupted root folder|1|log footer: 1 of 27 items corrupted;root folder inode: corrupted"
	"footer-inodes-btree|4|corrupted b-tree descriptor|1|log footer: 1 of 27 items corrupted;inodes b-tree descriptor: corrupted"
	"footer-log-bytes|4|incorrect log_bytes|1|log footer: 1 of 27 items corrupted;log size in bytes: corrupted"
	"footer-log-flags|4|corrupted footer's log flags|1|log footer: 1 of 27 items corrupted;log flags: corrupted"
	"footer-peb-create-time|4|footer's peb_create_time|1|log footer: 1 of 27 items corrupted;PEB creation time: corrupted"
	"header-relation-peb|4|corrupted relation_peb|1|log header: 1 of 41 items corrupted;relation PEB: corrupted"
	"header-csum|4|incorrect checksum|-|log header: 1 of 41 items corrupted;checksum: corrupted"
	"header-log-erasesize|4|incorrect seg_size|4|log header: 4 of 41 items corrupted;segment size: corrupted;megabytes per PEB: corrupted;PEBs per segment: corrupted;create threads per segment: corrupted"
	"header-log-segsize|4|incorrect pebs_per_seg|5|log header: 4 of 41 items corrupted;PEBs per segment: corrupted;segment bitmap descriptor: corrupted;mapping table descriptor: corrupted;create threads per segment: corrupted;log footer: 1 of 27 items corrupted;segments count: corrupted"
	"header-pebs-per-seg|4|incorrect pebs_per_seg|2|log header: 2 of 41 items corrupted;PEBs per segment: corrupted;create threads per segment: corrupted"
	"header-log-erasesize-max|4|incorrect pages_per_peb|7|log header: 7 of 41 items corrupted;erase size: corrupted;segment size: corrupted;megabytes per PEB: corrupted;PEBs per segment: corrupted;superblock PEBs: corrupted;log pages: corrupted;create threads per segment: corrupted"
	"header-log-segsize-overflow|4|incorrect pages_per_seg|4|log header: 4 of 41 items corrupted;segment size: corrupted;PEBs per segment: corrupted;create threads per segment: corrupted;relation PEB: corrupted"
	"header-segbmap-flags|4|corrupted segbmap's flags|1|log header: 1 of 41 items corrupted;segment bitmap descriptor: corrupted"
	"header-segbmap-bytes|4|corrupted bytes_count|1|log header: 1 of 41 items corrupted;segment bitmap descriptor: corrupted"
	"header-segbmap-extent-type|4|corrupted segbmap extent|1|log header: 1 of 41 items corrupted;segment bitmap descriptor: corrupted"
	"header-segbmap-extent-overlap|4|intersected segbmap extents|1|log header: 1 of 41 items corrupted;segment bitmap descriptor: corrupted"
	"header-maptbl-flags|4|corrupted maptbl's flags|1|log header: 1 of 41 items corrupted;mapping table descriptor: corrupted"
	"header-maptbl-lebs|4|corrupted mapping table|1|log header: 1 of 41 items corrupted;mapping table descriptor: corrupted"
	"header-maptbl-extent-len|4|corrupted maptbl extents|1|log header: 1 of 41 items corrupted;mapping table descriptor: corrupted"
	"header-magic|4|-|-|base snapshot segment: NOT FOUND"
)

restore() {
	dd if="$BACKUP" of="$DEV" bs=1M conv=notrunc,fsync status=none
}

result() {
	local name="$1" verdict="$2" details="$3"

	if [ "$verdict" = "PASS" ]; then
		PASSED=$((PASSED + 1))
	else
		FAILED=$((FAILED + 1))
		FAILED_CASES+=" $name"
	fi

	printf "CORRUPT-TEST %-24s %s %s\n" "$name" "$verdict" "$details"
}

# run_fsck <name> <log>: returns exit code of fsck
run_fsck() {
	timeout 300 "$FSCK" $FSCK_OPTS -d "$DEV" > "$2" 2>&1
}

check_case() {
	local name="$1" exp_rc="$2" exp_msg="$3" exp_items="$4" exp_explain="$5"
	local log="$OUT/$name.log"
	local rc items problems=""

	restore

	if [ "$name" != "clean" ]; then
		$CORRUPT "$DEV" "$name" > "$OUT/$name.corrupt" 2>&1 || {
			result "$name" FAIL "(corruption tool: $(cat "$OUT/$name.corrupt"))"
			return
		}
	fi

	run_fsck "$name" "$log"
	rc=$?

	items=$(grep -oE "is_base_snapshot_segment_corrupted\(\): finished: state [^,]*, checked [0-9]+, corrupted [0-9]+" \
		"$log" | grep -oE "corrupted [0-9]+$" | awk '{ print $2 }')

	[ "$rc" -ne "$exp_rc" ] && problems+=" rc $rc (expected $exp_rc)"

	if [ "$exp_msg" != "-" ] && ! grep -qF "$exp_msg" "$log"; then
		problems+=" no message '$exp_msg'"
	fi

	if [ "$exp_items" != "-" ] && [ "${items:-none}" != "$exp_items" ]; then
		problems+=" corrupted items ${items:-none} (expected $exp_items)"
	fi

	if [ "$exp_explain" = "none" ]; then
		grep -qF "Corruption details" "$log" && \
			problems+=" unexpected corruption details"
	elif [ "$exp_explain" != "-" ]; then
		local line
		IFS=';' read -ra lines <<< "$exp_explain"
		for line in "${lines[@]}"; do
			grep -qF "$line" "$log" || \
				problems+=" no explanation '$line'"
		done
	fi

	if [ -z "$problems" ]; then
		result "$name" PASS "(rc $rc, corrupted items ${items:-n/a})"
	else
		result "$name" FAIL "($(echo $problems))"
	fi
}

# check_recoverfs <name> <corruption case or "clean"/"none"> <found|not-found>
#   recoverfs.ssdfs has to find (or not) a valid PEB on the volume
check_recoverfs() {
	local name="recoverfs-$1" corrupt="$2" expected="$3"
	local log="$OUT/$name.log" outdir=/tmp/ssdfs-recoverfs-out
	local rc found

	if [ "$corrupt" != "none" ]; then
		restore
		if [ "$corrupt" != "clean" ]; then
			$CORRUPT "$DEV" "$corrupt" > "$OUT/$name.corrupt" 2>&1 || {
				result "$name" FAIL "(corruption tool: $(cat "$OUT/$name.corrupt"))"
				return
			}
		fi
	fi

	rm -rf "$outdir"
	mkdir -p "$outdir"
	timeout 300 "$RECOVERFS" -d "$DEV" "$outdir" > "$log" 2>&1
	rc=$?

	if grep -q "unable to find any valid PEB" "$log"; then
		found=not-found
	elif grep -qP "\[002\]\t\[SUCCESS\]" "$log"; then
		found=found
	else
		found=unknown
	fi

	if [ "$found" = "found" ] && [ $rc -ne 0 ]; then
		result "$name" FAIL "(valid PEB found, rc $rc, expected 0)"
	elif [ "$found" = "$expected" ]; then
		result "$name" PASS "(valid PEB $found, rc $rc)"
	else
		result "$name" FAIL "(valid PEB $found, expected $expected, rc $rc)"
	fi
}

echo "kernel: $(uname -r), device: $DEV"
test -b "$DEV" || { echo "SSDFS-TEST: FAIL (no device)"; exit 1; }
mkdir -p "$OUT"

echo
echo "===== STEP: mkfs ====="
"$MKFS" -f $MKFS_OPTS "$DEV" > "$OUT/mkfs.log" 2>&1 || {
	echo "SSDFS-TEST: FAIL at mkfs"
	exit 1
}
dd if="$DEV" of="$BACKUP" bs=1M count=$BACKUP_MB status=none

echo
echo "===== STEP: corruption tests ====="

if [ -z "${CASES:-}" ]; then
	check_case clean $EXIT_OK "No corruptions have been detected" 0 none
fi

for t in "${TESTS[@]}"; do
	IFS='|' read -r name rc msg items explain <<< "$t"
	if [ -n "${CASES:-}" ] && [[ " $CASES " != *" $name "* ]]; then
		continue
	fi
	check_case "$name" "$rc" "$msg" "$items" "$explain"
done

if [ -z "${CASES:-}" ]; then
	check_recoverfs clean clean found
	check_recoverfs header-csum header-csum found
	check_recoverfs header-magic header-magic found

	# usage error: no device
	"$FSCK" > "$OUT/usage.log" 2>&1
	rc=$?
	if [ $rc -eq $EXIT_USAGE ]; then
		result usage PASS "(rc $rc)"
	else
		result usage FAIL "(rc $rc, expected $EXIT_USAGE)"
	fi

	# usage error: invalid report level
	"$FSCK" -n -R 4 "$DEV" > "$OUT/usage-report-level.log" 2>&1
	rc=$?
	if [ $rc -eq $EXIT_USAGE ]; then
		result usage-report-level PASS "(rc $rc)"
	else
		result usage-report-level FAIL "(rc $rc, expected $EXIT_USAGE)"
	fi

	# zeroed first 4KB: segment header of PEB #0 (base snapshot
	# segment) is destroyed but the rest of metadata is intact.
	# The superblock segments, segment bitmap, and mapping table
	# have to be found by means of volume header of another PEB,
	# so only base snapshot segment has to be reported as not found.
	restore
	name=zeroed-first-4kb
	log="$OUT/$name.log"
	dd if=/dev/zero of="$DEV" bs=4096 count=1 conv=notrunc,fsync \
		status=none
	run_fsck $name "$log"
	rc=$?
	problems=""
	[ $rc -eq $EXIT_UNCORRECTED ] ||
		problems+=" rc $rc (expected $EXIT_UNCORRECTED)"
	grep -qx "  base snapshot segment: NOT FOUND" "$log" ||
		problems+=" no explanation of missing base snapshot"
	! grep -q "not found:" "$log" ||
		problems+=" other metadata are not found"
	! grep -q "not searched" "$log" ||
		problems+=" some metadata have not been searched"
	if [ -z "$problems" ]; then
		result $name PASS "(rc $rc)"
	else
		result $name FAIL "($problems )"
	fi

	# another file system over SSDFS volume: mkfs.ext4 overwrites
	# PEB #0 (ext4 superblock is located at the same offset as
	# SSDFS segment header) but stale SSDFS metadata stay in other
	# PEBs; fsck.ssdfs has to report no SSDFS volume without
	# suggesting the recovery
	if command -v mkfs.ext4 > /dev/null 2>&1; then
		restore
		name=ext4-over-ssdfs
		log="$OUT/$name.log"
		mkfs.ext4 -q -F -E nodiscard "$DEV" > "$OUT/$name.mkfs" 2>&1
		echo N | timeout 300 "$FSCK" -d "$DEV" > "$log" 2>&1
		rc=$?
		problems=""
		[ $rc -eq $EXIT_OPERATIONAL ] ||
			problems+=" rc $rc (expected $EXIT_OPERATIONAL)"
		grep -q "ext4 file system has been detected" "$log" ||
			problems+=" no message of ext4 detection"
		grep -q "No SSDFS file system has been detected" "$log" ||
			problems+=" no message of missing SSDFS volume"
		! grep -qE "is corrupted|Would you like" "$log" ||
			problems+=" recovery has been suggested"
		if [ -z "$problems" ]; then
			result $name PASS "(rc $rc)"
		else
			result $name FAIL "($problems )"
		fi
	else
		echo "CORRUPT-TEST ext4-over-ssdfs          SKIP (no mkfs.ext4)"
	fi

	# no SSDFS volume: wipe the metadata area
	dd if=/dev/zero of="$DEV" bs=1M count=$BACKUP_MB conv=fsync \
		status=none
	run_fsck no-volume "$OUT/no-volume.log"
	rc=$?
	if [ $rc -eq $EXIT_OPERATIONAL ]; then
		result no-volume PASS "(rc $rc)"
	else
		result no-volume FAIL "(rc $rc, expected $EXIT_OPERATIONAL)"
	fi

	check_recoverfs no-volume none not-found

	# zeroed beginning of the volume: PEB #0 and the first PEBs
	# (probed by detection logic) contain zeros but some PEB far
	# from the beginning keeps valid segment header. The volume
	# with 128KB erase size is created and the mapping table's PEB #8
	# is copied into PEB #313 (instead of segment written by kernel).
	# fsck.ssdfs has to scan the whole device and to report
	# the corrupted volume; recoverfs.ssdfs and dump.ssdfs have to
	# find valid PEB.
	name=zeroed-head
	log="$OUT/$name.log"
	if "$MKFS" -f -e 128KB -s 128KB "$DEV" > "$OUT/$name.mkfs" 2>&1; then
		dd if="$DEV" of="$DEV" bs=128K skip=8 seek=313 count=1 \
			conv=notrunc,fsync status=none
		dd if=/dev/zero of="$DEV" bs=128K count=313 \
			conv=notrunc,fsync status=none
		run_fsck $name "$log"
		rc=$?
		problems=""
		[ $rc -eq $EXIT_UNCORRECTED ] ||
			problems+=" rc $rc (expected $EXIT_UNCORRECTED)"
		grep -q "scan the whole device" "$log" ||
			problems+=" no message of device scan"
		grep -q "base snapshot segment: NOT FOUND" "$log" ||
			problems+=" no explanation of missing base snapshot"
		grep -qx "  not found: superblock segments" "$log" ||
			problems+=" no message of missing superblock segments"
		! grep -q "No SSDFS file system has been detected" "$log" ||
			problems+=" volume has not been detected"
		if [ -z "$problems" ]; then
			result $name PASS "(rc $rc)"
		else
			result $name FAIL "($problems )"
		fi

		check_recoverfs $name none found

		# dump.ssdfs has to find the valid PEB and show
		# the volume geometry
		timeout 300 "$DUMP" -g "$DEV" > "$OUT/dump-$name.log" 2>&1
		rc=$?
		if [ $rc -eq 0 ] &&
		   grep -q "PEB: 131072 bytes" "$OUT/dump-$name.log"; then
			result dump-$name PASS "(rc $rc)"
		else
			result dump-$name FAIL "(rc $rc, no volume geometry)"
		fi
	else
		result $name FAIL "(mkfs.ssdfs -e 128KB -s 128KB failed)"
	fi
fi

dmesg > "$LOG_DIR/dmesg.log" 2>&1

echo
echo "CORRUPT-TESTS: passed $PASSED, failed $FAILED${FAILED_CASES:+ (failed:$FAILED_CASES)}"

if [ $FAILED -eq 0 ]; then
	echo "SSDFS-TEST: PASS"
else
	echo "SSDFS-TEST: FAIL"
fi
