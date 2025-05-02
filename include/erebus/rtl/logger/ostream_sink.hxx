#pragma once

#include <erebus/rtl/log.hxx>

namespace Er::Log
{

ER_RTL_EXPORT ISink::Ptr makeOStreamSink(std::ostream& stream, FormatterPtr&& formatter, FilterPtr&& filter = FilterPtr{});


} // namespace Er::Log {}