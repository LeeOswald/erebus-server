#pragma once

#include <erebus/erebus.hxx>


namespace Er
{

struct EREBUS_EXPORT Empty
{
    constexpr Empty() noexcept = default;

    friend constexpr bool operator==(const Empty& a, const Empty& b) noexcept
    {
        return true;
    }

    friend constexpr auto operator<=>(const Empty& a, const Empty& b) noexcept
    {
        return std::strong_ordering::equal;
    }
};


} // namespace Er {}