#pragma once

#include <erebus/rtl/log.hxx>


namespace Er::Log
{

ER_RTL_EXPORT ISink::Ptr makeDebuggerSink(FormatterPtr&& formatter, FilterPtr&& filter = FilterPtr{});


} // namespace Er::Log {}