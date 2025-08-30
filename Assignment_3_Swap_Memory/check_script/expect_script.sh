#!/usr/bin/expect -f

set force_conservative 0  ;# set to 1 to force conservative mode even if
			  ;# script wasn't run conservatively originally
if {$force_conservative} {
	set send_slow {1 .1}
	proc send {ignore arg} {
		sleep .1
		exp_send -s -- $arg
	}
}

#
# 2) differing output - Some programs produce different output each time
# they run.  The "date" command is an obvious example.  Another is
# ftp, if it produces throughput statistics at the end of a file
# transfer.  If this causes a problem, delete these patterns or replace
# them with wildcards.  An alternative is to use the -p flag (for
# "prompt") which makes Expect only look for the last line of output
# (i.e., the prompt).  The -P flag allows you to define a character to
# toggle this mode off and on.
#
# Read the man page for more info.
#
# -Don


set timeout 10
# set testname [lindex $argv 0];

spawn /bin/sh
expect "$ "


# startup
puts "STARTUP"
send "make qemu-nox\r"
sleep 10
expect {
    "$" {}
    timeout {puts "Failed : Wrong Startup"; exit 1}
}

# echo 
puts "TESTING ECHO"
send "echo Hello1\r"
sleep 2
expect {
    "Hello1" {}
    timeout {puts "Failed : Echo not works"; exit 1}
}

# CTRL + I
puts "TESTING CTRL + I"
send "\x09"
sleep 2

expect {
    "Ctrl+I is detected by xv6" {}
    timeout {puts "Failed : Wrong Format"; exit 1}
}

expect {
    "PID NUM_PAGES" {}
    timeout {puts "Failed : Wrong Format"; exit 1}
}

expect {
    "1 3" {}
    timeout {puts "Failed : Wrong user space pages for init process"; exit 1}
}
puts "CTRL + I gives correct output for PID 1"

# Memtest
sleep 2
puts "TESTING MEMTEST"
send "memtest\r"
sleep 3

# Checking some of the (Threshold, Npg) pairs
expect {
    "Current Threshold = 100, Swapping 4 pages" {}
    timeout {puts "Failed : Incorrect threshold and swapped out pages"; exit 1}
}
expect {
    "Current Threshold = 66, Swapping 8 pages" {}
    timeout {puts "Failed : Incorrect threshold and swapped out pages"; exit 1}
}
expect {
    "Current Threshold = 54, Swapping 12 pages" {}
    timeout {puts "Failed : Incorrect threshold and swapped out pages"; exit 1}
}
expect {
    "Current Threshold = 37, Swapping 27 pages" {}
    timeout {puts "Failed : Incorrect threshold and swapped out pages"; exit 1}
}
expect {
    "Current Threshold = 22, Swapping 97 pages" {}
    timeout {puts "Failed : Incorrect threshold and swapped out pages"; exit 1}
}


expect {
    "Memtest Passed" {}
    timeout {puts "Failed : Test case does not pass"; exit 1}
}

sleep 2

# finishing test
puts "Passed"
send "\x01"; send "x"
expect "QEMU: Terminated\r"
expect "$ "
send "exit\r"
expect eof
