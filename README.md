tressette.h
===========

<img src="./matrixbastoni.png" width="200">


minimal tressette implementation in C with no heap allocations 

with low kernel mode usage (random number generator and `__builtint_memcpy` and `__builtin_memset` on linux)
kernel mode usage can be reduced to none by: 
- implementing a DOOM-style `__builtin_memcpy` and `__builtin_memset`
- having a source of randomness on embedded system

