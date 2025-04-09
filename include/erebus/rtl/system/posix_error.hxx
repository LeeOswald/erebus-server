#pragma once

#include <erebus/rtl/result.hxx>
#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/property_info.hxx>

namespace Er::System
{
    
ER_RTL_EXPORT [[nodiscard]] std::string posixErrorToString(int code);

ER_RTL_EXPORT [[nodiscard]] std::optional<ResultCode> resultFromPosixError(int code) noexcept;

ER_RTL_EXPORT [[nodiscard]] Er::Exception makePosixException(std::source_location location, std::string&& message, int code);

} // namespace Er::System {}


namespace Er::ExceptionProps
{

extern ER_RTL_EXPORT const Er::PropertyInfo PosixError;

} // namespace Er::ExceptionProps {}


#define ErThrowPosixError(message, code) throw Er::System::makePosixException(std::source_location::current(), message, code)