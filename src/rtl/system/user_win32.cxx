#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/system/win32_error.hxx>
#include <erebus/rtl/system/user.hxx>
#include <erebus/rtl/util/utf16.hxx>

#define SECURITY_WIN32
#include <security.h>


namespace Er::System
{

namespace
{

std::wstring getUserName()
{
    std::wstring user;
    ULONG length = 0;

    if (!::GetUserNameExW(NameSamCompatible, nullptr, &length))
    {
        if (::GetLastError() == ERROR_MORE_DATA)
        {
            user.resize(length);
            if (::GetUserNameExW(NameSamCompatible, user.data(), &length))
            {
                // trim trailing '\0'
                user.resize(std::wcslen(user.data()));
                return user;
            }
        }
    }

    ErThrowWin32Error("Failed to get current user name", ::GetLastError());
}

} // namespace {}   

namespace User
{

ER_RTL_EXPORT Info current()
{
    static auto namew = getUserName();
    static auto name = Er::Util::utf16To8bit(CP_UTF8, namew.data(), namew.length());
    
    Info user;
    user.name = name;
    return user;
}


} // namespace User {}

} // namespace Er::System {}