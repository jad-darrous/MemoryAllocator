# Memory Allocator
A simple Memory Allocator for pedagogic purpose.

This project implements a simple Memory Allocator that can be used instead the default libc malloc/free methods to run real applications.

The goal is not to build the fastest, most memory efficient or the best multi-threaded allocator. The goal is just to have fun and understand more about the operating system's memory allocator.

## Architecture

The basic types of block are *free block* and *busy block*.
Free block contains two fields; the size in bytes of the block and a pointer to the next free block.
Busy block consist just of it size.

The allocator maintains a single linked list of free blocks.

The allocator can be configured to use on of three allocation strategies:
* First fit: uses the free block is the list that is big enough to reserve the required bytes.
* Best fit: Select the free block that cause the minimum waste.
* Worst fit: Select the free block that cause the maximum waste.

The actual used memory is a static array of `char*` with size of `MEMORY_SIZE` that can be defined in the make file.

The time complexity of the allocation is O(n) because the list should be scanned to find the suitable free block. For freeing the memory, the complexity is also O(n) because the list should be scanned to find surrounding free blocks of the about to free busy block.

Corrupting the allocator meta-data (the headers of free and busy blocks) is hard to detect and fix. For now, freeing at wrong address can be detected.

Printing the information of the busy blocks when a program is about to exit is a nice feature to detect memory leak! calling the printing method by hand doesn't work with exited programs. I tried to use the `atexit` to register a handler from the shared library but it did not work.


## Running the code

No external dependencies are needed to run the code. Just a working version of **gcc** which come by default with any Linux/GNU distribution.

After cloning the project, go to `src` directory.

To make sure that everything works well, a collection of simple test are provided.
```
make test
```

To run an interactive shell for the allocator

```
make mem_shell
./mem_shell
```

For a more interesting stuff, we will run the allocator with an existing programs, in the example we will use the `ls` command. To do this we will use the `LD_PRELOAD` trick.

```
make test_ls
```

The idea is to build a shared library of the allocator and tell the OS to load this library before loading anything else, even before loading the libc. In this case the default malloc/free methods will be hidden by ours.

## Performance test

Even though the performance is not a goal but it is nice to a test and see how far my allocator is from the default one.

Now move to the `performance` directory, compile the provided code, and run it twice (don't forget to copy the `libmalloc.so` from the other directory)

```
gcc stress_test.c -o stress_test
echo "Default allocator"
time ./stress_test
echo "Our allocator"
time LD_PRELOAD=./libmalloc.so ./stress_test
LD_PRELOAD=""
```
