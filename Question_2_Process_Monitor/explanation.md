# Question 2 — Process Monitor (Preventing Zombie/Unresponsive Child Processes)

## Design summary

`monitor.c` simulates a web server's request-handling parent process. It forks
4 child "worker" processes. Three of them do a short amount of simulated work
(`sleep(1)` or `sleep(2)`) and then exit normally. One child (index 2) is
deliberately made to hang forever (`while (1) pause();`) and it also installs
a `SIGTERM` handler that ignores the signal, so it behaves like a truly stuck
request handler that does not cooperate with a polite shutdown request.

The parent never blocks forever waiting on any single child. Instead it runs
a polling loop that, on every iteration (every 200ms):

1. Calls `waitpid(pid, &status, WNOHANG)` on every child that hasn't been
   reaped yet. If a child has exited, this reaps it immediately (removing it
   from the process table before it can become a zombie) and prints its exit
   status.
2. For children still running, checks `time(NULL) - start_time`. If a child
   has been running for at least `TIMEOUT_SEC` (3s) and hasn't been signaled
   yet, the parent sends `SIGTERM` (a polite "please stop" request) and
   records the time it did so.
3. If a child that already received `SIGTERM` is still alive after
   `GRACE_SEC` (2s) more, the parent escalates and sends `SIGKILL`, which
   cannot be caught or ignored.
4. The loop keeps running until every child has been reaped, guaranteeing
   zero zombies are left behind when the program exits.

## Commands executed

### 1. Compile

```
cc -Wall -Wextra -o monitor monitor.c
```

Compiled with no warnings or errors (exit code 0). This confirms the code is
clean under strict warning flags — no implicit declarations, no unused
variables, no signedness issues.

### 2. Run

```
time ./monitor
```

Observed real output (PIDs will differ per run, this is from the actual
captured run in `output.txt`):

- `[parent] Spawned child 0 (pid=31494)` ... `will finish in 1s` — child 0
  exited and was reaped almost immediately after, well before the 3-second
  timeout, so the parent never had to signal it.
- Child 1 (pid=31495) and child 3 (pid=31497) similarly finished in 2s and
  were reaped normally (`exited normally (code=0)`).
- Child 2 (pid=31496) printed `simulating an unresponsive worker (hanging,
  ignoring SIGTERM)` and never exited on its own.
- At the 3-second mark, the parent printed `Child 2 (pid=31496) unresponsive
  (running >= 3s) -> sending SIGTERM` — this is the timeout-detection logic
  firing.
- Because the hung child's handler swallows `SIGTERM`, it was still alive
  2 seconds later, so the parent printed `still alive after 2s grace period
  -> escalating to SIGKILL` and killed it with `SIGKILL`.
- The final reap line, `Child 2 (pid=31496) reaped, killed by signal 9`,
  confirms `WIFSIGNALED`/`WTERMSIG` correctly identified the kill signal (9 =
  SIGKILL), and the last line `All 4 children reaped, no zombies remain`
  confirms every forked process — including the one that had to be
  force-killed — was collected by `waitpid`.
- Wall-clock time for the whole run was `5.597s total` (per the `time`
  builtin), consistent with: 3s to detect the hang + 2s grace period before
  SIGKILL, with the other 3 children finishing and being reaped well before
  that window closed.

### 3. Inspect process table mid-run (verifying no zombies appear)

```
./monitor > /tmp/bg_run_out.txt 2>&1 &
sleep 1.3
ps -axo pid,ppid,stat,command | awk 'NR==1 || $4 ~ /monitor/'
sleep 5
ps -axo pid,ppid,stat,command | awk 'NR==1 || $4 ~ /monitor/'
```

At `t ~ 1.3s`, the `ps` snapshot showed exactly 4 live `./monitor` processes:
the parent (pid 32030) and 3 remaining children (32036, 32037, 32038, none in
`Z` zombie state). Child pid 32035 — the 1-second worker — had *already*
finished and been reaped by this point, so it does not appear at all; this is
direct evidence that the polling `waitpid(WNOHANG)` loop reaps finished
workers essentially as soon as they exit, instead of letting them linger as
zombies. After the run fully completed (second `ps` call, several seconds
later), the filtered `ps` output was empty (header row only) — proving that
even the child that had to be force-killed with `SIGKILL` left no zombie or
orphaned process behind.

## Files created

- `monitor.c` — the C source implementing the monitor.
- `monitor` — the compiled executable (produced by the compile command above).
- `output.txt` — captured real compile output + real run output + the `ps`
  zombie-check demonstration.
- `explanation.md` — this file.
- `screenshots/README.txt` — instructions for the human user to capture their
  own terminal screenshots for submission.

## How fork(), waitpid(), and signal handling work together

`fork()` is what turns the single monitor process into a small pool of
concurrent "request handlers," exactly like a web server spawning a worker
per incoming connection — each child is an independent process with its own
PID, so one worker hanging cannot block or crash the others. But every
forked child that exits leaves behind an entry in the kernel's process table
(a zombie) until its parent explicitly collects its exit status; if the
parent never does this, a busy server accumulates zombies indefinitely,
which is one face of the "excessive child processes" problem. `waitpid(pid,
&status, WNOHANG)` solves this half of the problem: called in a non-blocking
polling loop, it lets the parent check every child on every iteration without
ever stalling on a single slow one, reaping each child the instant it
finishes and immediately reporting how it died. The other half of the
problem — a child that never finishes at all — is not something waiting can
fix by itself, because `waitpid` only tells you a process is *still running*,
not that it is *stuck*. That is where the parent's own timestamp bookkeeping
and signals come in: by recording each child's start time, the parent can
independently decide a child has overrun its budget and is unresponsive, and
use `kill(pid, SIGTERM)` to ask it to shut down cooperatively. Because a
misbehaving or truly stuck worker might ignore or be unable to act on
`SIGTERM`, the parent escalates after a grace period to `kill(pid, SIGKILL)`,
which the kernel enforces unconditionally. Finally, the same polling loop
picks up that forced exit via `waitpid(WNOHANG)` on its very next iteration,
so even a force-killed child is reaped and never becomes a zombie. Together,
`fork()` gives concurrency, `waitpid(WNOHANG)` in a loop gives non-blocking
reaping that prevents zombies, and `SIGTERM`/`SIGKILL` give the parent a
graduated way to reclaim resources from workers that stop responding —
exactly the combination a web server needs to stay responsive under load
instead of drowning in stuck or abandoned child processes.
