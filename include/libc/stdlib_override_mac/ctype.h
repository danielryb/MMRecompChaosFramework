#ifndef __STDLIB_OVERRIDE_MAC_CTYPE_H__
#define __STDLIB_OVERRIDE_MAC_CTYPE_H__

#define _CTYPE_H_

#if __has_include_next(<ctype.h>)
#  include_next <ctype.h>
#endif

#endif
