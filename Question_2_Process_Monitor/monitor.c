#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>

#define NUM_CHILDREN   4
#define HANG_INDEX     2      /* this worker simulates an unresponsive request handler */
#define TIMEOUT_SEC    3      /* max seconds a worker may run before it is considered stuck */
#define GRACE_SEC      2      /* seconds to wait after SIGTERM before escalating to SIGKILL */
#define POLL_USEC      200000

typedef enum { RUNNING, TERM_SENT, KILL_SENT, REAPED } child_state_t;

typedef struct {
    pid_t pid;
    int index;
    time_t start_time;
    time_t signal_time;
    child_state_t state;
} child_t;

static void ignore_sigterm(int sig) {
    (void)sig; /* unresponsive worker deliberately swallows SIGTERM to force a SIGKILL escalation */
}

static void run_child(int index) {
    if (index == HANG_INDEX) {
        struct sigaction sa = {0};
        sa.sa_handler = ignore_sigterm;
        sigaction(SIGTERM, &sa, NULL);
        printf("[child %d] pid=%d simulating an unresponsive worker (hanging, ignoring SIGTERM)\n",
               index, getpid());
        fflush(stdout);
        while (1) pause();
    }

    int work_sec = 1 + (index % 2); /* keeps all normal workers well under TIMEOUT_SEC */
    printf("[child %d] pid=%d handling request, will finish in %ds\n", index, getpid(), work_sec);
    fflush(stdout);
    sleep(work_sec);
    printf("[child %d] pid=%d finished normally\n", index, getpid());
    exit(0);
}

static void report_exit(child_t *c, int status) {
    if (WIFEXITED(status))
        printf("[parent] Child %d (pid=%d) reaped, exited normally (code=%d)\n",
               c->index, c->pid, WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("[parent] Child %d (pid=%d) reaped, killed by signal %d\n",
               c->index, c->pid, WTERMSIG(status));
    fflush(stdout);
}

int main(void) {
    child_t children[NUM_CHILDREN];

    printf("[parent] Starting monitor, launching %d worker processes\n", NUM_CHILDREN);
    fflush(stdout);

    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        if (pid == 0)
            run_child(i);

        children[i].pid = pid;
        children[i].index = i;
        children[i].start_time = time(NULL);
        children[i].signal_time = 0;
        children[i].state = RUNNING;
        printf("[parent] Spawned child %d (pid=%d)\n", i, pid);
        fflush(stdout);
    }

    int remaining = NUM_CHILDREN;
    while (remaining > 0) {
        for (int i = 0; i < NUM_CHILDREN; i++) {
            child_t *c = &children[i];
            if (c->state == REAPED) continue;

            int status;
            pid_t r = waitpid(c->pid, &status, WNOHANG);
            if (r == c->pid) {
                report_exit(c, status);
                c->state = REAPED;
                remaining--;
                continue;
            }

            time_t now = time(NULL);
            if (c->state == RUNNING && now - c->start_time >= TIMEOUT_SEC) {
                printf("[parent] Child %d (pid=%d) unresponsive (running >= %ds) -> sending SIGTERM\n",
                       c->index, c->pid, TIMEOUT_SEC);
                fflush(stdout);
                kill(c->pid, SIGTERM);
                c->state = TERM_SENT;
                c->signal_time = now;
            } else if (c->state == TERM_SENT && now - c->signal_time >= GRACE_SEC) {
                printf("[parent] Child %d (pid=%d) still alive after %ds grace period -> escalating to SIGKILL\n",
                       c->index, c->pid, GRACE_SEC);
                fflush(stdout);
                kill(c->pid, SIGKILL);
                c->state = KILL_SENT;
            }
        }
        if (remaining > 0)
            usleep(POLL_USEC);
    }

    printf("[parent] All %d children reaped, no zombies remain. Monitor exiting.\n", NUM_CHILDREN);
    return 0;
}
