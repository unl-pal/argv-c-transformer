All allocated memory blocks are tracked. The set of tracked blocks is defined as the
smallest set of blocks satisfying the following two rules:
- A block is tracked whenever there is a pointer to this block (not necessarily
  pointing to the beginning of the block) or to the first address after this block
  (see 6.5.6 of <a href="https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf">C11</a>
  standard) stored in a program variable. The variable can be of a pointer type or
  of a compound type containing a pointer. The variable does not have to be in the
  current scope, it can be global or on the call stack.
- If some pointer in a tracked block points to another block (again, not necessarily
  to the beginning of the block) or to the first address after this block, this pointed
  block is also tracked.

In particular, a leaked memory block is not tracked. Hence, a program with a memory leak does not satisfy this property.
