#!/bin/sh
# Builds and Runs each stage, expecting the configuration file as an argument

if [ -f "$1" ]; then
  configFile="$1"
  echo "Conifiguration File set to $1"
else
  echo "Configuration File Not Provided - Aborting"
  exit 1
fi

set -e

echo "=================================== CMake ==================================="
cmake -B build -S . -G Ninja

echo "=================================== Compiling ==================================="
ninja -C build filter transform

set +e

echo "=================================== Reset Directories ==================================="
rm -r filteredFiles/*
rm -r benchmark/*

set -e

echo "=================================== Using Resources ==================================="
clangResourceDir="$(clang -print-resource-dir)"
echo "Using Resource Directory: $clangResourceDir"

# echo "=================================== Run Download ==================================="
# python3 ./src/download/Downloader.py "$configFile"

echo "=================================== Run Filter ==================================="
./build/filter "$configFile"

echo "=================================== Run Transform ==================================="
./build/transform "$configFile"

# find benchmark -empty -delete
