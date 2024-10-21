#include <erebus/exception.hxx>
#include <erebus/system/user.hxx>
#include <erebus/util/posixerror.hxx>

#include <pwd.h>

namespace Er
{

namespace System
{

namespace User
{

EREBUS_EXPORT std::string name(uid_t id)
{
    struct passwd* pw = ::getpwuid(id);
    if (pw)
        return std::string(pw->pw_name);

    return std::string();
}

EREBUS_EXPORT std::optional<Info> lookup(uid_t uid)
{
    static auto required = ::sysconf(_SC_GETPW_R_SIZE_MAX);
    if (required == -1)
        required = 4096;
        
    std::vector<char> buffer;
    buffer.resize(size_t(required));
    struct passwd pwd = {};
    struct passwd* out = nullptr;
    auto result = ::getpwuid_r(uid, &pwd, buffer.data(), buffer.size(), &out);
    if (!out)
    {
        if (result == 0)
            return std::nullopt;

        ErThrowPosixError(Er::format("Could not lookup user {}", uid), result);
    }

    Info info;
    info.name = out->pw_name ? out->pw_name : "";
    info.userId = uid;
    info.groupId = out->pw_gid;
    info.fullName = out->pw_gecos ? out->pw_gecos : "";
    info.homeDir = out->pw_dir ? out->pw_dir : "";
    info.shell = out->pw_shell ? out->pw_shell : "";

    return std::make_optional<Info>(std::move(info));
}

EREBUS_EXPORT Info current()
{
    auto info = lookup(::getuid());
    if (!info)
        ErThrow(Er::format("Could not lookup user {}", ::getuid()));

    return *info;
}

EREBUS_EXPORT std::vector<Info> enumerate()
{
    std::vector<Info> users;

    static auto required = ::sysconf(_SC_GETPW_R_SIZE_MAX);
    if (required == -1)
        required = 4096;
    
    std::vector<char> buffer;
    buffer.resize(size_t(required));
        
    ::setpwent();
    while (true) 
    {
        struct passwd pwd = {};
        struct passwd* out = nullptr;
        auto result = ::getpwent_r(&pwd, buffer.data(), buffer.size(), &out);
        if (!out)
        {
            if ((result == 0) || (result == ENOENT))
                break;

            ErThrowPosixError("Failed to enumerate users", result);
        }
        
        Info info;
        info.name = out->pw_name ? out->pw_name : "";
        info.userId = out->pw_uid;
        info.groupId = out->pw_gid;
        info.fullName = out->pw_gecos ? out->pw_gecos : "";
        info.homeDir = out->pw_dir ? out->pw_dir : "";
        info.shell = out->pw_shell ? out->pw_shell : "";

        users.push_back(std::move(info));
    }
    ::endpwent();

    return users;
}


} // namespace User {}

} // namespace System {}

} // namespace Er {}