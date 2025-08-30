#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_PROCS 3  // Number of processes to create

int main() {
    // int pids[NUM_PROCS];

    // Create child processes using custom_fork with start_later = 1
    for (int i = 0; i < NUM_PROCS; i++) {
        int t = i+1;
        int pid = custom_fork(1, 50); // Start later, execution time 50
        if (pid < 0) {
            printf(1, "Failed to fork process %d\n", i);
            exit();
        } else if (pid == 0) {
            // Child process
            sleep(100 *t);
            printf(1, "Child %d (PID: %d) started but should not run yet.\n", i, getpid());
            for (volatile int j = 0; j < 100000000; j++); // Simulated work
            sleep(200 *t);
            printf(1, "Child %d (PID: %d) exiting.\n", i, getpid());
            exit();
        } else {
            // Parent stores PID
            // pids[i] = pid;

        }
    }

    printf(1, "All child processes created with start_later flag set.\n");
    // sleep(400);

    // Start scheduling these processes
    printf(1, "Calling sys_scheduler_start() to allow execution.\n");
    scheduler_start();

    // Wait for children to finish
    for (int i = 0; i < NUM_PROCS; i++) {
        wait();

    }

    printf(1, "All child processes completed.\n");
    exit();
}
