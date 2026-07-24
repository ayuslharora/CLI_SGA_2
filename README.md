# Graded Lab Assignment — Modules 5-10

Each `Question_N_*` folder contains the script(s)/program(s) for that question,
its real captured output, an `explanation.md` walking through every command
executed with 1-2 sentence observations, and a `screenshots/` folder.

| Folder | Topic |
|---|---|
| `Question_1_Duplicate_Submissions/` | Shell script: duplicate detection, backup, and reporting |
| `Question_2_Process_Monitor/` | C: fork(), zombie-safe reaping, signal-based termination |
| `Question_3_File_Syscalls/` | C: open/read/write/lseek/close on fixed-size records |
| `Question_4_Log_Pipeline/` | Shell pipeline: tail -f / grep / tee / /dev/null |
| `Question_5_Vi_Recovery/` | vi/vim crash recovery mechanisms, live swap-file demo |

## Screenshots

Each `screenshots/README.txt` lists the exact commands to run and screenshot
yourself — these were run and verified locally to produce the outputs saved
in each folder, but the actual screenshot images have to be captured on your
own machine (Cmd+Shift+4 on macOS) and dropped into that folder before
pushing to GitHub.

## Environment

All commands/programs were developed and verified on macOS (Darwin), using
the same POSIX APIs (fork, wait/waitpid, signals, open/read/write/lseek/close,
pipes) that a Linux server provides. Question 1 additionally auto-detects
`sha256sum` vs `shasum -a 256` so it runs unmodified on a Linux grading
server.
