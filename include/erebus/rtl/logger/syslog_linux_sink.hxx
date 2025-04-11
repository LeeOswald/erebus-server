#pragma once

#include <erebus/rtl/log.hxx>

namespace Er::Log
{

ER_RTL_EXPORT ISink::Ptr makeSyslogSink(const char* tag, IFormatter::Ptr formatter, Filter&& filter = Filter{});


} // namespace Er::Log {}