#include <erebus/system/user.hxx>

#include <pwd.h>

namespace Er
{

namespace System
{

namespace CurrentUser
{

namespace
{

std::string getUserName()
{
    uid_t uid = ::geteuid();
    struct passwd* pw = ::getpwuid(uid);
    if (pw)
        return std::string(pw->pw_name);

    return std::getenv("USERNAME");
}

} // namespace {}   


EREBUS_EXPORT std::string name()
{
    static std::string user = getUserName();
    return user;
}

EREBUS_EXPORT bool root() noexcept
{
    return (::geteuid() == 0);
}

} // namespace CurrentUser {}

} // namespace System {}

} // namespace Er {}