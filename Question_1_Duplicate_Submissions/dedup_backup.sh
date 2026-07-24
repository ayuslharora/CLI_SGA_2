#!/usr/bin/env bash
#
# dedup_backup.sh
#
# Scans a directory of student submissions, detects duplicate files by
# content (not filename), backs up one copy of every unique submission,
# and produces a processing report. All errors go to a separate log
# instead of the report or stdout.
#
# Usage: ./dedup_backup.sh <submissions_dir> [backup_dir]

set -u

SUBMIT_DIR="${1:?Usage: $0 <submissions_dir> [backup_dir]}"
BACKUP_DIR="${2:-./backup}"
REPORT_FILE="report.txt"
ERROR_LOG="errors.log"
HASH_LIST="hashes.txt"

# Start each run with clean report/error/hash files (truncate, don't append)
: > "$ERROR_LOG"
: > "$HASH_LIST"

if [ ! -d "$SUBMIT_DIR" ]; then
    echo "ERROR: submissions directory '$SUBMIT_DIR' does not exist" >> "$ERROR_LOG"
    exit 1
fi

mkdir -p "$BACKUP_DIR" 2>>"$ERROR_LOG"

# Linux servers ship sha256sum; macOS ships shasum -a 256. Pick whichever
# is available so the script is portable across both.
if command -v sha256sum >/dev/null 2>&1; then
    HASH_CMD=(sha256sum)
else
    HASH_CMD=(shasum -a 256)
fi

total=0

# find ... -print0 / read -d '' handles filenames with spaces safely.
# Each file is hashed; unreadable files (e.g. permission-denied) are
# caught here and logged instead of aborting the whole run.
while IFS= read -r -d '' file; do
    total=$((total + 1))
    if hash_line=$("${HASH_CMD[@]}" "$file" 2>>"$ERROR_LOG"); then
        hash=$(printf '%s\n' "$hash_line" | awk '{print $1}')
        printf '%s  %s\n' "$hash" "$file" >> "$HASH_LIST"
    else
        echo "ERROR: could not read/hash '$file' (permission denied?)" >> "$ERROR_LOG"
    fi
done < <(find "$SUBMIT_DIR" -type f -print0 2>>"$ERROR_LOG")

# Sort by hash so identical-content files become adjacent lines, then
# walk the sorted list: the first file for a given hash is the unique
# copy that gets backed up; every later file with the same hash is a
# duplicate and is only logged, not copied again.
sort -k1,1 "$HASH_LIST" > "${HASH_LIST}.sorted"

unique=0
duplicate=0
backed_up=0
prev_hash=""

while read -r hash path; do
    if [ "$hash" != "$prev_hash" ]; then
        unique=$((unique + 1))
        if cp -- "$path" "$BACKUP_DIR/" 2>>"$ERROR_LOG"; then
            backed_up=$((backed_up + 1))
        else
            echo "ERROR: failed to back up '$path'" >> "$ERROR_LOG"
        fi
    else
        duplicate=$((duplicate + 1))
        echo "DUPLICATE: '$path' matches an earlier submission (sha256 $hash)" >> "$ERROR_LOG"
    fi
    prev_hash="$hash"
done < "${HASH_LIST}.sorted"

{
    echo "Submission Deduplication Report"
    echo "Generated: $(date)"
    echo "Source directory: $SUBMIT_DIR"
    echo "Backup directory: $BACKUP_DIR"
    echo "--------------------------------"
    echo "Files processed : $total"
    echo "Unique files    : $unique"
    echo "Duplicate files : $duplicate"
    echo "Files backed up : $backed_up"
} > "$REPORT_FILE"

echo "Done. See $REPORT_FILE and $ERROR_LOG."
