# COL331 -- Operating-Systems -- Course Assignments
This repository contains the completed assignments for the Operating Systems course. 
The primary focus of these assignments is to add and modify functionalities within the xv6 teaching operating system.

## Repository Structure & File Descriptions
Inside each assignment folder, you will find the following files:
- The original assignment problem statement, detailing the requirements and tasks.
- This is the compressed tarball containing the complete source code for the xv6 project after implementing the required features.
- changes.patch: A patch file that contains all the modifications made to the original xv6 source code. This file makes it easy to see the exact changes implemented for the assignment.


## Assignments Overview

### Assignment 1: Shell Enhancements and System Calls
Enhanced Shell: Implemented a username-password based login system to protect the xv6 shell.
history Command: Added a new shell command and a corresponding sys_gethistory system call to display a history of executed processes with their PID, name, and memory usage.
block/unblock Commands: Introduced sys_block and sys_unblock system calls to block and unblock other system calls for processes spawned by the current shell.

### Assignment 2: Signals and Scheduling
Signal Handling: Added support for handling keyboard interrupts in xv6:
Ctrl+C (SIGINT): Terminates processes.
Ctrl+B (SIGBG): Suspends processes and sends them to the background.
Ctrl+F (SIGFG): Resumes suspended background processes.
Ctrl+G (SIGCUSTOM): Invokes a user-registered custom signal handler.
Priority Boosting Scheduler: Replaced the default round-robin scheduler with a priority-based scheduler. The priority of a process is dynamically adjusted based on its CPU consumption and wait time to prevent starvation.

### Assignment 3: Memory Management
Memory Printer: Created a handler invoked by Ctrl+I that prints the number of memory pages allocated to each running, sleeping, and runnable user process.
Page Swapping: Implemented a page swapping mechanism to move memory pages to a reserved swap space on the disk when physical memory is low.
Adaptive Page Replacement: Developed an adaptive page replacement strategy that dynamically adjusts the threshold for swapping and the number of pages to be swapped out based on system load.

## How to Use the Patch Files
The changes.patch file in each assignment directory allows you to apply the modifications to a clean xv6 source tree.
Start with a clean xv6 directory.
Copy the changes.patch file into the root of the clean xv6 directory.
Apply the patch using the following command:
git apply changes.patch


Your xv6 source code is now updated with the solution for that assignment. You can compile and run it using make qemu.
