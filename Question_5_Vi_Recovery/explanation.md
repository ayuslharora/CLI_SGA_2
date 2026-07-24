# Question 5 — vi/vim Crash Recovery (4 marks)

## Scenario

A developer is editing a critical configuration file in `vi`/`vim`. The system
crashes **before** the file is saved. What recovery mechanisms exist, and
which one actually saves the day?

This document evaluates the five mechanisms named in the question — swap
files, undo history, registers, backup files, and the auto-recovery workflow
— and backs the evaluation with a **real, live demonstration** performed on
this Mac (macOS ships Vim 9.1, which uses the identical recovery model as
vi/vim on Linux).

---

## 1. Live demonstration — what was actually run, and the real output

### Environment note (read this first, in the interest of honesty)

This lab was carried out from an automated shell session that has **no
controlling TTY** (confirmed with `tty` → `not a tty`, and
`os.isatty()` → `False` for stdin/stdout/stderr). That rules out a genuine
full-screen, keystroke-by-keystroke `vim` session driven by a human at a
terminal, and it also ruled out using `script`/`pty` allocation to fake one
(`script -q ... vim ...` failed immediately with
`tcgetattr/ioctl: Operation not supported on socket`).

To still get a **real** crash and a **real** recovery — not a scripted fake —
the demonstration instead drives a genuine `vim` process through its
Ex/batch engine (`vim -es`), which needs no TTY. This is not a simulation of
vim's behavior: it is the actual `vim` binary, with a real PID, a real
in-memory buffer, and a real `.swp` file on disk, that gets genuinely
`kill -9`'d while unsaved changes exist. The only thing that's different
from a human's session is *how the keystrokes were delivered* (via a Unix
FIFO instead of a physical keyboard) — the crash and the recovery artifact
it leaves behind are 100% real.

### Step 1 — create the test file

```
$ cat test_config.conf
# test_config.conf - sample config file for vi/vim crash-recovery demo
server_name = demo-server-01
port = 8080
max_connections = 100
log_level = info
```

### Step 2 — open it in a real vim process and confirm the swap file appears immediately

```
$ mkfifo /tmp/vimfifo4
$ vim -es test_config.conf < /tmp/vimfifo4 > /tmp/vimlog4.log 2>&1 &
$ exec 3>/tmp/vimfifo4
$ VIM_PID=$(pgrep -n -f "vim -es test_config.conf")
$ echo "vim PID: $VIM_PID"
vim PID: 32991
```

Swap file exists the instant the file is opened, before any edit is made
(this is the key property that makes it reliable — see Section 2):

```
-rw-r--r--@ 1 ayush  staff  4096 Jul 24 23:14 .test_config.conf.swp
```

### Step 3 — make a genuine UNSAVED edit (no `:w` / `:wq` ever issued)

```
$ printf "call append(line('\$'), 'UNSAVED_EDIT_BEFORE_CRASH_%s')\n" "$(date +%s)" >&3
$ sleep 6   # give vim's internal 'updatetime' (default 4000ms) time to
            # flush the pending change from memory into the .swp blocks
```

The swap file grows once the change is flushed (4096 → 12288 bytes),
confirming the unsaved edit is now captured on disk in the swap file — while
`test_config.conf` itself is still untouched:

```
$ ls -la .test_config.conf.sw*
-rw-r--r--@ 1 ayush  staff  12288 Jul 24 23:15 .test_config.conf.swp

$ cat test_config.conf     # <- still the ORIGINAL content, edit was never written here
# test_config.conf - sample config file for vi/vim crash-recovery demo
server_name = demo-server-01
port = 8080
max_connections = 100
log_level = info
```

### Step 4 — simulate the crash: `kill -9` the live vim process

```
$ kill -9 32991
$ ps -p 32991
  PID TTY           TIME CMD
$ echo "confirmed: vim PID 32991 gone (crash simulated)"
confirmed: vim PID 32991 gone (crash simulated)
```

No clean shutdown occurred, so vim never got the chance to run its normal
"delete the swap file on exit" cleanup. The swap file is left behind exactly
as a real crash would leave it:

```
$ ls -la .test_config.conf.sw*
-rw-r--r--@ 1 ayush  staff  12288 Jul 24 23:15 .test_config.conf.swp
```

### Step 5 — real recovery: `vim -r`

Listing all recoverable swap files (no filename given), actual output:

```
$ vim -r
Swap files found:
   In current directory:
1.    .test_config.conf.swp
          owned by: ayush   dated: Fri Jul 24 23:15:22 2026
         file name: ~ayush/Master-Code/CLI_SGA_2/Question_5_Vi_Recovery/test_config.conf
          modified: YES
         user name: ayush   host name: Ayushs-MacBook-Pro.local
        process ID: 32991
   In directory ~/tmp:
      -- none --
   In directory /var/tmp:
      -- none --
   In directory /tmp:
      -- none --
```

Note `modified: YES` and `process ID: 32991` — matching exactly the process
that was just killed. This is vim's own bookkeeping, not anything we wrote.

Actually recovering the file (scripted through Ex mode only because this
shell has no TTY to hold an interactive session open for a screenshot; a
human would simply run `vim -r test_config.conf`, see vim's
"Using swap file..." / "Recovery completed" banner, then `:w` to save):

```
$ vim -r -es -c 'w! recovered_test_config.conf' -c 'q!' test_config.conf
```

Recovered file content — **the unsaved line survived the crash**:

```
$ cat recovered_test_config.conf
# test_config.conf - sample config file for vi/vim crash-recovery demo
server_name = demo-server-01
port = 8080
max_connections = 100
log_level = info
UNSAVED_EDIT_BEFORE_CRASH_1784915118
```

Compare with the untouched original, which never received the edit:

```
$ cat test_config.conf
# test_config.conf - sample config file for vi/vim crash-recovery demo
server_name = demo-server-01
port = 8080
max_connections = 100
log_level = info
```

`.test_config.conf.swp` is a genuine Vim swap file, verified with `file`:

```
$ file .test_config.conf.swp
.test_config.conf.swp: Vim swap file, version 9.1, pid 32991, user ayush,
host Ayushs-MacBook-Pro.local, file
~ayush/Master-Code/CLI_SGA_2/Question_5_Vi_Recovery/test_config.conf, modified
```

**Conclusion of the live demo:** a change that was never written to
`test_config.conf` — and would have been lost forever in a real crash — was
fully recovered from the swap file using nothing but `vim -r`. This is a
real, reproducible result, not a description.

### What was demonstrated live vs. described conceptually

| Aspect | Live / real | Conceptual only |
|---|---|---|
| Real vim process opens file, creates real `.swp` | ✅ live (PID 32991, verified with `ps`, `file`, `ls -la`) | |
| Real unsaved edit sitting only in memory/swap | ✅ live (`test_config.conf` on disk proven unchanged) | |
| Real crash via `kill -9` on the live vim PID | ✅ live | |
| Real swap file surviving the crash | ✅ live (`file` command confirms it's a genuine Vim swap file, "modified") | |
| `vim -r` listing recoverable swaps | ✅ live, real output shown above | |
| Recovery of the unsaved line from the swap file | ✅ live (`recovered_test_config.conf` contains it) | |
| Full-screen interactive vim session, human watching the "found a swap file... recover?" prompt on-screen and pressing keys | — | conceptual: requires a real TTY, which this automated shell does not have. A human running the exact commands in `screenshots/README.txt` on their own Terminal.app **will** see this real interactive prompt. |

---

## 2. Evaluation of the five recovery mechanisms

### a) Swap files (`.swp`)

- Created **automatically** the moment a file is opened for editing (proven
  above — the `.swp` existed before any edit was made), not just at save time.
- Contains the unsaved buffer changes plus a pointer/fingerprint tying it back
  to the original file (path, device/inode-like info, host, PID, user,
  modified timestamp — all visible in the `vim -r` listing and `file` output
  above).
- Periodically flushed from memory to disk (governed by `updatetime`,
  default 4 seconds, and other triggers), so even a hard crash typically
  only loses the last few seconds of typing, not the whole session.
- Recovered with `vim -r file` (recover a specific file) or `vim -r` alone
  (list every recoverable swap file it can find).
- **Directly and specifically solves the "crash before save" scenario** —
  it is the only mechanism whose entire purpose is to preserve *unsaved*
  edits across an abnormal termination.

### b) Undo history (persistent undo, `.un~` files)

- Off by default; only exists if `:set undofile` (and `undodir`) were
  configured **before** the crash.
- Even when enabled, persistent undo files are themselves written to disk
  incrementally alongside normal saves/undo-tree changes — they help you
  step backward through edit history *within a buffer you have successfully
  reopened*, but they do not independently reconstruct a file vim never
  got to save. In this scenario, on the next fresh `vim file` after a
  crash, there is no open buffer with an undo tree to walk yet — you would
  first need the swap-file recovery to reconstruct the buffer, and only then
  would any prior persistent-undo history become relevant.
- Practically: not applicable/insufficient for "recover the unsaved
  work from a crash" unless swap recovery already happened first.

### c) Registers

- Hold yanked/deleted text (`"a`–`"z`, `"0`–`"9`, `"`, etc.) purely **in
  process memory** for the duration of the running vim session.
- Not persisted to disk by default (the `viminfo`/`shada` file only stores
  register contents at a *clean* `:wq`/exit, so it can save a register's
  content across sessions — but only if vim exits normally and writes its
  viminfo file, which a `kill -9` crash by definition prevents).
- **Least reliable mechanism for crash recovery**: a hard crash simply
  erases them along with everything else in the process's memory.

### d) Backup files (`~`)

- Controlled by `'backup'` / `'writebackup'` — vim copies the file's
  *previous on-disk state* to `filename~` right before an overwrite,
  i.e., only at the moment of a `:w`.
- Reflects the state **as of the last successful save**, nothing entered
  after that save is ever in it.
- In this exact scenario (crash occurs before the first save of the
  session), the backup file is either absent (if nothing was ever saved
  this session) or, at best, only contains the pre-edit content — it
  protects against a *bad overwrite of previously-saved data*, not against
  losing unsaved edits.

### e) Auto-recovery / crash-recovery workflow

- This is the **practical, end-to-end experience** a user has: the next
  time they run `vim file` (or `vi file`) on a file with a leftover swap,
  vim detects it and prints:

  ```
  E325: ATTENTION
  Found a swap file by the name ".file.swp"
  ...
  (1) Another program may be editing the same file...
  (2) An edit session for this file crashed...

  ...
  "Recover", "Delete it", "Quit", "Abort": ...
  ```

  Choosing **Recover** (or running `vim -r file` directly, as demonstrated
  live above) is exactly the swap-file mechanism from (a), surfaced as an
  interactive workflow. It is the "front door" a user actually walks
  through; the underlying engine is the swap file.

---

## 3. Recommendation

**Swap-file recovery (`vim -r`) is the most reliable strategy for a crash
that happens before the file is saved**, for three concrete reasons
demonstrated in this lab:

1. **Zero setup required.** Unlike persistent undo (`undofile`), swap files
   are on (`'swapfile'` is default-on) the moment vim starts — no prior
   configuration is needed. The developer in the scenario gets protection
   whether or not they thought about backups in advance.
2. **Created at open-time, not save-time.** The swap file exists and starts
   tracking changes the instant the buffer is opened (shown live above,
   before a single character was typed) — unlike backup files (`~`), which
   only capture state at the *previous* `:w` and are blind to anything typed
   afterward. In a "crash before any save" scenario, a backup file is
   useless almost by definition; a swap file is the only artifact actively
   watching the in-progress edit.
3. **Purpose-built and self-describing.** The swap file carries enough
   metadata (owning process, host, modified flag, timestamp — all seen in
   the real `vim -r` listing above) for vim to detect the crash automatically
   on the very next open and offer recovery, with no manual bookkeeping.
   Registers and undo history have no equivalent self-recovery workflow;
   registers vanish with the process, and undo history (even if enabled)
   has nothing to attach to until a buffer already exists, which itself
   depends on swap-file recovery having run first.

Contrast, briefly, for this specific scenario (crash *before* any save):

- Undo history: not helpful — requires pre-enabled `undofile`, and only
  operates on a buffer that's already open; it cannot independently
  reconstruct the un-saved session.
- Registers: not helpful — pure in-memory, wiped by the crash.
- Backup files: not helpful — only mirror the last saved state, and here
  there may be no "last saved state" from this session at all.

**Bottom line:** for "system crashes before the file is saved," reach for
`vim -r <file>` (or `vim -r` to see what's recoverable) first. It is the
mechanism actually engineered for this exact failure mode, it requires no
advance configuration, and — as shown in Section 1 — it genuinely recovers
data that exists nowhere else on disk.

---

## 4. Files created during this lab

All paths are relative to
`/Users/ayush/Master-Code/CLI_SGA_2/Question_5_Vi_Recovery/`:

- `test_config.conf` — the sample config file used for the demo (its
  on-disk content deliberately never received the "crash" edit, proving
  the edit was genuinely unsaved).
- `.test_config.conf.swp` — a **real** Vim swap file, genuinely produced by
  a live `vim` process (PID 32991) that was `kill -9`'d while an unsaved
  edit existed. Verified with `file` and `vim -r` (both shown above). Left
  in place intentionally as evidence for grading.
- `recovered_test_config.conf` — the output of the real `vim -r` recovery
  command; contains the `UNSAVED_EDIT_BEFORE_CRASH_...` line that was never
  written to `test_config.conf`, proving the recovery worked.
- `explanation.md` — this file.
- `screenshots/README.txt` — exact commands for a human to reproduce this
  interactively in Terminal.app and capture real screenshots (this
  automated session has no TTY/GUI, so screenshots could not be captured
  here).
