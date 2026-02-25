#ifndef __STDLIB_OVERRIDE_MAC_STDIO_H__
#define __STDLIB_OVERRIDE_MAC_STDIO_H__

#define _STDIO_H_
#include "libc/stdio.h"

#if __has_include_next(<stdio.h>)
#  include_next <stdio.h>
#endif

#endif
