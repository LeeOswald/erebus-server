#pragma once


#include <erebus/log.hxx>
#include <erebus/util/generichandle.hxx>


namespace Er
{

namespace Private
{

class LogRotator
    : public Er::Log::LogBase
{
public:
    ~LogRotator() {}
    explicit LogRotator(Er::Log::Level level, const char* fileName);

private:
    const int kKeep = 3;
};


class Logger final
    : public LogRotator
{
public:
    ~Logger();
    explicit Logger(Er::Log::Level level, const char* fileName);

    bool valid() const noexcept
    {
        return m_file.valid();
    }

private:
#if ER_POSIX
    struct FileCloser
    {
        void operator()(int fd) noexcept
        {
            ::close(fd);
        }
    };

    using File = Util::GenericHandle<int, int, -1, FileCloser>;
#else
    struct FileCloser
    {
        void operator()(HANDLE fd) noexcept
        {
            ::CloseHandle(fd);
        }
    };

    using File = Util::GenericHandle<HANDLE, intptr_t, -1, FileCloser>;
#endif

    void delegate(std::shared_ptr<Er::Log::Record> r);

    File m_file;
};


} // namespace Private {}

} // namespace Er {}
