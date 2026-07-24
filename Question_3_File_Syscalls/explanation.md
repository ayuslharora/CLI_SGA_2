# Question 3 — Secure File-Processing Utility with Linux/POSIX Syscalls

## Design

`struct Employee { int id; char name[50]; float salary; }` is a **fixed-size**
record (60 bytes on this system: 4-byte `int` + 50-byte `name` + 4-byte
`float`, plus 2 bytes of compiler padding to align `salary` — confirmed with
`sizeof`). Because every record occupies exactly the same number of bytes,
record `i` always lives at byte offset `i * sizeof(struct Employee)` in the
file. That fixed layout is what makes constant-time random access with
`lseek()` possible — the program never has to parse or scan the file to find
a record; it can compute the exact offset and jump straight there.

The program (`employee_records.c`) uses only `open()`, `read()`, `write()`,
`lseek()`, and `close()` from `<fcntl.h>`/`<unistd.h>` — no `fopen`/`fread`/
`fwrite`/`fseek` from stdio.

## Commands executed

### 1. Compile
```
$ cc -Wall -Wextra -o employee_records employee_records.c
```
**What I did:** Compiled the C source with both warning flags enabled to
catch any type or format mistakes.
**Observed:** No output at all — meaning zero warnings and zero errors. The
binary `employee_records` was produced in the same folder.

### 2. Run
```
$ ./employee_records
```
**What I did:** Executed the compiled program, which in one run: creates
`employees.dat`, writes 5 initial employee records, updates one record in
place, retrieves an arbitrary record directly, and prints a final dump.
**Observed:** The real terminal output (captured verbatim in `output.txt`)
showed:
- The 5 initial records (Alice 55000, Bob 48000, Charlie 62000, Diana 71000,
  Ethan 39000) printed correctly after the initial `write()` calls.
- The update step changed record index 2 (Charlie) from salary 62000.00 to
  85000.00 using only `lseek()` + `write()` on that one record.
- The random-access retrieval step fetched index 4 (Ethan, 39000.00)
  directly via `lseek()` + `read()`, without reading indices 0–3 first.
- The final full dump confirmed only Charlie's record changed
  (62000.00 → 85000.00); Alice, Bob, Diana, and Ethan were untouched,
  proving the update did not rewrite the whole file.

### 3. Inspect file size
```
$ ls -l employees.dat
```
**What I did:** Checked the resulting binary file's size on disk.
**Observed:** `-rw-r--r--@ 1 ayush staff 300 ... employees.dat`. Since
`sizeof(struct Employee)` is 60 bytes and there are 5 records, 5 × 60 = 300
bytes — exactly matching the file size on disk, confirming the file contains
precisely 5 fixed-size records and nothing else (no delimiters, no extra
metadata).

### 4. Inspect raw bytes
```
$ xxd employees.dat | head -20
```
**What I did:** Dumped the raw binary contents to visually confirm the
on-disk record layout (id as little-endian 4 bytes, name as a padded
ASCII/NUL-terminated 50-byte field, salary as a 4-byte IEEE-754 float),
and to confirm Charlie's record shows the updated salary bytes in place at
its original offset (index 2 × 60 = byte 120 / 0x78) rather than at the end
of the file.
**Observed:** The hex dump shows "Alice", "Bob", "Char[lie]", "Diana",
"Etha[n]" at evenly spaced 0x3C (60-byte) intervals, confirming the fixed
record stride, and the updated numeric bytes sit at Charlie's original
offset.

## How each syscall contributes

`open()` is the entry point that both creates the file and establishes the
access mode: `O_CREAT | O_TRUNC | O_RDWR` with mode `0644` means "create the
file if it doesn't exist, start from empty if it does, and allow both
reading and writing" — this single call is what lets the same file descriptor
be used later for both writing new records and reading/updating existing
ones, and its `0644` permission bits keep the file readable/writable only by
the owner and read-only for others, which matters for a "secure" utility.
`write()` is used twice for two different purposes: first to append the
initial batch of records sequentially right after creation, and later to
overwrite exactly one record's 60 bytes during an update — in both cases it
copies the in-memory `struct Employee` bytes directly to the file at the
descriptor's current position. `lseek()` is the key to efficiency: instead of
scanning the file from the start to find a record, the program computes
`offset = index * sizeof(struct Employee)` and calls
`lseek(fd, offset, SEEK_SET)`, which repositions the file's read/write cursor
in O(1) time regardless of file size — this is what turns an update into an
in-place operation and a retrieval into direct/random access rather than a
linear scan. `read()` then pulls exactly `sizeof(struct Employee)` bytes from
wherever `lseek()` last positioned the cursor, reconstructing a single record
in memory for inspection or verification. `close()` releases the file
descriptor back to the OS once all operations are done, flushing any
pending state and freeing the kernel resource — a well-behaved program does
not leak descriptors, which matters for a long-running or repeatedly-invoked
utility. Together, these five calls let the program treat the file as an
array of fixed-size records on disk: `open()` gets a handle with the right
permissions, `write()`/`read()` move record-sized chunks of data in and out,
`lseek()` provides constant-time addressing into that array so any single
record can be updated or retrieved without touching the rest of the file,
and `close()` cleans up — exactly the behavior described in the question
(create, write, update one record without rewriting everything, and retrieve
any record efficiently).

## Files created during this lab

- `employee_records.c` — the C source (low-level syscalls only).
- `employee_records` — the compiled binary (`cc -Wall -Wextra -o employee_records employee_records.c`).
- `employees.dat` — the binary data file produced by running the program (300 bytes = 5 × 60-byte records).
- `output.txt` — captured terminal output of the compile command, the program run, `ls -l employees.dat`, and `xxd employees.dat | head -20`.
- `explanation.md` — this file.
- `screenshots/README.txt` — instructions for the commands to screenshot manually for submission.
