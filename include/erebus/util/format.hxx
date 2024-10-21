#pragma once

#include <erebus/erebus.hxx>


namespace Er
{

namespace Util
{

EREBUS_EXPORT std::string cformatv(const char* format, va_list args);
EREBUS_EXPORT std::string cformat(const char* format, ...);

} // namespace Util {}

} // namespace Er {}

