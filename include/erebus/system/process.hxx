#pragma once

#include <erebus/erebus.hxx>



namespace Er
{

namespace System
{

using Pid = uintptr_t;


namespace CurrentProcess
{

EREBUS_EXPORT Pid id() noexcept; 
EREBUS_EXPORT std::string exe();

} // namespace CurrentProcess {}

} // namespace System {}

} // namespace Er {}