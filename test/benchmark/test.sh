#!/bin/bash
echo -e "N_THREADS\tN_TRANSACTIONS\tTIME\tN_ABORTS\tPROCESS_READ_TIME\tPROCESS_WRITE_TIME\tPROCESS_VALIDATION_TIME\tPROCESS_OTHER_TIME\tCOMMIT_VALIDATION_TIME\tCOMMIT_OTHER_TIME\tWASTED_TIME" > results.txt

make

for (( i = 1; i < 13; i++ )); do
	cd ../intset
	make clean
	make NR_TASKLETS=$i
	cd ../benchmark
	
	for (( j = 0; j < 10; j++ )); do
		./launch >> results.txt
	done
done