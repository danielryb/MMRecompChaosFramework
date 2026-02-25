#ifndef __STDLIB_OVERRIDE_MAC_MATH_H__
#define __STDLIB_OVERRIDE_MAC_MATH_H__

#define __MATH_H__
#include "libc/math.h"

#if __has_include_next(<math.h>)
#  include_next <math.h>
#endif

#endif
