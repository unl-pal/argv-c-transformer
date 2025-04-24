#!/bin/sh

echo "Keeping Dependencies"
mv ./build/_deps/ ./
echo "Clearing Build Folder"
rm -r ./build/*
mv ./_deps build/
