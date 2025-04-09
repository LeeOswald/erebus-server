#pragma once

#include <erebus/rtl/log.hxx>

namespace Er::Log
{

ER_RTL_EXPORT ISink::Ptr makeOStreamSink(std::ostream& stream, IFormatter::Ptr&& formatter, Filter&& filter = Filter{});


} // namespace Er::Log {}