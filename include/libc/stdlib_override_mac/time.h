#ifndef __STDLIB_OVERRIDE_MAC_TIME_H__
#define __STDLIB_OVERRIDE_MAC_TIME_H__

#define _TIME_H_
#include "libc/time.h"

#if __has_include_next(<time.h>)
#  include_next <time.h>
#endif

#endif
