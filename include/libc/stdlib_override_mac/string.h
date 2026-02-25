#ifndef __STDLIB_OVERRIDE_MAC_STRING_H__
#define __STDLIB_OVERRIDE_MAC_STRING_H__

#define _STRING_H_
#include "libc/string.h"

#if __has_include_next(<string.h>)
#  include_next <string.h>
#endif

#endif
