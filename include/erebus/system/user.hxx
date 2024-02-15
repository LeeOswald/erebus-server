#pragma once

#include <erebus/erebus.hxx>



namespace Er
{

namespace System
{

namespace CurrentUser
{

#if ER_LINUX
EREBUS_EXPORT bool root() noexcept;
#endif

EREBUS_EXPORT std::string name();

} // namespace CurrentUser {}

} // namespace System {}

} // namespace Er {}