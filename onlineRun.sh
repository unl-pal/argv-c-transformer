#!/bin/sh

python3 ./src/download/Downloader.py

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

# set -e

echo "=================================== Using Resources ==================================="
clangResourceDir="$(clang -print-resource-dir)"
echo "Using Resource Directory: $clangResourceDir"

echo "=================================== Run Filter ==================================="
./build/filter properties.config

echo "=================================== Run Transform ==================================="
./build/transform properties.config
