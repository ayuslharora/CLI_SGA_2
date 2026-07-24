# Question 4 — Real-Time Log Monitoring Pipeline

## Goal

Build a monitoring tool that:
- Displays newly added log entries in real time.
- Extracts only `ERROR` messages.
- Maintains a separate, persistent report file.
- Suppresses unnecessary terminal output.

## Files in this folder

- `app.log` — starter log file (10 lines, mix of INFO / WARN / ERROR).
- `generate_logs.sh` — appends 8 more INFO/WARN/ERROR lines to `app.log`, one per second, to simulate a live system.
- `error_report.txt` — the persistent report produced by the pipeline (real output, captured below).
- `output.txt` — full real terminal transcript of the run described below.
- `explanation.md` — this file.
- `screenshots/README.txt` — exact commands for the human to screenshot in Terminal.app.

## The pipeline command

```bash
tail -f app.log | grep --line-buffered "ERROR" | tee -a error_report.txt > /dev/null &
```

This was launched as a background job, then `generate_logs.sh` was run in the foreground so new lines
landed in `app.log` while the pipeline was live, and finally the background job was stopped with
`kill %1`.

## Commands executed, in order, with real observations

### 1. `rm -f error_report.txt`
Removed any old report so the run below starts from a clean file. No output (silent on success).

### 2. `tail -f app.log | grep --line-buffered "ERROR" | tee -a error_report.txt > /dev/null &`
Started the monitoring pipeline as a background job (bash reported `[1] 31699`, the PID of the last
stage, `tee`). `tail -f` immediately printed the file's existing tail into the pipe; since `app.log`
only had 10 lines at that point, all 10 were read, `grep --line-buffered "ERROR"` matched the 3
existing ERROR lines, and `tee -a` appended them to `error_report.txt` while its own stdout copy was
discarded by `> /dev/null`. Nothing appeared on the terminal because of the redirect — exactly the
"suppress unnecessary output" requirement.

### 3. `./generate_logs.sh`
Ran the log generator in the foreground while the background pipeline was still running. It appended
8 new lines to `app.log` at 1-second intervals (3 of them `ERROR`) and printed:
```
generate_logs.sh: finished appending 8 new log lines to app.log
```
Because `tail -f` was watching the file, each new line was picked up within the same second it was
written, piped through `grep`, and the 3 new `ERROR` lines were appended to `error_report.txt` live —
this is the "display newly added entries in real time" behavior in action, just with the terminal echo
suppressed.

### 4. `jobs -l`
Confirmed the pipeline job was still alive before stopping it:
```
[1]+ 31695 Running    tail -f app.log
     31698             | grep --line-buffered "ERROR"
     31699             | tee -a error_report.txt > /dev/null &
```

### 5. `kill %1`
Sent SIGTERM to the background pipeline job. Bash reported:
```
Terminated: 15    tail -f app.log
```
A `pkill -f "tail -f app.log"` safety check afterward found no surviving process, confirming the
whole pipeline (tail → grep → tee) shut down cleanly once `tail` was killed and its pipe closed.

### 6. `wait`
Waited for the job to fully finish before reading the report. Returned immediately with no output
since the job was already dead.

### 7. `cat error_report.txt`
Printed the final, real contents of the report file:
```
2026-07-24 10:00:05 ERROR Database connection failed: timeout after 30s
2026-07-24 10:00:18 ERROR NullPointerException in UserService.getProfile()
2026-07-24 10:00:31 ERROR Disk write failed: /var/log/app/data.bin: No space left on device
2026-07-24 23:13:31 ERROR Payment gateway timeout: no response from upstream
2026-07-24 23:13:34 ERROR Redis connection refused: 127.0.0.1:6379
2026-07-24 23:13:37 ERROR Unhandled exception in OrderController.process(): NullPointerException
```
All 6 lines contain `ERROR` and nothing else — no INFO or WARN lines leaked through. The first 3 are
the ERROR lines that were already in `app.log` before the pipeline started (picked up by `tail`'s
default last-10-lines behavior); the last 3 are the ERROR lines appended live by `generate_logs.sh`
while the pipeline was running.

### 8. `wc -l error_report.txt`
Confirmed the line count: `6 error_report.txt`, matching the 6 ERROR lines counted by hand above.

## How pipes, grep, tail, redirection, and /dev/null improve efficiency

Piping connects `tail -f`'s stdout directly to `grep`'s stdin, and `grep`'s stdout directly to `tee`'s
stdin, so three small, single-purpose Unix tools are chained into one monitoring tool instead of
writing a monolithic custom program that opens the file, polls for changes, filters lines, and manages
a report file itself. `tail -f` keeps the file descriptor open and blocks waiting for new data (using
the OS's file-change notifications), so it delivers newly appended lines the instant they are written —
this is what gives "real time" monitoring almost for free. `grep --line-buffered "ERROR"` filters the
stream down to only the lines that matter, flushing its output after every matching line rather than
waiting for a full buffer, which keeps the downstream stages just as responsive as `tail`. `tee -a
error_report.txt` is the key to "maintain a separate report file": it duplicates the stream, writing an
appended copy to `error_report.txt` on disk while still passing the same data through to its own
stdout, so nothing is lost even though the report file is persistent, and the log data is only read
from disk once by `tail`. Finally, redirecting that trailing stdout to `/dev/null` discards the
now-redundant terminal echo — the data has already been safely written by `tee`, so echoing it a
second time to the screen would just be noise, and `/dev/null` is the standard "black hole" device for
throwing away output cheaply without an if/else check in code. Together this design is more efficient
than a bespoke program because each tool streams data line-by-line instead of loading the whole file
into memory, each process does one job well (composability), the pipeline can be restarted or edited by
swapping one stage (e.g. changing the grep pattern) without touching the others, and the whole thing
was assembled from existing, battle-tested OS utilities in one line instead of writing, compiling, and
maintaining custom file-watching and I/O-handling code.
