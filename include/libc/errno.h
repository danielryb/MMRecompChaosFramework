#ifndef LIBC_ERRNO_H
#define LIBC_ERRNO_H

extern int __errno;
#define errno __errno

#endif
