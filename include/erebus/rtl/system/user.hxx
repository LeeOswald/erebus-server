#pragma once

#include <erebus/rtl/rtl.hxx>

#include <vector>

namespace Er::System
{


namespace User
{

struct Info
{
    std::string name;
#if ER_LINUX
    uid_t userId = uid_t(-1);
    gid_t groupId = gid_t(-1);
    std::string fullName;
    std::string homeDir;
    std::string shell;
#endif
};


ER_RTL_EXPORT Info current();

#if ER_LINUX

ER_RTL_EXPORT std::optional<Info> lookup(uid_t uid);
ER_RTL_EXPORT std::vector<Info> enumerate();

#endif

} // namespace User {}

} // namespace Er::System {}