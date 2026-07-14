#ifndef __MODDING_H__
#define __MODDING_H__

// Do not edit these defines. They use special section names that the recomp mod tool recognizes for specific modding functionality.

#ifdef __cplusplus
#   define EXTERNC extern "C"
#else
#   define EXTERNC
#endif

// The RECOMP_IMPORT has the following attributes:
//   noinline: Prevent import function definitions from being inlined, allowing the mod tool to find relocs.
//   weak: Allow import definitions to be in headers without triggering multiple definition errors.
#define RECOMP_IMPORT(mod, func) /* NULL */

#define RECOMP_EXPORT EXTERNC /* NULL */

#define RECOMP_PATCH EXTERNC /* NULL */

#define RECOMP_FORCE_PATCH EXTERNC /* NULL */

#define RECOMP_DECLARE_EVENT(func) void func;

#define RECOMP_CALLBACK(mod, event) /* NULL */

#define RECOMP_HOOK(func) /* NULL */

#define RECOMP_HOOK_RETURN(func) /* NULL */


#endif
