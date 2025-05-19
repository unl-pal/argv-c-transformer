#!/bin/bash
set -e

echo "=================================== CMake ==================================="
cmake -B build -S . -G Ninja

echo "=================================== Compiling ==================================="
ninja -C build

set +e

echo "=================================== Reset Directories ==================================="
rm -r filteredFiles/*

echo "=================================== Using Resources ==================================="
clangResourceDir="$(/usr/bin/clang -print-resource-dir)"
echo "Using Resource Directory: $clangResourceDir"

echo "=================================== Run Filter ==================================="
./build/filter samples/Tester/full.c properties.config "${clangResourceDir}"
