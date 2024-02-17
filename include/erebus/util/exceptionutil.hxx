#pragma once

#include <erebus/exception.hxx>
#include <erebus/log.hxx>

namespace Er
{

namespace Util
{

EREBUS_EXPORT std::string formatException(const std::exception& e) noexcept;
EREBUS_EXPORT std::string formatException(const Er::Exception& e) noexcept;

void EREBUS_EXPORT logException(Log::ILog* log, Log::Level level, const std::exception& e) noexcept;
void EREBUS_EXPORT logException(Log::ILog* log, Log::Level level, const Er::Exception& e) noexcept;


} // namespace Util {}

} // namespace Er {}

