#pragma once

#include <erebus/rtl/result.hxx>
#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/system/unwindows.h>


namespace Er::System
{

namespace __
{

constexpr inline HRESULT facilityFromHRESULT(HRESULT x)
{
    return (x >> 16) & 0xFFF;
}

constexpr inline UINT win32ErrorFromHRESULT(HRESULT x)
{
    return static_cast<UINT>(x & 0xFFFF);
}

} // namespace __ {}


[[nodiscard]] ER_RTL_EXPORT std::string win32ErrorToString(DWORD e, HMODULE module = 0);

[[nodiscard]] ER_RTL_EXPORT std::optional<ResultCode> resultFromWin32Error(DWORD e) noexcept;

[[nodiscard]] inline std::optional<ResultCode> resultFromHRESULT(HRESULT e) noexcept
{
    if (__::facilityFromHRESULT(e) == FACILITY_WIN32)
        return resultFromWin32Error(__::win32ErrorFromHRESULT(e));

    return std::nullopt;
}


ER_RTL_EXPORT [[nodiscard]] Exception makeWin32Exception(std::source_location location, std::string&& message, DWORD code);

} // namespace Er::System {}


namespace Er::ExceptionProps
{

constexpr std::string_view Win32Error{ "win32_error" };

} // namespace Er::ExceptionProps {}


#define ErThrowWin32Error(message, code) \
    throw Er::System::makeWin32Exception(std::source_location::current(), message, code)