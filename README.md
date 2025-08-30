# COL331 – Operating Systems  
**Assignments from the Operating Systems course (COL331) at IIT Delhi, taught by Prof. S.R. Sarangi**

This repository contains implementations of various operating system concepts using the [xv6](https://github.com/mit-pdos/xv6-public) teaching OS, as part of the coursework for COL331.

---

## 📁 Assignments

### [Assignment 1: Shell Commands in xv6](./Assignment_1_Shell_Commands)
- Implemented custom shell commands and associated system calls in xv6.
- Features:
  - `history` system call to display command history.
  - `block` and `unblock` system calls to control permissions for other system calls.
  - `chmod` system call to modify file permissions.

---

### [Assignment 2: Signal Handling and Scheduling](./Assignment_2_Signals_and_Scheduling)
- Enhanced xv6 with signal handling and a modified scheduler.
- Features:
  - Signal delivery and handling, including interrupt, background, and foreground signals.
  - Custom signal handler registration.
  - Priority boosting added to the scheduler for fairer CPU allocation.

---

### [Assignment 3: Swap Memory Implementation in xv6](./Assignment_3_Swap_Memory)
- Added swap memory support to xv6 for better memory management.
- Features:
  - Modified disk structure to accommodate swap space.
  - Implemented page swapping (in and out) between RAM and disk.
  - Adaptive page replacement policy for efficient memory usage.

---

Each assignment progressively extends the functionality of the xv6 kernel, exploring real-world OS concepts such as process control, memory management, and CPU scheduling.
