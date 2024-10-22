#pragma once

#include <erebus/erebus.hxx>

#if ER_POSIX
#define BOOST_STACKTRACE_USE_BACKTRACE
#endif

#include <boost/stacktrace.hpp>

#include <vector>

namespace Er
{

using StackTrace = boost::stacktrace::stacktrace;

using DecodedStackTrace = std::vector<std::string>;

DecodedStackTrace EREBUS_EXPORT decodeStackTrace(const StackTrace& stack);


} // namespace Er {}

