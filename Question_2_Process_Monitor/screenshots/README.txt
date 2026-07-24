Screenshots to capture for submission (Question 2 — Process Monitor)
=====================================================================

These are the exact commands that were actually executed to build this
deliverable (see explanation.md and output.txt for the real captured
output). Run them yourself in Terminal.app from inside this directory,
and take one screenshot per step:

  cd /Users/ayush/Master-Code/CLI_SGA_2/Question_2_Process_Monitor

1) Compile (screenshot the command + the fact that it produces no
   warnings/errors and exit code 0):

     cc -Wall -Wextra -o monitor monitor.c
     echo "exit code: $?"

2) Run the monitor and let it run to completion (should take about
   5-6 seconds). Screenshot the full scrolled terminal output showing:
     - all 4 children spawned with their PIDs
     - the 3 normal children finishing and being reaped
     - child 2 being detected as unresponsive, SIGTERM sent
     - child 2 still alive after the grace period, SIGKILL sent
     - the final "All 4 children reaped, no zombies remain" line

     time ./monitor

3) (Optional but recommended) Demonstrate that no zombies are left
   behind, by starting the monitor in the background, checking the
   process table partway through the run, then again after it
   finishes:

     ./monitor > /tmp/bg_run_out.txt 2>&1 &
     sleep 1.3
     ps -axo pid,ppid,stat,command | awk 'NR==1 || $4 ~ /monitor/'
     sleep 5
     ps -axo pid,ppid,stat,command | awk 'NR==1 || $4 ~ /monitor/'
     cat /tmp/bg_run_out.txt

   Screenshot the first ps output (showing the parent + still-running
   children, none in "Z" zombie state), and the second ps output
   (showing it is empty/header-only, proving every child — including
   the force-killed one — was fully reaped with no leftovers).

Save each screenshot into this screenshots/ folder (e.g.
screenshots/01_compile.png, screenshots/02_run.png,
screenshots/03_ps_check.png) before submitting.
