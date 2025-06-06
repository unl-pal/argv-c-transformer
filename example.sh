rm -rf ./build/*
cmake -B ./build/ -S . -G Ninja
ninja -C ./build/ example
./build/example samples/ -extra-arg=-fparse-all-comments -extra-arg=-resource-dir=/usr/lib64/llvm20/bin/../../../lib/clang/20 -extra-arg=-xc
