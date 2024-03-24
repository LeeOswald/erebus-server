#pragma once

#include <erebus/platform.hxx>

#include <erebus/noncopyable.hxx>

#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef EREBUS_EXPORTS
        #define EREBUS_EXPORT __declspec(dllexport)
    #else
        #define EREBUS_EXPORT __declspec(dllimport)
    #endif
#else
    #define EREBUS_EXPORT __attribute__((visibility("default")))
#endif


namespace Er
{

namespace Log
{
    struct ILog;
}

EREBUS_EXPORT void initialize(Er::Log::ILog* log);
EREBUS_EXPORT void finalize(Er::Log::ILog* log);

class LibScope
    : public Er::NonCopyable
{
public:
    ~LibScope()
    {
        finalize(m_log);
    }

    LibScope(Er::Log::ILog* log)
        : m_log(log)
    {
        initialize(log);
    }

private:
    Er::Log::ILog* const m_log;
};


//
// binary string for use with gRPC 'bytes' type
//

struct EREBUS_EXPORT Bytes final
{
    Bytes() noexcept
    {}

    Bytes(const Bytes& b)
        : m_bytes(b.m_bytes)
    {}

    Bytes(Bytes&& b) noexcept
        : m_bytes(std::move(b.m_bytes))
    {}

    explicit Bytes(const std::string& b)
        : m_bytes(b)
    {}

    explicit Bytes(std::string&& b) noexcept
        : m_bytes(std::move(b))
    {}

    Bytes& operator=(const Bytes& o)
    {
        Bytes tmp(o);
        m_bytes.swap(tmp.m_bytes);
        return *this;
    }

    Bytes& operator=(Bytes&& o) noexcept
    {
        Bytes tmp(std::move(o));
        m_bytes.swap(tmp.m_bytes);
        return *this;
    }

    friend auto operator==(const Bytes& a, const Bytes& b) noexcept
    {
        return a.m_bytes == b.m_bytes;
    }

    friend auto operator<=>(const Bytes& a, const Bytes& b) noexcept
    {
        return a.m_bytes <=> b.m_bytes;
    }

    const std::string& bytes() const noexcept
    {
        return m_bytes;
    }

    std::string& bytes() noexcept
    {
        return m_bytes;
    }

    char* data() noexcept
    {
        return m_bytes.data();
    }

    const char* data() const noexcept
    {
        return m_bytes.data();
    }

    std::size_t size() const noexcept
    {
        return m_bytes.size();
    }

    bool empty() const noexcept
    {
        return m_bytes.empty();
    }

private:
    std::string m_bytes;
};


template <typename T>
auto saturatingSub(T a, T b) noexcept 
{
   return a > b ? a - b : 0;
}

} // namespace Er {}
