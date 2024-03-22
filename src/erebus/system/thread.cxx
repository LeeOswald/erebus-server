#include <erebus/system/thread.hxx>

#if ER_POSIX
#include <pthread.h>
#endif


namespace Er
{

namespace System
{

namespace CurrentThread
{

EREBUS_EXPORT Tid id() noexcept
{
#if ER_POSIX
    return ::gettid();
#elif ER_WINDOWS
    return ::GetCurrentThreadId();
#endif
}

#if ER_WINDOWS

#pragma pack(push,8)
typedef struct
{
    DWORD dwType;      // Must be 0x1000.
    LPCSTR szName;     // Pointer to name (in user addr space).
    DWORD dwThreadID;  // Thread ID (-1=caller thread).
    DWORD dwFlags;     // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

const DWORD VCThreadNameMagic = 0x406D1388;

EREBUS_EXPORT void setName(const char* name)
{
    if (::IsDebuggerPresent())
    {
        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = name;
        info.dwThreadID = DWORD(-1);
        info.dwFlags = 0;

        __try
        {
            ::RaiseException(VCThreadNameMagic, 0, sizeof(info) / sizeof(DWORD), reinterpret_cast<DWORD_PTR*>(&info));
        }
        __except (EXCEPTION_CONTINUE_EXECUTION)
        {
        }
    }
}

#elif ER_LINUX

EREBUS_EXPORT void setName(const char* name)
{
    auto self = ::pthread_self();
    ::pthread_setname_np(self, name);
}

#endif


} // namespace CurrentThread {}

} // namespace System {}

} // namespace Er {}