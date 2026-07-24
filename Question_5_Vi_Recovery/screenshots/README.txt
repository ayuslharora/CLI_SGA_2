Question 5 - vi/vim Crash Recovery
Reproduce and screenshot this lab interactively in Terminal.app (macOS)
========================================================================

The automated session used to produce explanation.md had no controlling
TTY, so it could not drive a full-screen interactive vim session or take
GUI screenshots. Everything below is the exact real command sequence a
human should run in two Terminal.app windows/tabs side by side, so the
"found a swap file... recover?" prompt genuinely appears on screen and can
be screenshotted for submission.

You will need TWO terminal windows/tabs open at the same folder:
  cd /Users/ayush/Master-Code/CLI_SGA_2/Question_5_Vi_Recovery

--------------------------------------------------------------------------
STEP 1 - Start from a clean state (Terminal A)
--------------------------------------------------------------------------
    cd /Users/ayush/Master-Code/CLI_SGA_2/Question_5_Vi_Recovery
    rm -f .test_config.conf.swp recovered_test_config.conf
    cat test_config.conf

Screenshot #1: the clean starting file.

--------------------------------------------------------------------------
STEP 2 - Open the file in vim and make an UNSAVED edit (Terminal A)
--------------------------------------------------------------------------
    vim test_config.conf

Inside vim:
  1. Press  G  (go to last line)
  2. Press  o  (open a new line below, enters INSERT mode)
  3. Type:   UNSAVED_EDIT_BEFORE_CRASH
  4. Press  Esc  (back to NORMAL mode)
  5. Do NOT press :w or :wq. Leave vim sitting open, unsaved.

Screenshot #2: vim on screen showing the new unsaved line, still in vim
(you can see "[+]" or "modified" indicators depending on your statusline).

--------------------------------------------------------------------------
STEP 3 - Find vim's process ID and kill -9 it (Terminal B - the OTHER window)
--------------------------------------------------------------------------
    ps aux | grep "vim test_config.conf" | grep -v grep

Note the PID in the second column, then:

    kill -9 <PID>

Screenshot #3: Terminal B showing the ps output and the kill -9 command.

Back in Terminal A, vim's window will have vanished/terminal returned to
the shell prompt - this is the simulated crash (no clean exit, no chance
for vim to delete its own swap file).

--------------------------------------------------------------------------
STEP 4 - Confirm the crash artifact (either terminal)
--------------------------------------------------------------------------
    ls -la .test_config.conf.swp
    file .test_config.conf.swp
    cat test_config.conf     # <- still the OLD content, edit is missing here

Screenshot #4: the .swp file's existence and its "file" type identification,
plus test_config.conf still missing the unsaved line.

--------------------------------------------------------------------------
STEP 5 - Trigger vim's real auto-recovery prompt by reopening the file
--------------------------------------------------------------------------
    vim test_config.conf

Vim will detect the leftover swap file and show its real ATTENTION prompt,
approximately:

    E325: ATTENTION
    Found a swap file by the name ".test_config.conf.swp"
              owned by: <you>   dated: ...
             file name: .../test_config.conf
              modified: YES
             user name: <you>   host name: <your Mac's hostname>
            process ID: <PID>
    While opening file "test_config.conf"
                 dated: ...
    (1) Another program may be editing the same file...
    (2) An edit session for this file crashed.
        If this is the case, use ":recover" or "vim -r test_config.conf"
        to recover the changes (see ":help recovery").
        If you did this already, delete the swap file ".test_config.conf.swp"
        to avoid this message.

    Swap file ".test_config.conf.swp" already exists!
    [O]pen Read-Only, (E)dit anyway, (R)ecover, (D)elete it, (Q)uit, (A)bort:

Screenshot #5: this exact prompt on screen - this IS the "auto-recovery
workflow" named in the assignment question.

Press  R  to recover.

Screenshot #6: vim now showing the recovered buffer, including the
UNSAVED_EDIT_BEFORE_CRASH line, with a banner near the top such as:
    "recovered_test_config.conf" [readonly] [converted] ...
    (exact wording varies by vim version)

Then save the recovered content, e.g.:
    :w recovered_test_config.conf
    :q

Screenshot #7: cat recovered_test_config.conf, showing the previously
unsaved line was successfully recovered.

--------------------------------------------------------------------------
STEP 6 - Alternative non-interactive recovery command (either terminal)
--------------------------------------------------------------------------
    vim -r                      # lists all recoverable swap files found
    vim -r test_config.conf     # recovers directly into a vim session

Screenshot #8: output of "vim -r" listing the swap file with its metadata
(owner, date, modified: YES, process ID).

--------------------------------------------------------------------------
STEP 7 - Clean up (optional, either terminal)
--------------------------------------------------------------------------
    rm -f .test_config.conf.swp

========================================================================
Note: this exact sequence (minus the screenshots) was executed by the
grader's automated tooling using a non-interactive equivalent (vim -es
batch mode driven through a Unix FIFO, since that environment has no TTY
to hold an interactive vim session open). The real swap file it produced,
the real kill -9 crash, and the real vim -r recovery output are recorded
verbatim in explanation.md. This README exists so a human can additionally
reproduce the fully interactive, on-screen version (with the literal
ATTENTION/recover prompt) for a screenshot-based submission.
