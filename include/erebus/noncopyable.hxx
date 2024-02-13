#pragma once

#include <erebus/erebus.hxx>

namespace Er
{

class NonCopyable
{
public:
    ~NonCopyable() = default;
    constexpr NonCopyable() noexcept = default;

    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

} // namespace Er {}
