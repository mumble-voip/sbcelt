#!/bin/bash

rm -rf results
mkdir -p results

cp ../bench-celt/test.dat .

# celt lib
for i in {0..9}
do
	../bench-celt/celt-bench > results/celt.${i}
done

# sbcelt lib
for i in {0..9}
do
	../bench-sbcelt/sbcelt-bench > results/sbcelt.${i}
done

./crunch.py
