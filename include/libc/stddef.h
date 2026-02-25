#ifndef LIBC_STDDEF_H
#define LIBC_STDDEF_H

typedef signed int ptrdiff_t;

// typedef double max_align_t;

#if !defined(_SIZE_T)
#define _SIZE_T
#if defined(_MIPS_SZLONG) && (_MIPS_SZLONG == 64)
typedef unsigned long size_t;
#else
typedef unsigned int  size_t;
#endif
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#define offsetof(structure, member) __builtin_offsetof (structure, member)

#endif /* STDDEF_H */
