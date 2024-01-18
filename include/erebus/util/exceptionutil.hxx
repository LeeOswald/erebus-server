#pragma once

#include <erebus/exception.hxx>
#include <erebus/log.hxx>

namespace Er
{

namespace Util
{


std::string EREBUS_EXPORT formatException(const std::exception& e) noexcept;
std::string EREBUS_EXPORT formatException(const Er::Exception& e) noexcept;

void EREBUS_EXPORT logException(Log::ILog* log, Log::Level level, const std::exception& e) noexcept;
void EREBUS_EXPORT logException(Log::ILog* log, Log::Level level, const Er::Exception& e) noexcept;


} // namespace Util {}

} // namespace Er {}

