#ifndef __STDLIB_OVERRIDE_MAC_LIMITS_H__
#define __STDLIB_OVERRIDE_MAC_LIMITS_H__

#define _LIMITS_H_
#include "libc/limits.h"

#if __has_include_next(<limits.h>)
#  include_next <limits.h>
#endif

#endif
