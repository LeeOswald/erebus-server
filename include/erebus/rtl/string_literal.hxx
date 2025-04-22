#pragma once

#include <erebus/rtl/rtl.hxx>


namespace Er
{


template <std::size_t N>
struct StringLiteral 
{
    constexpr StringLiteral(const char(&str)[N]) noexcept
    {
        std::copy_n(str, N, value);
    }

    constexpr char const* data() const noexcept
    {
        return value;
    }

    constexpr std::size_t size() const noexcept
    {
        return N - 1; // length w/out '\0'
    }

    char value[N];
};


} // namespace Er {}