#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "recomputils.h"

#ifdef DEBUG
#define debug_log(...) recomp_printf("DEBUG "__VA_ARGS__)
#else
#define debug_log(...) /* null */
#endif

#define warning(...) recomp_printf("WARNING " __VA_ARGS__)
#define error(...) recomp_printf("ERROR " __VA_ARGS__)

#endif /* __DEBUG_H__ */