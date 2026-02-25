#ifndef LIBC_LIMITS_H
#define LIBC_LIMITS_H

#define CHAR_BIT 8
#define CHAR_WIDTH CHAR_BIT

#define FLT_MAX 3.40282347e+38f
#define SHRT_MAX 32767.0f

#define SCHAR_WIDTH (CHAR_BIT)
#define SHORT_WIDTH (CHAR_BIT * sizeof(short))
#define INT_WIDTH   (CHAR_BIT * sizeof(int))
#define LONG_WIDTH  (CHAR_BIT * sizeof(long))
#define LLONG_WIDTH (CHAR_BIT * sizeof(long long))

#define SCHAR_MIN (-(1LL << (SCHAR_WIDTH - 1)))
#define SHORT_MIN (-(1LL << (SHORT_WIDTH - 1)))
#define INT_MIN   (-(1LL << (INT_WIDTH - 1)))
#define LONG_MIN  (-(1LL << (LONG_WIDTH - 1)))
#define LLONG_MIN (2 * -(1LL << (LLONG_WIDTH - 2)))

#define SCHAR_MAX (-(SCHAR_MIN + 1))
#define SHORT_MAX (-(SHORT_MIN + 1))
#define INT_MAX   (-(INT_MIN + 1))
#define LONG_MAX  (-(LONG_MIN + 1))
#define LLONG_MAX (-(LLONG_MIN + 1))

#define UCHAR_WIDTH  (CHAR_BIT)
#define USHRT_WIDTH  (CHAR_BIT * sizeof(unsigned short))
#define UINT_WIDTH   (CHAR_BIT * sizeof(unsigned int))
#define ULONG_WIDTH  (CHAR_BIT * sizeof(unsigned long))
#define ULLONG_WIDTH (CHAR_BIT * sizeof(unsigned long long))

#define UCHAR_MAX  ((unsigned char)-1)
#define USHRT_MAX  ((unsigned short)-1)
#define UINT_MAX   ((unsigned int)-1)
#define ULONG_MAX  ((unsigned long)-1)
#define ULLONG_MAX ((unsigned long long)-1)

#endif
