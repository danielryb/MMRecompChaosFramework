#include <cstddef>
#include "recomputils.h"

void* operator new(size_t size) {
    return recomp_alloc(size ? size : 1);
}

void* operator new[](size_t size) {
    return recomp_alloc(size ? size : 1);
}

void operator delete(void* ptr) noexcept {
    if (ptr) {
        recomp_free(ptr);
    }
}

void operator delete(void* ptr, std::size_t size) noexcept {
    if (ptr) {
        recomp_free(ptr);
    }
}

void operator delete[](void* ptr) noexcept {
    if (ptr) {
        recomp_free(ptr);
    }
}

void operator delete[](void* ptr, std::size_t size) noexcept {
    if (ptr) {
        recomp_free(ptr);
    }
}
