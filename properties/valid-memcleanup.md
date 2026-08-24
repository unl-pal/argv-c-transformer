All allocated memory is deallocated before the program terminates.
In addition to valid-memtrack: There exists no finite execution of the program on which the program terminates
but still points to allocated memory.
(Comparison to Valgrind: This property can be violated even if Valgrind reports 'still reachable'.)