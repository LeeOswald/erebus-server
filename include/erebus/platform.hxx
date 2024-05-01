#pragma once

#if defined(_WIN32)
    #define ER_WINDOWS 1
    #define ER_LINUX 0
    #define ER_POSIX 0
#elif defined(__linux)
    #define ER_WINDOWS 0
    #define ER_LINUX 1
    #define ER_POSIX 1
#else
    #error Unsupported OS
#endif


#if !defined(NDEBUG)
    #define ER_DEBUG 1
#else
    #define ER_DEBUG 0
#endif

#define ER_32 (UINT32_MAX == UINTPTR_MAX)
#define ER_64 (UINT64_MAX == UINTPTR_MAX)

// often-used platform headers


#if ER_LINUX
    #ifndef _GNU_SOURCE
        #define _GNU_SOURCE
    #endif
#endif

#include <errno.h>
#include <signal.h>

#if ER_LINUX
    #include <sys/types.h>
    #include <unistd.h>
#endif

#if ER_WINDOWS
    #include <windows.h>
#endif

// often-used std headers
#include <algorithm>
#include <climits>
#include <concepts>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#ifndef _countof

    template<typename T, size_t N>
    constexpr size_t __countof_impl(T(&arr)[N]) noexcept
    {
        return std::extent<T[N]>::value;
    }

    #define _countof(a) __countof_impl(a)

#endif // _countof

#ifdef min
    #undef min
#endif

#ifdef max
    #undef max
#endif
