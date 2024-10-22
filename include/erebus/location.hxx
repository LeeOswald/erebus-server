#pragma once

#include <erebus/stacktrace.hxx>

#include <source_location>


namespace Er
{

struct Location final
{
    std::optional<std::source_location> source;
    std::optional<StackTrace> stack;
    std::optional<DecodedStackTrace> decoded;

    Location() noexcept = default;

    Location(const std::source_location& source) noexcept
        : source(source)
    {}

    Location(const std::source_location& source, StackTrace&& stack) noexcept
        : source(source)
        , stack(std::move(stack))
    {
    }
};

} // namespace Er {}


#define ER_HERE() ::Er::Location(std::source_location::current(), ::Er::StackTrace(0, static_cast<std::size_t>(-1)))
#define ER_HERE2(skip, count) ::Er::Location(std::source_location::current(), ::Er::StackTrace(skip, count)))
#define ER_SOURCE() ::Er::Location(std::source_location::current())