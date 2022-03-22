#!/bin/bash
echo -e "N_THREADS\tN_TRANSACTIONS\tTIME\tN_ABORTS" > results.txt

make

for (( i = 1; i < 3; i++ )); do
	cd ../bank
	make clean
	make NR_TASKLETS=$i
	cd ../benchmark
	
	for (( j = 0; j < 1; j++ )); do
		./launch >> results.txt
	done
done