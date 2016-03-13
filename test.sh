#!/bin/bash
rm result
for((integer = 1; integer <= 5; integer++))
do
    echo TEST$integer
    echo TEST$integer >> result
	./rdt_sim 1000 0.1 100 0.3 0.3 0.3 0 >> result 2>&1
done
