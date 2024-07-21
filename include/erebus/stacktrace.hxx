#pragma once

#include <erebus/erebus.hxx>

#if ER_POSIX
#define BOOST_STACKTRACE_USE_BACKTRACE
#endif

#include <boost/stacktrace.hpp>

#include <vector>

namespace Er
{

struct StackTrace final
{
    explicit StackTrace(std::size_t skip, std::size_t count)
        : m_stack(skip, count)
    {
    }

    boost::stacktrace::stacktrace const& get() const noexcept
    {
        return m_stack;
    }

private:
    boost::stacktrace::stacktrace m_stack;
};

using DecodedStackTrace = std::vector<std::string>;

DecodedStackTrace EREBUS_EXPORT decodeStackTrace(const StackTrace& stack);


} // namespace Er {}

