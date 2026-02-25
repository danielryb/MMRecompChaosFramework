#include "libc/string.h"
#include "libc/errno.h"

int __errno = 0;

const void* memchr( const void* ptr, int ch, size_t count ) {
    const unsigned char* ptr_char = (const unsigned char*)ptr; 
    char to_find = (unsigned char)ch;
    for (size_t i = 0; i < count; i++) {
        if (ptr_char[i] == to_find) {
            return &ptr_char[i];
        }
    }
    return NULL;
}

int memcmp( const void* lhs, const void* rhs, size_t count ) {
    const unsigned char* lhs_char = (const unsigned char*)lhs;
    const unsigned char* rhs_char = (const unsigned char*)rhs; 
    for (size_t i = 0; i < count; i++) {
        unsigned char l = lhs_char[i];
        unsigned char r = rhs_char[i];
        if (l < r) { return -1; }
        if (l > r) { return 1; }
    }
    return 0;
}