#pragma once

#include <windows.h>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

using NTSTATUS = long;


#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

extern "C"
{

NTSYSAPI NTSTATUS NTAPI RtlGetVersion(PRTL_OSVERSIONINFOEXW VersionInformation);

} // extern "C" {}