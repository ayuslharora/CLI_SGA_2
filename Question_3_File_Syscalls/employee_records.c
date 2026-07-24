/*
 * employee_records.c
 *
 * Secure file-processing utility built entirely on low-level POSIX
 * syscalls: open(), read(), write(), lseek(), close().
 *
 * Demonstrates:
 *   1. Creating a binary file of fixed-size employee records.
 *   2. Writing an initial batch of records sequentially.
 *   3. Updating ONE record in place using lseek() + write()
 *      (no rewrite of the whole file).
 *   4. Retrieving a record from an arbitrary index using
 *      lseek() + read() (direct/random access, not a scan).
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_FILE "employees.dat"
#define NUM_RECORDS 5

struct Employee {
    int   id;
    char  name[50];
    float salary;
};

/* Aborts with a clear message when a syscall fails; used only on real
 * error paths (open/write/read returning -1), not on impossible cases. */
static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void print_record(const char *label, int index, const struct Employee *e) {
    printf("%-10s [index %d] id=%-4d name=%-12s salary=%.2f\n",
           label, index, e->id, e->name, e->salary);
}

/* Creates (or truncates) the data file and writes the initial records
 * sequentially with write(). O_CREAT|O_TRUNC|O_RDWR guarantees we start
 * from a clean, known file layout each run. */
static int create_and_populate(const struct Employee *records, int count) {
    int fd = open(DATA_FILE, O_CREAT | O_TRUNC | O_RDWR, 0644);
    if (fd == -1) die("open (create)");

    for (int i = 0; i < count; i++) {
        ssize_t written = write(fd, &records[i], sizeof(struct Employee));
        if (written == -1) die("write (initial populate)");
        if (written != (ssize_t)sizeof(struct Employee)) {
            fprintf(stderr, "short write on record %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
    return fd;
}

/* Reads every record back sequentially from the start of the file
 * purely to display current state; production code would not need
 * this for random access, it is here only for the before/after report. */
static void print_all_records(int fd, int count) {
    struct Employee e;
    if (lseek(fd, 0, SEEK_SET) == -1) die("lseek (rewind for dump)");

    printf("\n-- All records after initial write --\n");
    for (int i = 0; i < count; i++) {
        ssize_t r = read(fd, &e, sizeof(e));
        if (r == -1) die("read (dump)");
        if (r == 0) break; /* end of file, nothing more to show */
        print_record("record", i, &e);
    }
}

/* Updates exactly one record in place. lseek() jumps directly to
 * index * sizeof(struct Employee) -- O(1) positioning -- then write()
 * overwrites only that record's bytes. The rest of the file is
 * untouched, so this is NOT a rewrite of the whole file. */
static void update_record(int fd, int index, struct Employee new_value) {
    off_t offset = (off_t)index * sizeof(struct Employee);

    if (lseek(fd, offset, SEEK_SET) == -1) die("lseek (update)");

    ssize_t written = write(fd, &new_value, sizeof(new_value));
    if (written == -1) die("write (update)");
    if (written != (ssize_t)sizeof(new_value)) {
        fprintf(stderr, "short write while updating record %d\n", index);
        exit(EXIT_FAILURE);
    }
}

/* Retrieves a single record from an arbitrary index using direct
 * access: lseek() to index * sizeof(struct Employee), then a single
 * read(). No scanning of preceding records is required. */
static struct Employee get_record(int fd, int index) {
    struct Employee e;
    off_t offset = (off_t)index * sizeof(struct Employee);

    if (lseek(fd, offset, SEEK_SET) == -1) die("lseek (get)");

    ssize_t r = read(fd, &e, sizeof(e));
    if (r == -1) die("read (get)");
    if (r != (ssize_t)sizeof(e)) {
        fprintf(stderr, "record %d not found (short/empty read)\n", index);
        exit(EXIT_FAILURE);
    }
    return e;
}

int main(void) {
    struct Employee initial[NUM_RECORDS] = {
        {101, "Alice",   55000.00f},
        {102, "Bob",     48000.00f},
        {103, "Charlie", 62000.00f},
        {104, "Diana",   71000.00f},
        {105, "Ethan",   39000.00f},
    };

    int fd = create_and_populate(initial, NUM_RECORDS);
    print_all_records(fd, NUM_RECORDS);

    /* Update the record at index 2 (Charlie) in place: a raise. */
    int update_index = 2;
    struct Employee updated = {103, "Charlie", 85000.00f};
    update_record(fd, update_index, updated);

    struct Employee after_update = get_record(fd, update_index);
    printf("\n-- Updated record (in place, via lseek+write) --\n");
    print_record("updated", update_index, &after_update);

    /* Retrieve an arbitrary record directly, e.g. index 4 (Ethan),
     * without scanning index 0..3 first. */
    int lookup_index = 4;
    struct Employee looked_up = get_record(fd, lookup_index);
    printf("\n-- Random-access retrieval (via lseek+read) --\n");
    print_record("fetched", lookup_index, &looked_up);

    /* Final full dump to show the file now reflects the update and
     * nothing else changed. */
    print_all_records(fd, NUM_RECORDS);

    if (close(fd) == -1) die("close");

    printf("\nFile descriptor closed. Done.\n");
    return 0;
}
