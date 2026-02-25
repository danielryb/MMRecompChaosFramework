#ifndef LIBC_STDIO_H
#define LIBC_STDIO_H

#include "stddef.h"

typedef struct _iobuf {
    int dummy;
} FILE;

#define EOF (-1)
#define WEOF (-1)

int remove(const char *filename);
size_t fwrite( const void* __restrict__ buffer, size_t size, size_t count, FILE* __restrict__ stream );

int fflush(FILE* stream);
int fputc(int c, FILE* stream);
int fputs(const char* __restrict__ s, FILE* __restrict__ stream);
int fprintf(FILE* __restrict__ stream, const char* __restrict__ format, ...);

extern FILE __stdout;
extern FILE __stderr;

#define stdout &__stdout
#define stderr &__stderr

#endif
