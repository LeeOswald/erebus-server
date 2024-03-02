#pragma once

#include <erebus/erebus.hxx>

#include <vector>

namespace Er
{

namespace System
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


EREBUS_EXPORT Info current();

#if ER_LINUX

EREBUS_EXPORT std::optional<Info> lookup(uid_t uid);
EREBUS_EXPORT std::vector<Info> enumerate();

#endif

} // namespace User {}

} // namespace System {}

} // namespace Er {}