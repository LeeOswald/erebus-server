#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/util/lock_file.hxx>
#if ER_POSIX
    #include <fcntl.h>
    #include <sys/file.h>
#elif ER_WINDOWS
    #include <erebus/rtl/util/utf16.hxx>
#endif


namespace Er::Util
{


LockFile::~LockFile()
{
#if ER_POSIX
    ::unlink(m_path.c_str());
    std::ignore = ::flock(m_file, LOCK_UN);

#elif ER_WINDOWS
    ::DeleteFileW(m_wpath.c_str());

#endif
}

LockFile::LockFile(const std::string& path)
    : m_path(path)
#if ER_WINDOWS
    , m_wpath(Er::Util::utf8ToUtf16(path))
#endif
#if ER_POSIX
    , m_file(::open(path.c_str(), O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH))
#elif ER_WINDOWS
    , m_file(::CreateFileW(m_wpath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0))
#endif
{
#if ER_POSIX
    if (m_file == -1)
    {
        throw Exception(std::source_location::current(), Error{ int(errno), PosixError }, ExceptionProperties::ObjectName(m_path));
    }

    if (::flock(m_file, LOCK_EX | LOCK_NB) == -1)
    {
        throw Exception(std::source_location::current(), Error{ int(errno), PosixError }, ExceptionProperties::ObjectName(m_path));
    }

 #elif ER_WINDOWS
    if (m_file == INVALID_HANDLE_VALUE)
    {
        throw Exception(std::source_location::current(), Error(::GetLastError(), Win32Error), ExceptionProperties::ObjectName(m_path));
    }
#endif
}

void LockFile::put(std::string_view data)
{
#if ER_POSIX
    ::ftruncate(m_file, 0);

    const ssize_t written = ::write(m_file, data.data(), data.length());

    if (written != static_cast<ssize_t>(data.length()))
    {
        throw Exception(std::source_location::current(), Error{ int(errno), PosixError }, ExceptionProperties::ObjectName(m_path));
    }

    ::fdatasync(m_file);

#elif ER_WINDOWS
    DWORD written = 0;
    ::WriteFile(m_file, data.data(), data.length(), &written, nullptr);
    if (written != static_cast<DWORD>(data.length()))
    {
        throw Exception(std::source_location::current(), Error(::GetLastError(), Win32Error), ExceptionProperties::ObjectName(m_path));
    }

    ::FlushFileBuffers(m_file);

#endif
}


} // namespace Er::Util {}
