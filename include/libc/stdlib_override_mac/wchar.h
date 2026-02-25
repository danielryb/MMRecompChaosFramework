#ifndef __STDLIB_OVERRIDE_MAC_WCHAR_H__
#define __STDLIB_OVERRIDE_MAC_WCHAR_H__

#define _WCHAR_H_
#include "libc/wchar.h"
#include "libc/stdio.h"
#include "libc/time.h"

#if __has_include_next(<wchar.h>)
#  include_next <wchar.h>
#endif

#endif
