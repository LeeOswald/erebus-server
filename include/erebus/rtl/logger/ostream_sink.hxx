#pragma once

#include <erebus/rtl/log.hxx>

namespace Er::Log
{

ER_RTL_EXPORT SinkPtr makeOStreamSink(std::ostream& stream, FormatterPtr&& formatter, FilterPtr&& filter = FilterPtr{});


} // namespace Er::Log {}