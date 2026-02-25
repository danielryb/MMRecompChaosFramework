#ifndef LIBC_WCHAR_H
#define LIBC_WCHAR_H

#include "stddef.h"
#include "uchar.h"
#include "stdio.h"

#ifndef __cplusplus
    typedef unsigned short wchar_t;
#endif

typedef __WINT_TYPE__ wint_t;

#ifdef __cplusplus
extern "C" {
#endif

long wcstol( const wchar_t * __restrict__ str, wchar_t ** __restrict__ str_end, int base );
long long wcstoll( const wchar_t * __restrict__ str, wchar_t ** __restrict__ str_end, int base );
unsigned long wcstoul( const wchar_t * __restrict__ str, wchar_t ** __restrict__ str_end, int base );
unsigned long long wcstoull( const wchar_t * __restrict__ str, wchar_t ** __restrict__ str_end, int base );

float wcstof( const wchar_t* __restrict__ str, wchar_t** __restrict__ str_end );
double wcstod( const wchar_t* __restrict__ str, wchar_t** __restrict__ str_end );
long double wcstold( const wchar_t* __restrict__ str, wchar_t** __restrict__ str_end );

wchar_t* wcscpy( wchar_t* __restrict__ dest, const wchar_t* __restrict__ src );
// errno_t wcscpy_s( wchar_t* __restrict__ dest, rsize_t destsz, const wchar_t* __restrict__ src );

wchar_t *wcsncpy( wchar_t *__restrict__ dest, const wchar_t *__restrict__ src, size_t count );
// errno_t wcsncpy_s( wchar_t *__restrict__ dest, rsize_t destsz, const wchar_t *__restrict__ src, rsize_t count);

wchar_t *wcscat( wchar_t *__restrict__ dest, const wchar_t *__restrict__ src );
// errno_t wcscat_s( wchar_t *__restrict__ dest, rsize_t destsz, const wchar_t *__restrict__ src );

wchar_t *wcsncat( wchar_t *__restrict__ dest, const wchar_t *__restrict__ src, size_t count );
// errno_t wcsncat_s( wchar_t *__restrict__ dest, rsize_t destsz, const wchar_t *__restrict__ src, rsize_t count );

size_t wcsxfrm( wchar_t* __restrict__ dest, const wchar_t* __restrict__ src, size_t count );

size_t wcslen( const wchar_t *str );
size_t wcsnlen_s(const wchar_t *str, size_t strsz);
int wcscmp( const wchar_t* lhs, const wchar_t* rhs );
int wcsncmp( const wchar_t* lhs, const wchar_t* rhs, size_t count );
int wcscoll( const wchar_t *lhs, const wchar_t *rhs );
wchar_t *wcschr( const wchar_t *str, wchar_t ch );
wchar_t *wcsrchr( const wchar_t *str, wchar_t ch );
size_t wcsspn( const wchar_t* dest, const wchar_t* src );
size_t wcscspn( const wchar_t* dest, const wchar_t* src );
wchar_t *wcspbrk( const wchar_t *dest, const wchar_t *str );
wchar_t *wcsstr( const wchar_t *dest, const wchar_t *src );
wchar_t* wcstok( wchar_t*  __restrict__ str, const wchar_t* __restrict__ delim, wchar_t** __restrict__ ptr );
// wchar_t* wcstok_s( wchar_t* __restrict__ str, rsize_t* __restrict__ strmax, const wchar_t* __restrict__ delim, wchar_t** __restrict__ ptr);

wchar_t *wmemcpy(wchar_t *__restrict__ dest, const wchar_t *__restrict__ src, size_t count );
// errno_t wmemcpy_s( wchar_t *__restrict__ dest, rsize_t destsz, const wchar_t *__restrict__ src, rsize_t count );
wchar_t* wmemmove( wchar_t* dest, const wchar_t* src, size_t count );
// errno_t wmemmove_s( wchar_t *dest, rsize_t destsz, const wchar_t *src, rsize_t count);
int wmemcmp( const wchar_t* lhs, const wchar_t* rhs, size_t count );
wchar_t *wmemchr( const wchar_t *ptr, wchar_t ch, size_t count );
wchar_t* wmemset( wchar_t* dest, wchar_t ch, size_t count );

int swprintf( wchar_t* __restrict__ buffer, size_t bufsz, const wchar_t* __restrict__ format, ... );

#ifdef __cplusplus
}
#endif

#endif
