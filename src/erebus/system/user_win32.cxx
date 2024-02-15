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

namespace CurrentUser
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

    auto e = ::GetLastError();
    auto message = Er::Util::win32ErrorToString(e);
    throw Er::Exception(ER_HERE(), "Failed to get current user name", Er::ExceptionProps::Win32ErrorCode(e), Er::ExceptionProps::DecodedError(std::move(message)));
}

} // namespace {}   


EREBUS_EXPORT std::string name()
{
    static auto userw = getUserName();
    static auto user = Er::Util::utf16To8bit(CP_UTF8, userw.data(), userw.length());
    return user;
}


} // namespace CurrentUser {}

} // namespace System {}

} // namespace Er {}