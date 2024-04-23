#pragma once

#include <erebus/erebus.hxx>


namespace Er
{

namespace System
{


//
// high-resolution timestamp
//

struct EREBUS_EXPORT Timestamp final
{
    int64_t value;

    constexpr explicit Timestamp(int64_t value = 0) noexcept
        : value(value)
    {}

    static Timestamp now() noexcept;
    static int64_t resolution() noexcept;
};


int64_t milliseconds(const Timestamp& start, const Timestamp& end) noexcept
{
    return (end.value - start.value) * 1000 / Timestamp::resolution();
}



} // namespace System {}

} // namespace Er {}