#!/bin/bash

for file in *.i; do
  gcc -E -P "$file" > "${file%c}"i
done

BENCHMARKS="/home/nat/Repos/bench-defs/sv-benchmarks/c/argc-benchmarks/"

cp ./*.yml "$BENCHMARKS"
cp ./*.c "$BENCHMARKS"
cp ./*.i "$BENCHMARKS"
cp ./*.set "$BENCHMARKS"
