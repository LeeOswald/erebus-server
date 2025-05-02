#pragma once

#include <erebus/rtl/log.hxx>


namespace Er::Log
{

ER_RTL_EXPORT SinkPtr makeDebuggerSink(FormatterPtr&& formatter, FilterPtr&& filter = FilterPtr{});


} // namespace Er::Log {}