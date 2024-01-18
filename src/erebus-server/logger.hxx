#pragma once


#include <erebus/log.hxx>
#include <erebus/util/generichandle.hxx>


namespace Er
{

namespace Private
{


class Logger final
    : public Er::Log::ILog
    , public boost::noncopyable
{
public:
    ~Logger();
    explicit Logger(Er::Log::Level level, const char* fileName);

    Er::Log::Level level() const noexcept override;
    bool writev(Er::Log::Level level, const char* format, va_list args) noexcept override;
    bool write(Er::Log::Level level, const char* format, ...) noexcept override;
    bool write(Er::Log::Level l, std::string_view s) noexcept override;

    bool exclusive() const noexcept;

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

    File m_file;
    Er::Log::Level m_level = Er::Log::Level::Info;
    bool m_exclusive = true;
};


} // namespace Private {}

} // namespace Er {}
