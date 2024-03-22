#pragma once

#include <erebus/erebus.hxx>


namespace Er
{
    
namespace System
{

using Tid = uintptr_t;


namespace CurrentThread
{

EREBUS_EXPORT Tid id() noexcept;

EREBUS_EXPORT void setName(const char* name);


} // namespace CurrentThread {}

} // namespace System {}
    
} // namespace Er {}