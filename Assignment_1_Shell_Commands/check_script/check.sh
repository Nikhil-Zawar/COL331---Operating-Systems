!/bin/bash

submission=$1
mkdir ./test_dir

echo $submission

fileNameRegex="assignment1_easy_[0-9]{4}[A-Z]{2}.[0-9]{4}.tar.gz"

if ! [[ $submission =~ $fileNameRegex ]]; then
	echo "File doesn't match the naming convention"
	exit
fi

echo "Setting the test directory"

tar -xzvf $submission -C ./test_dir
cp assig1_*.c out* *.sh Makefile ./test_dir
cd ./test_dir

# FILE=report.pdf
# if ! [[ -f "$FILE" ]]; then
# 	echo "Report does not exist"
# 	exit
# fi

echo "Executing the test cases"

pkill qemu-system-x86
pkill qemu-system-i386
make clean
make fs.img
make

#make qemu 
echo "Running..1"
./test_1_history.sh gethistory | grep -iE '^[0-9]+ (cat|echo|forktest|grep|init|kill|ln|ls|mkdir|rm|sh|stressfs|usertests|wc|assig1_2|assig1_4) [0-9]+' | sed 's/$ //g' | sort > res_assig1_1

echo "Running..2"
./test_2_block.sh block | grep -i 'Hello' | sed 's/$ //g' | sort > res_assig1_2

echo "Running..3"
./test_3_unblock.sh unblock | grep -i 'Hello' | sed 's/$ //g' | sort > res_assig1_3

echo "Running..4"
./test_4_chmod.sh chmod | grep -i 'Operation' | sed 's/$ //g' | sort > res_assig1_4

check_test=4
total_test=0

for ((t=1;t<=$check_test;++t))
do
	echo -n "Test #${t}: "

	# NOTE: we are doing case insensitive matching.  If this is not what you want,
	# just remove the "-i" flag
	if diff -i --strip-trailing-cr <(sort out_assig1_$t) <(sort res_assig1_$t) > /dev/null
	then
		echo -e "\e[0;32mPASS\e[0m"
		((total_test++))
	else
		echo -e "\e[0;31mFAIL\e[0m"
	fi
done
echo "$total_test" test cases passed



