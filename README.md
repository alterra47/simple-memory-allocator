# simple-memory-allocator

Uses the UNIX system instruction sbrk() to manipulate the program break counter, inorder to allocate memory to the heap and manage it.

The instuction to compile our library is,

'''
gcc -o memalloc.so -fPIC -shared memalloc.c
'''

This library needs to preloaded for the following demonstration, the instruction is,

'''
 export LD_PRELOAD=$PWD/memalloc.so
'''

Now we can use any commands which access the memory to check the effectiveness of our memory allocater.

(You can unset the preloaded compiled library by using the instruction below)

'''
unset LD_PRELOAD
'''
