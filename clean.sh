#!/bin/sh

mv ./build/_deps/ ./
rm -r ./build/*
mv ./_deps build/
# rm -r ./bin/
# rm -r ./database/*.ast
rm -r ./filteredFiles/*
rm -r ./preprocessed/*
rm -r ./benchmark/*

# cp -r ./samples/OG/* ./samples/Tester/
