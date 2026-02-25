//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <__config>
#include <__verbose_abort>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

_LIBCPP_BEGIN_NAMESPACE_STD

_LIBCPP_WEAK void __libcpp_verbose_abort(char const* format, ...) {
  // Write message to stderr. We do this before formatting into a
  // buffer so that we still get some information out if that fails.
  {
    va_list list;
    va_start(list, format);
    // std::vfprintf(stderr, format, list);
    va_end(list);
  }

  *(volatile int*)0 = 0;
  while (1);
}

_LIBCPP_END_NAMESPACE_STD
