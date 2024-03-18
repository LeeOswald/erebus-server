#pragma once

#include <erebus/erebus.hxx>


namespace Er
{
    
//
// wrapper that enables usage of string literals as template args
//

template <size_t N>
struct StringLiteral
{
    static constexpr size_t size = N;

    constexpr StringLiteral(const char (&str)[N])
    {
        std::copy_n(str, N, value);
    }

    char value[N];
};


template <StringLiteral L>
inline constexpr const char* fromStringLiteral() noexcept
{
    return L.value;
}


} // namespace Er {}
