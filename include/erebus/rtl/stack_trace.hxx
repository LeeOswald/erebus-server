#pragma once

#include <erebus/rtl/rtl.hxx>

#include <boost/stacktrace.hpp>

#include <functional>

namespace Er
{

using StackTrace = boost::stacktrace::stacktrace;

struct StackFrame
{
    enum
    {
        Normal,
        Unknown,
        Skipped
    } type;

    union
    {
        void const* address;
        std::size_t skipped;
    };

    std::string symbol;

    constexpr StackFrame() noexcept
        : type(Unknown)
        , address(0)
        , symbol()
    {}

    constexpr StackFrame(std::size_t skipped) noexcept
        : type(Skipped)
        , skipped(skipped)
        , symbol()
    {}

    constexpr StackFrame(void const* address, std::string&& symbol) noexcept
        : type(Normal)
        , address(address)
        , symbol(std::move(symbol))
    {}
};

using StackFrameCallback = std::function<void(const StackFrame&)>;

void filterStackTrace(const StackTrace& stack, StackFrameCallback&& filter);

} // namespace Er {}