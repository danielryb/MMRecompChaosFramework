#ifndef __FINITE_VECTOR_H__
#define __FINITE_VECTOR_H__

#include <type_traits>
#include <memory>
#include <new>

template <typename T>
class finite_vector {
private:
    const std::size_t max_size_;
    std::size_t size_ = 0;

    using aligned_T = std::aligned_storage_t<sizeof(T), alignof(T)>;
    std::unique_ptr<aligned_T[]> arr;

public:
    finite_vector(std::size_t max_size)
    : max_size_(max_size), arr(std::make_unique<aligned_T[]>(max_size)) {}

    T& operator[](std::size_t idx) {
        return *std::launder(reinterpret_cast<T*>(&arr[idx]));
    }

    const T& operator[](std::size_t idx) const {
        return *std::launder(reinterpret_cast<const T*>(&arr[idx]));
    }

    template<typename... Args>
    void emplace_back(Args&... args) {
        new (&arr[size_]) T(args...);
        size_++;
    }

    std::size_t size() const {
        return size_;
    }

    std::size_t max_size() const {
        return max_size;
    }
};

#endif /* __FINITE_VECTOR_H__ */
