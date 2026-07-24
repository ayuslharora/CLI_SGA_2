# Question 1 — Duplicate Submission Detection, Backup, and Reporting

## Setup

Sample submissions directory: `sample_submissions/` — 7 files (`s1.txt` .. `s6.c`, `s7_locked.txt`), where `s2.txt` is a byte-for-byte copy of `s1.txt`, `s4.txt` is a byte-for-byte copy of `s3.txt`, and `s7_locked.txt` has its permissions set to `000` to exercise the error-handling path (a real permission-denied read).

```
chmod 000 sample_submissions/s7_locked.txt
```
Removed all read/write/execute bits from the file so any later attempt to hash it fails with a genuine "Permission denied" error, which is what a corrupted or restricted student submission on a shared server would look like.

## Running the script

```
chmod +x dedup_backup.sh
./dedup_backup.sh sample_submissions backup
```
Made the script executable and ran it against the sample directory, with `backup` as the destination for unique files. Output: `Done. See report.txt and errors.log.` — the script deliberately keeps stdout to a single status line so all real detail goes to the report/error files instead of the terminal.

```
cat report.txt
```
Displayed the generated report. Observed:
```
Files processed : 7
Unique files    : 4
Duplicate files : 2
Files backed up : 4
```
This matches the sample data exactly: 7 files in, 1 unhashable (`s7_locked.txt`), 6 hashed, of which `s2`/`s4` are duplicates of `s1`/`s3`, leaving 4 unique files (`s1`, `s3`, `s5`, `s6`) all backed up.

```
cat errors.log
```
Displayed the separate error/duplicate log. Observed:
```
sha256sum: sample_submissions/s7_locked.txt: Permission denied
ERROR: could not read/hash 'sample_submissions/s7_locked.txt' (permission denied?)
DUPLICATE: 'sample_submissions/s2.txt' matches an earlier submission (sha256 03662fbc...)
DUPLICATE: 'sample_submissions/s4.txt' matches an earlier submission (sha256 4c4f5723...)
```
Confirms the permission error was captured separately from the report (not mixed into it), and both duplicates were correctly identified by content hash rather than filename.

```
ls backup/
```
Observed exactly `s1.txt s3.txt s5.txt s6.c` — one copy of each distinct submission, no duplicates copied twice.

## Commands, redirection, and file-handling techniques used

- **`find "$SUBMIT_DIR" -type f -print0`** — recursively lists only regular files, `-print0` NUL-delimits names so filenames containing spaces don't break the loop that reads them.
- **`sha256sum` / `shasum -a 256`** — content hashing rather than filename/size comparison, so a duplicate is detected even if a student renames their copy before resubmitting; the script picks whichever tool exists (`command -v`) so it runs unmodified on both Linux servers and macOS.
- **`sort -k1,1`** — sorts the hash/path list by hash so every submission with identical content becomes an adjacent block, turning duplicate detection into a simple "did the hash change from the previous line" check instead of an O(n²) comparison.
- **`cp --`** — copies only the first file seen for each hash into the backup directory; the `--` guards against a filename that starts with `-` being misread as an option.
- **Redirection (`>`, `>>`, `2>>`)** — `>` truncates `report.txt`/`hashes.txt` at the start of each run (a report should reflect the latest run, not accumulate); `2>>` appends every stderr from `find`, the hash command, and `cp` into `errors.log` so failures never pollute the report or the terminal; `>>` inside the script appends duplicate/permission notices to the same error log as they're discovered, one per event.
- **`mkdir -p`** — creates the backup directory (and any missing parents) without failing if it already exists, so the script is safely re-runnable.

## Files created during the lab

- `dedup_backup.sh` — the script
- `sample_submissions/` — synthetic input data (including one intentionally unreadable file)
- `hashes.txt`, `hashes.txt.sorted` — intermediate hash/path lists
- `report.txt` — processed/duplicate/backed-up counts
- `errors.log` — permission errors and duplicate notices, kept separate from the report
- `backup/` — one copy of each unique submission
