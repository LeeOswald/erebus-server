#pragma once

#include <erebus/sourcelocation.hxx>
#include <erebus/stacktrace.hxx>

namespace Er
{

struct Location final
{
    std::optional<SourceLocation> source;
    std::optional<StackTrace> stack;
    std::optional<DecodedStackTrace> decoded;

    Location() noexcept = default;

    Location(SourceLocation&& source) noexcept
        : source(std::move(source))
    {}

    Location(SourceLocation&& source, StackTrace&& stack) noexcept
        : source(std::move(source))
        , stack(std::move(stack))
    {
    }

    Location(SourceLocation&& source, DecodedStackTrace&& decoded) noexcept
        : source(std::move(source))
        , decoded(std::move(decoded))
    {
    }
};

} // namespace Er {}


#define ER_HERE() ::Er::Location(std::source_location::current(), ::Er::StackTrace(0, static_cast<std::size_t>(-1)))
#define ER_HERE2(skip, count) ::Er::Location(std::source_location::current(), ::Er::StackTrace(skip, count)))
#define ER_SOURCE() ::Er::Location(std::source_location::current())