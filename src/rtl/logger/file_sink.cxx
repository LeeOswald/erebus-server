
#include <erebus/rtl/logger/file_sink.hxx>
#include <erebus/rtl/util/null_mutex.hxx>
#include <erebus/rtl/util/generic_handle.hxx>

#include <filesystem>
#include <mutex>

#if ER_POSIX
    #include <fcntl.h>
    #include <sys/file.h>

    #include <erebus/rtl/system/posix_error.hxx>
#endif

#if ER_WINDOWS
    #include <erebus/rtl/util/utf16.hxx>
    #include <erebus/rtl/system/win32_error.hxx>
#endif


namespace Er::Log
{

namespace
{

void rotateLogs(std::string_view logFileName, int keepCount)
{
    // rename my_log.log -> my_log.log.1, etc
    // deleting oldest files as necessary

    for (int i = keepCount - 1; i >= 0; --i)
    {
        std::string destName = std::string(logFileName) + std::string(".") + std::to_string(i);
        std::error_code ec;
        std::filesystem::remove(destName, ec);
        
        std::string srcName = std::string(logFileName);
        if (i > 0)
        {
            srcName.append(".");
            srcName.append(std::to_string(i - 1));
        }
        
        std::filesystem::rename(srcName, destName, ec);
        // no error handling here
        // we only care if log file creation fails
    }
}


template <class MutexT>
class FileSink
    : public SinkBase
{
public:
    ~FileSink()
    {
    }

    FileSink(std::string_view fileName, IFormatter::Ptr&& formatter, unsigned logsToKeep, std::uint64_t maxFileSize, Filter&& filter)
        : SinkBase(std::move(formatter), std::move(filter))
        , m_fileName(fileName)
        , m_logsToKeep(logsToKeep)
        , m_maxFileSize(maxFileSize)
    {
        rotateLogs(m_fileName, m_logsToKeep);
        makeLog();
    }

    void write(Record::Ptr r) override
    {
        if (!filter(r.get()))
            return;
        
        auto formatted = format(r.get());
        const auto available = formatted.length();
        if (!available)
            return;

        const auto data = formatted.data();

        std::unique_lock l(m_mutex);

        std::size_t written = 0;
        while (available > written)
        {
#if ER_POSIX
            auto wr = ::write(m_file, data + written, available - written);
            if (wr < 0)
            {
                if (errno == EINTR)
                    continue;

                ErThrowPosixError("Log file write failed", int(errno));
            }
            else
            {
                written += wr;
            }
        
#elif ER_WINDOWS
            DWORD wr = 0;
            if (!::WriteFile(m_file, data + written, available - written, &wr, nullptr))
            {
                ErThrowWin32Error("Log file write failed", ::GetLastError());
            }
            else
            {
                written += wr;
            }
#endif
        }

        m_currentSize += written;

        if (m_currentSize >= m_maxFileSize)
        {
            m_file.reset();
            rotateLogs(m_fileName, m_logsToKeep);
            makeLog();
        }
    }

    void write(AtomicRecord a) override
    {
        for (auto r : a)
            write(r);
    }

    void flush() override
    {
        std::unique_lock l(m_mutex);
#if ER_POSIX
        ::fdatasync(m_file);
#elif ER_WINDOWS
        ::FlushFileBuffers(m_file);
#endif
    }

private:
    void makeLog()
    {
#if ER_POSIX
        m_file.reset(::open(m_fileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH));
#elif ER_WINDOWS
        m_file.reset(::CreateFileW(Er::Util::utf8ToUtf16(m_fileName).c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0));
#endif

#if ER_POSIX
        if (!m_file.valid())
            ErThrowPosixError("Log file could not be created", int(errno));
#elif ER_WINDOWS
        if (m_file == INVALID_HANDLE_VALUE)
            ErThrowWin32Error("Log file could not be created", ::GetLastError());
#endif

        m_currentSize = 0;
    }

    const std::string m_fileName;
    const unsigned m_logsToKeep;
    const std::uint64_t m_maxFileSize;
    MutexT m_mutex;
    std::uint64_t m_currentSize = 0;
    Util::FileHandle m_file;
};


} // namespace {}

ER_RTL_EXPORT ISink::Ptr makeFileSink(
    ThreadSafe mode, 
    std::string_view fileName,
    IFormatter::Ptr&& formatter,
    unsigned logsToKeep, 
    std::uint64_t maxFileSize, 
    Filter&& filter
)
{
    if (mode == ThreadSafe::No)
        return std::make_shared<FileSink<Util::NullMutex>>(fileName, std::move(formatter), logsToKeep, maxFileSize, std::move(filter));
    else
        return std::make_shared<FileSink<std::mutex>>(fileName, std::move(formatter), logsToKeep, maxFileSize, std::move(filter));
}


} // namespace Er::Log {}