#ifndef __STDLIB_OVERRIDE_STDLIB_H__
#define __STDLIB_OVERRIDE_STDLIB_H__

#define _STDLIB_H
#include "libc/stdlib.h"

#if __has_include_next(<stdlib.h>)
#  include_next <stdlib.h>
#endif

#endif
