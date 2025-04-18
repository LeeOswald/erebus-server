#pragma once

#include <erebus/result.hxx>
#include <erebus/rtl/exception.hxx>


namespace Er::System
{
    
ER_RTL_EXPORT [[nodiscard]] std::string posixErrorToString(int code);

ER_RTL_EXPORT [[nodiscard]] std::optional<ResultCode> resultFromPosixError(int code) noexcept;

ER_RTL_EXPORT [[nodiscard]] Er::Exception makePosixException(std::source_location location, std::string&& message, int code);

} // namespace Er::System {}


namespace Er::ExceptionProps
{

constexpr std::string_view PosixError{ "posix_error" };

} // namespace Er::ExceptionProps {}


#define ErThrowPosixError(message, code) throw Er::System::makePosixException(std::source_location::current(), message, code)