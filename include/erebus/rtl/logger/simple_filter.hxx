#pragma once

#include <erebus/rtl/log.hxx>


namespace Er::Log
{

ER_RTL_EXPORT [[nodiscard]] FilterPtr makeLevelFilter(Level min, Level max = Level::Fatal);

} // namespace Er::Log {}