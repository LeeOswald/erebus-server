#pragma once

#include "../../erebus-config.h"

#if !defined(ER_WINDOWS) && !defined(ER_LINUX)
    #error Unknown OS. Check your build configuration.
#endif


#if !defined(ER_64) && !defined(ER_32)
    #error Unknown platform bitness. Check your build configuration.
#endif

#if !defined(NDEBUG)
    #define ER_DEBUG 1
#else
    #define ER_DEBUG 0
#endif

#if ER_WINDOWS
    #ifdef ER_RTL_EXPORTS
        #define ER_RTL_EXPORT __declspec(dllexport)
    #else
        #define ER_RTL_EXPORT __declspec(dllimport)
    #endif
#else
    #define ER_RTL_EXPORT __attribute__((visibility("default")))
#endif

#define ER_PLATFORM_HXX_INCLUDED 1

// absolutely necessary system headers go here
#include <concepts>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
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

#include <erebus/result.hxx>


namespace Er
{

enum class ThreadSafe
{
    No,
    Yes
};

} // namespace Er {}


#if defined __clang__ || defined __GNUC__
    #define ER_PRETTY_FUNCTION __PRETTY_FUNCTION__
    #define ER_PRETTY_FUNCTION_PREFIX '='
    #define ER_PRETTY_FUNCTION_SUFFIX ']'
#elif defined _MSC_VER
    #define ER_PRETTY_FUNCTION __FUNCSIG__
    #define ER_PRETTY_FUNCTION_PREFIX '<'
    #define ER_PRETTY_FUNCTION_SUFFIX '>'
#endif