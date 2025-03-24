# Dependencies
llvm-devel
clang-devel

May have to update your path to use clang include
export PATH=/usr/lib/clang/19/include/:$PATH
to compile the asts without errors on includes from the c file standard library
headers

If expanding to use other headers then I will need to figure out ohow to add
those headers included in the project to the compiler as well
