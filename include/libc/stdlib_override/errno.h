#ifndef __STDLIB_OVERRIDE_ERRNO_H__
#define __STDLIB_OVERRIDE_ERRNO_H__

#define _ERRNO_H
#include "libc/errno.h"

#if __has_include_next(<errno.h>)
#  include_next <errno.h>
#endif

#endif
