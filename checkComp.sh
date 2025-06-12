#!/bin/sh

for file in *.c; do
  clang -fsyntax-only verifier.c "$file"
done

if [ -f "$1" ]; then
  echo "Running for ${1}"
  clang -fsyntax-only verifier.c "$1"
  exit 0
elif [ -d "$1" ]; then
  echo "Running for Dir: ${1}"
  echo "If only the <name> prints then the compilation is a SUCESS"
  find "$1" -name "*.c" -type f -exec echo "Checking for Errors in: " {} \; -exec clang -fsyntax-only verifier.c {} \;
  # for item in "$1"/*; do
  #   echo "$item"
  #   if [ -f "$item" ]; then
  #     echo "Running for ${item}"
  #     clang -fsyntax-only verifier.c "$1"
  #   fi
  # done
  exit 0
fi
