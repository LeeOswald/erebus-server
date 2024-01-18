#pragma once

#include <erebus/erebus.hxx>

#include <boost/stacktrace.hpp>

#if defined(__GNUC__) && (__GNUC__ < 11)
    #include <experimental/source_location>

    namespace Er
    {
        using source_location = std::experimental::source_location;

    } // namespace Er {}

#else
    #include <source_location>

    namespace Er
    {
        using source_location = std::source_location;

    } // namespace Er {}

#endif


namespace Er
{

struct SourceLocation
{
    SourceLocation() noexcept = default;

    SourceLocation(Er::source_location&& source, boost::stacktrace::stacktrace&& stack)
        : source(std::move(source))
        , stack(std::move(stack))
    {}

    source_location source;
    boost::stacktrace::stacktrace stack;
};

}

#define ER_HERE() Er::SourceLocation(Er::source_location::current(), boost::stacktrace::stacktrace())

