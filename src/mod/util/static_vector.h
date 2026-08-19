#ifndef __STATIC_VECTOR_H__
#define __STATIC_VECTOR_H__

#include <type_traits>
#include <new>

template <typename T, std::size_t N>
class static_vector {
private:
    static constexpr std::size_t MAX_SIZE = N;
    std::size_t _size = 0;

    using aligned_T = std::aligned_storage_t<sizeof(T), alignof(T)>;
    aligned_T arr[N];

public:
    ~static_vector() {
        for (std::size_t i = 0; i < _size; i++) {
            (*this)[i].~T();
        }
    }

    T& operator[](std::size_t idx) {
        return *std::launder(reinterpret_cast<T*>(&arr[idx]));
    }

    const T& operator[](std::size_t idx) const {
        return *std::launder(reinterpret_cast<const T*>(&arr[idx]));
    }

    template<typename... Args>
    void emplace_back(Args&... args) {
        new (&arr[_size]) T(args...);
        _size++;
    }

    std::size_t size() const {
        return _size;
    }

    constexpr std::size_t max_size() const {
        return MAX_SIZE;
    }
};

#endif /* __STATIC_VECTOR_H__ */