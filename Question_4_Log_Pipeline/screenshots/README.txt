Question 4 — Screenshots to capture in Terminal.app
=====================================================

You cannot be screenshotted by the assistant, so run the following commands
yourself in Terminal.app (macOS) and take a screenshot after each step, in
this exact order. Run everything from inside this folder:

    cd /Users/ayush/Master-Code/CLI_SGA_2/Question_4_Log_Pipeline

Step 1 — Make sure the generator script is executable (one time only):

    chmod +x generate_logs.sh

Step 2 — Start the monitoring pipeline in the background (this is the core
deliverable command — take a screenshot right after running it, showing the
"[1] <pid>" job-control line bash prints):

    tail -f app.log | grep --line-buffered "ERROR" | tee -a error_report.txt > /dev/null &

Step 3 — In the SAME terminal window, run the generator in the foreground so
new log lines are appended while the pipeline (from Step 2) is still running
in the background. Screenshot the output while it runs (it takes about 8
seconds, one line per second):

    ./generate_logs.sh

Step 4 — Confirm the background pipeline job is still (or was) running —
screenshot the job list:

    jobs -l

Step 5 — Stop the background pipeline now that the generator has finished:

    kill %1

Step 6 — Show the final captured report — screenshot this output, it should
contain ONLY lines with the word ERROR in them:

    cat error_report.txt

Step 7 (optional but recommended) — Show the full app.log for comparison, so
the grader can see the INFO/WARN lines that were correctly filtered OUT of
error_report.txt:

    cat app.log

Take one screenshot per step (7 screenshots total, or 6 if you skip the
optional step 7). Save them in this screenshots/ folder as step1.png,
step2.png, ... step7.png.
