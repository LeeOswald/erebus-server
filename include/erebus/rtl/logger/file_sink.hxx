#pragma once

#include <erebus/rtl/log.hxx>

namespace Er::Log
{

ER_RTL_EXPORT ISink::Ptr makeFileSink(
    ThreadSafe mode, 
    std::string_view fileName,
    FormatterPtr&& formatter,
    unsigned logsToKeep, 
    std::uint64_t maxFileSize,
    Filter&& filter = Filter{}
);

} // namespace Er::Log {}