#pragma once

#include <erebus/rtl/log.hxx>

namespace Er::Log
{

ER_RTL_EXPORT SinkPtr makeSyslogSink(const char* tag, FormatterPtr&& formatter, FilterPtr&& filter = FilterPtr{});


} // namespace Er::Log {}