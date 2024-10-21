# ubinutils
Partial, tiny, and portable implementation of some binutils tools without any dependencies.

## Tools 
- partial implementation of `nm` (it only implements the most useful parts)
- `ar` clone with support for the following commands: `t`, `x`, `p`
- `size` clone which kinof works

## Supported Formats
- ELF32 and ELF64 
- COFF
- PE
- unix ar files with optional SysV/GNU extension

## Building
```shell
clang -o nm lib/*.c tools/nm.c
clang -o ar lib/*.c tools/ar.c
clang -o size lib/*.c tools/size.c
```
