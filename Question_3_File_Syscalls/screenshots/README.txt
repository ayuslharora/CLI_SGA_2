Screenshots to capture manually in Terminal.app for submission
================================================================

Run these commands yourself, in order, inside:
  /Users/ayush/Master-Code/CLI_SGA_2/Question_3_File_Syscalls

and take a screenshot of each Terminal window/output for the assignment
submission (this agent cannot capture GUI screenshots itself).

1) Compile the program:
   cc -Wall -Wextra -o employee_records employee_records.c

   Screenshot should show the command and the empty/clean output
   (no warnings, no errors).

2) Run the program:
   ./employee_records

   Screenshot should show the full output: the initial 5 records,
   the "Updated record" section (Charlie's salary changed in place),
   the "Random-access retrieval" section (Ethan fetched directly by
   index), and the final full dump plus "File descriptor closed. Done."

3) Confirm the binary file size matches record_count * sizeof(struct):
   ls -l employees.dat

   Screenshot should show a 300-byte file (5 records x 60 bytes each).

4) (Optional but recommended) Inspect the raw binary layout:
   xxd employees.dat | head -20

   Screenshot should show the employee names/bytes spaced at even
   60-byte (0x3C) intervals, confirming the fixed-size record layout.

Notes:
- Run these from the Question_3_File_Syscalls folder so employees.dat
  is created/read in the right place.
- If you re-run ./employee_records multiple times, employees.dat is
  truncated and rebuilt each time (O_TRUNC), so the output will be
  identical/reproducible.
