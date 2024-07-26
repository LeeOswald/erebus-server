#include <erebus/exception.hxx>
#include <erebus/util/utf16.hxx>
#include <erebus/util/win32error.hxx>
#include <erebus/system/user.hxx>


#define SECURITY_WIN32
#include <security.h>


namespace Er
{

namespace System
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
                return user;
        }
    }

    ErThrowWin32Error("Failed to get current user name", ::GetLastError());
}

} // namespace {}   

namespace User
{

EREBUS_EXPORT Info current()
{
    static auto namew = getUserName();
    static auto name = Er::Util::utf16To8bit(CP_UTF8, namew.data(), namew.length());
    
    Info user;
    user.name = name;
    return user;
}


} // namespace User {}

} // namespace System {}

} // namespace Er {}