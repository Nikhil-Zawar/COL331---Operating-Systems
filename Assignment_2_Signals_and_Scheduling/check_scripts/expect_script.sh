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


set timeout 5
# set testname [lindex $argv 0];

spawn /bin/sh
expect "$ "


# startup
puts "STARTUP"
send "make qemu-nox\r"
sleep 1
expect {
    "$" {}
    timeout {puts "Wrong Startup"; exit 1}
}

sleep 1

# echo 
puts "TESTING ECHO"
send "echo Hello1\r"
sleep 2
expect {
    "Hello1" {}
    timeout {puts "Echo not works"; exit 1}
}

# user function tom
puts "TESTING TOM"
send "tom\r"
sleep 2
expect {
    "This is normal code running" {}
    timeout {puts "user program not works"; exit 1}
}

# test ctrl+c
puts "TESTING CTRL+C"
send "\x03"
expect {
    "Ctrl-C is detected by xv6" {}
    timeout {puts "Correct format not followed"; exit 1}
}
sleep 2
send "echo Hello2\r"
sleep 2
expect {
    "Hello2" {}
    timeout {puts "Echo not works after Ctrl+C"; exit 1}
}

# test ctrl+b
puts "TESTING CTRL+B"
send "tom\r"
sleep 2
expect {
    "This is normal code running" {}
    timeout {puts "user program not works"; exit 1}
}
send "\x02"
expect {
    "Ctrl-B is detected by xv6" {}
    timeout {puts "Correct format not followed"; exit 1}
}
sleep 2
send "echo Hello3\r"
sleep 2
expect {
    "Hello3" {}
    timeout {puts "Echo not works after Ctrl+B"; exit 1}
}

# test ctrl+f
puts "TESTING CTRL+F"
send "\x06"
expect {
    "Ctrl-F is detected by xv6" {}
    timeout {puts "Correct format not followed"; exit 1}
}
sleep 2
expect {
    "This is normal code running" {}
    timeout {puts "user program not brought to foreground"; exit 1}
}

# test ctrl+c again
puts "TESTING CTRL+C AGAIN"
send "\x03"
expect {
    "Ctrl-C is detected by xv6" {}
    timeout {puts "Correct format not followed"; exit 1}
}
sleep 2
send "echo Hello4\r"
sleep 2
expect {
    "Hello4" {}
    timeout {puts "Echo not works after Ctrl+C"; exit 1}
}

# test custom signal handler
puts "TESTING CUSTOM SIGNAL HANDLER"
send "tom\r"
sleep 2
expect {
    "This is normal code running" {}
    timeout {puts "user program not works"; exit 1}
}
send "\x07"
expect {
    "Ctrl-G is detected by xv6" {}
    timeout {puts "Correct format not followed"; exit 1}
}
expect {
    "I am inside the handler" {}
    timeout {puts "signhandler not working"; exit 1}
}
expect {
    "I am Shivam" {}
    timeout {puts "not calling another function 1"; exit 1}
}
expect {
    "Final call" {}
    timeout {puts "not calling another function 2"; exit 1}
}
expect {
    "This is normal code running" {}
    timeout {puts "signal handler not returning"; exit 1}
}

# test ctrl+c again bro
puts "TESTING CTRL+C AGAIN BRO"
send "\x03"
expect {
    "Ctrl-C is detected by xv6" {}
    timeout {puts "Correct format not followed"; exit 1}
}
sleep 2
send "echo Hello5\r"
sleep 2
expect {
    "Hello5" {}
    timeout {puts "Echo not works after Ctrl+C"; exit 1}
}

# finishing test
puts "YAYY FINISHED SIGNAL TEST"
# send "\x01"; send "x"
# expect "QEMU: Terminated\r"
# expect "$ "
# send "exit\r"
# expect eof


# puts "No of Command Line Arguments : [llength $argv]"
# set argsCount [llength $argv];
# if { $argsCount < 1} {
#     puts "Need a user_function name. \n";
#     exit 1
# } 
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


set timeout 50
# set testname [lindex $argv 0];

# spawn /bin/sh
# expect "$ "


# startup
# puts "STARTUP"
# send "make qemu-nox\r"
# sleep 1
# expect {
#     "$" {}
#     timeout {puts "Wrong Startup"; exit 1}
# }

# # echo 
# puts "TESTING ECHO"
# send "echo Hello1\r"
# sleep 2
# expect {
#     "Hello1" {}
#     timeout {puts "Echo not works"; exit 1}
# }

# user function test_sched
puts "TESTING SCHEDULER"
send "test_sched\r"
sleep 2

# Initialize storage variables
array set stats {}
set child_processes [list]
set started_pids [list]
set exited_pids [list]

set expected_child_count 3  ;# Set the number of expected child processes
set matched_count 0         ;# Counter to track matches

# send "\x07"
expect {
    "All child processes created with start_later flag set." {}
    timeout {puts "Child processes creation Failed!!!"; exit 1}
}
expect {
    "Calling sys_scheduler_start() to allow execution." {}
    timeout {puts "sys_scheduler_start() not called!!"; exit 1}
}
expect {
    -re "Child (\\d+) \\(PID: (\\d+)\\) started but should not run yet.\r\n" {
        set child_num $expect_out(1,string)
        set pid $expect_out(2,string)
        lappend started_pids $pid
        lappend child_processes $child_num
        incr matched_count
        if {$matched_count < $expected_child_count} {
            exp_continue
        }
    }
    timeout { puts "Child $child_num (PID $pid) failed";   exit 1}
}    

set matched_count 0 
expect {     
    -re "Child \\d+ \\(PID: (\\d+)\\) exiting.\r\nPID: (\\d+)\r\nTAT: (\\d+)\r\nWT: (\\d+)\r\nRT: (\\d+)\r\n#CS: (\\d+)" {
            set child_num $expect_out(1,string)
            set pid $expect_out(2,string)
            lappend exited_pids $pid
            # set pid $expect_out(3,string)
            set stats($pid,tt) $expect_out(3,string)
            set stats($pid,wt) $expect_out(4,string)
            set stats($pid,rt) $expect_out(5,string)
            set stats($pid,cs) $expect_out(6,string)
            # set stats($pid,trt) $expect_out(7,string)
            incr matched_count
            if {$matched_count < $expected_child_count} {
                exp_continue
            }
    }      
    timeout { puts "Child $child_num (PID $pid) failed";   exit 1}
}
expect {
    "All child processes completed." {}
    timeout {puts "Scheduler test timed out!!!"; exit 1}
}

# Verify process completion
puts "\nProcess Tracking:"
puts "Started PIDs: $started_pids"
puts "Exited PIDs: $exited_pids"

foreach pid $started_pids {
    if {$pid ni $exited_pids} {
        puts "Warning: PID $pid started but never exited!"
    }
}

# Verify each child_num has a corresponding PID
if {[llength $child_processes] != [llength $started_pids]} {
    puts "ERROR: Mismatch between child numbers and PIDs!"
    exit 1
}

# Finalizing test
puts "SCHEDULER TEST PASSED"
send "\x01"; send "x"
expect "QEMU: Terminated\r"
expect "$ "
send "exit\r"
expect eof
