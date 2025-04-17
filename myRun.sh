#!/bin/bash
set -e

echo "=================================== CMake ==================================="
cmake -B build -S . -G Ninja

echo "================================ Copy compile_commands ================================"
cp ./build/compile_commands.json ./compile_commands.json

echo "=================================== Compiling ==================================="
ninja -C build

set +e

echo "=================================== Reset Directories ==================================="
rm -r filteredFiles/*
rm -r preprocessed/*
rm -r benchmark/*

echo "=================================== Using Resources ==================================="
clangResourceDir=$(/usr/bin/clang -print-resource-dir)
echo "Using Resource Directory: $clangResourceDir"

echo "=================================== Run Filter ==================================="
./build/filter samples/Tester/ properties.config "$clangResourceDir"

echo "=================================== Run Transform ==================================="
./build/transform filteredFiles/ "$clangResourceDir"
