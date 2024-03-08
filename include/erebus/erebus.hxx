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

EREBUS_EXPORT void initialize();
EREBUS_EXPORT void finalize();

class LibScope
    : public Er::NonCopyable
{
public:
    ~LibScope()
    {
        finalize();
    }

    LibScope()
    {
        initialize();
    }
};


struct EREBUS_EXPORT Bytes
    : public std::string
{
    using Base = std::string;

    Bytes() noexcept
    {}

    Bytes(const Bytes& b)
        : Base(b)
    {}

    Bytes(Bytes&& b) noexcept
        : Base(std::move(b))
    {}

    explicit Bytes(const std::string& b)
        : Base(b)
    {}

    explicit Bytes(std::string&& b) noexcept
        : Base(std::move(b))
    {}
};

} // namespace Er {}
