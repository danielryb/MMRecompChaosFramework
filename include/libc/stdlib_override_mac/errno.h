#ifndef __STDLIB_OVERRIDE_MAC_ERRNO_H__
#define __STDLIB_OVERRIDE_MAC_ERRNO_H__

#define _SYS_ERRNO_H_
#include "libc/errno.h"

#if __has_include_next(<errno.h>)
#  include_next <errno.h>
#endif

#endif
