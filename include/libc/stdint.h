#ifndef LIBC_STDINT_H
#define LIBC_STDINT_H

typedef signed long long intmax_t;
typedef unsigned long long uintmax_t;

typedef unsigned char uint8_t;
typedef signed char int8_t;

typedef unsigned short uint16_t;
typedef signed short int16_t;

typedef unsigned int uint32_t;
typedef signed int int32_t;

typedef unsigned long long uint64_t;
typedef signed long long int64_t;

typedef signed long intptr_t;
typedef unsigned long uintptr_t;

typedef uint8_t uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;
typedef uint64_t uint_least64_t;
typedef int8_t int_least8_t;
typedef int16_t int_least16_t;
typedef int32_t int_least32_t;
typedef int64_t int_least64_t;

typedef uint8_t uint_fast8_t;
typedef uint16_t uint_fast16_t;
typedef uint32_t uint_fast32_t;
typedef uint64_t uint_fast64_t;
typedef int8_t int_fast8_t;
typedef int16_t int_fast16_t;
typedef int32_t int_fast32_t;
typedef int64_t int_fast64_t;

#define INT8_MIN    (-0x80)
#define INT16_MIN   (-0x8000)
#define INT32_MIN   (-0x80000000)
#define INT64_MIN   (-0x8000000000000000)

#define INT8_MAX    0x7F
#define INT16_MAX   0x7FFF
#define INT32_MAX   0x7FFFFFFF
#define INT64_MAX   0x7FFFFFFFFFFFFFFF

#define SIZE_MAX    ((1ULL << (sizeof(void*) * 8)) - 1)

#define UINT8_MAX   0xFF
#define UINT16_MAX  0xFFFF
#define UINT32_MAX  0xFFFFFFFF
#define UINT64_MAX  0xFFFFFFFFFFFFFFFF

#define INTPTR_MIN  (-0x80000000)
#define INTPTR_MAX  0x7FFFFFFF
#define UINTPTR_MAX 0xFFFFFFFF

#define INT8_C(x) ((int_least8_t)(x))
#define INT16_C(x) ((int_least16_t)(x))
#define INT32_C(x) ((int_least32_t)(x))
#define INT64_C(x) ((int_least64_t)(x))

#define INTMAX_C(x) ((intmax_t)(x))

#define UINT8_C(x) ((uint_least8_t)(x))
#define UINT16_C(x) ((uint_least16_t)(x))
#define UINT32_C(x) ((uint_least32_t)(x))
#define UINT64_C(x) ((uint_least64_t)(x))

#define UINTMAX_C(x) ((uintmax_t)(x))


#endif /* STDINT_H */
