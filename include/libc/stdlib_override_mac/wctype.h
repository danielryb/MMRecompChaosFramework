#ifndef __STDLIB_OVERRIDE_MAC_WCTYPE_H__
#define __STDLIB_OVERRIDE_MAC_WCTYPE_H__

#define _WCTYPE_H_

#include "libc/wchar.h"

#if __has_include_next(<wctype.h>)
#  include_next <wctype.h>
#endif

#endif
