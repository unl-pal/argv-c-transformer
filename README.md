# Dependencies
openssl-devel
llvm-devel
clang-devel

test/Index/binop.cpp
61:// CHECK: BinaryOperator=.* BinOp=.* 1
62:// CHECK: BinaryOperator=->* BinOp=->* 2
63:// CHECK: BinaryOperator=* BinOp=* 3
64:// CHECK: BinaryOperator=/ BinOp=/ 4
65:// CHECK: BinaryOperator=% BinOp=% 5
66:// CHECK: BinaryOperator=+ BinOp=+ 6
67:// CHECK: BinaryOperator=- BinOp=- 7
68:// CHECK: BinaryOperator=<< BinOp=<< 8
69:// CHECK: BinaryOperator=>> BinOp=>> 9
70:// CHECK: BinaryOperator=< BinOp=< 11
71:// CHECK: BinaryOperator=> BinOp=> 12
72:// CHECK: BinaryOperator=<= BinOp=<= 13
73:// CHECK: BinaryOperator=>= BinOp=>= 14
74:// CHECK: BinaryOperator=== BinOp=== 15
75:// CHECK: BinaryOperator=!= BinOp=!= 16
76:// CHECK: BinaryOperator=& BinOp=& 17
77:// CHECK: BinaryOperator=^ BinOp=^ 18
78:// CHECK: BinaryOperator=| BinOp=| 19
79:// CHECK: BinaryOperator=&& BinOp=&& 20
80:// CHECK: BinaryOperator=|| BinOp=|| 21
81:// CHECK: BinaryOperator== BinOp== 22
82:// CHECK: CompoundAssignOperator=*= BinOp=*= 23
83:// CHECK: CompoundAssignOperator=/= BinOp=/= 24
84:// CHECK: CompoundAssignOperator=%= BinOp=%= 25
85:// CHECK: CompoundAssignOperator=+= BinOp=+= 26
86:// CHECK: CompoundAssignOperator=-= BinOp=-= 27
87:// CHECK: CompoundAssignOperator=<<= BinOp=<<= 28
88:// CHECK: CompoundAssignOperator=>>= BinOp=>>= 29
89:// CHECK: CompoundAssignOperator=&= BinOp=&= 30
90:// CHECK: CompoundAssignOperator=^= BinOp=^= 31
91:// CHECK: CompoundAssignOperator=|= BinOp=|= 32
92:// CHECK: BinaryOperator=, BinOp=, 33

May have to update your path to use clang include
export PATH=/usr/lib/clang/19/include/:$PATH
to compile the asts without errors on includes from the c file standard library
headers

If expanding to use other headers then I will need to figure out ohow to add
those headers included in the project to the compiler as well
