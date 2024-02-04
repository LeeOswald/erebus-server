#pragma once

#include <erebus/erebus.hxx>

#include <boost/stacktrace.hpp>

#include <vector>

namespace Er
{

using StackTrace = boost::stacktrace::stacktrace;

using DecodedStackTrace = std::vector<std::string>;

DecodedStackTrace EREBUS_EXPORT decodeStackTrace(const StackTrace& stack);


} // namespace Er {}

