#!/bin/bash

for file in *.i; do
  gcc -E -P "${file%i}c" > "$file"
done

BENCHMARKS="/home/nat/Repos/bench-defs/sv-benchmarks/c/argc-benchmarks/"

cp -- *.yml *.c *.i *.set "$BENCHMARKS"
