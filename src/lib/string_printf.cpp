#include "string_printf.hpp"
#include "stdarg.h"
extern "C" {
    #include "PR/xstdio.h"
}

void* snprintf_pfn(void* dst, const char* fmt, size_t size) {
    std::string* str = reinterpret_cast<std::string*>(dst);

    str->append(fmt, size);

    return str;
}

std::string recomputils::string_printf(const char* fmt, ...) {
    std::string ret{};

    va_list args;
    va_start(args, fmt);

    _Printf(&snprintf_pfn, &ret, fmt, args);
    va_end(args);
    
    return ret;
}
