#ifndef __DECOMP_H__
#define __DECOMP_H__

#include <stdbool.h>

typedef void PlayState;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

typedef float f32;

#ifdef __cplusplus
extern "C" {
#endif

void Rand_Seed(u32 seed);
f32 Rand_ZeroOne(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __DECOMP_H__ */