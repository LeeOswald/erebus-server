#pragma once

#include <erebus/erebus.hxx>


namespace Er
{

namespace Util
{

EREBUS_EXPORT std::string formatv(const char* format, va_list args);
EREBUS_EXPORT std::string format(const char* format, ...);

} // namespace Util {}

} // namespace Er {}

