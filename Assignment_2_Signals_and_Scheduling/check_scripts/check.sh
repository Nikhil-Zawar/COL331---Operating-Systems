#!/bin/bash


submission=$1
mkdir ./test_dir

echo $submission

fileNameRegex_double="assignment2_easy_[0-9]{4}[A-Z]{2}.[0-9]{4}.tar.gz"
fileNameRegex_single="assignment2_easy_[0-9]{4}[A-Z]{2}.[0-9]{4}_[0-9]{4}[A-Z]{2}.[0-9]{4}.tar.gz"

if ! [[ $submission =~ $fileNameRegex_double || $submission =~ $fileNameRegex_single ]]; then
    echo "File doesn't match the naming convention"
    exit
fi

echo "Setting the test directory"

tar -xzvf $submission -C ./test_dir
cp tom.c test_sched.c expect_script.sh Makefile ./test_dir
cd ./test_dir

expect -f expect_script.sh
