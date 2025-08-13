#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "recomputils.h"

#ifdef DEBUG
#define debug_log(fmt, ...) recomp_printf("DEBUG " fmt "\n" __VA_OPT__(,) __VA_ARGS__)
#else
#define debug_log(...) /* null */
#endif

#define warning(fmt, ...) recomp_printf("WARNING " fmt "\n" __VA_OPT__(,) __VA_ARGS__)
#define error(fmt, ...) recomp_printf("ERROR " fmt "\n" __VA_OPT__(,) __VA_ARGS__)

#endif /* __DEBUG_H__ */