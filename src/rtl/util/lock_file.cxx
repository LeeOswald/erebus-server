#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/util/lock_file.hxx>
#if ER_POSIX
    #include <fcntl.h>
    #include <sys/file.h>

    #include <erebus/rtl/system/posix_error.hxx>
#elif ER_WINDOWS
    #include <erebus/rtl/system/win32_error.hxx>    
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
        ErThrowPosixError(Er::format("Failed to create the lockfile {}", m_path), errno);

    if (::flock(m_file, LOCK_EX | LOCK_NB) == -1)
        ErThrowPosixError(Er::format("Failed to acquire the lockfile {}", m_path), errno);

 #elif ER_WINDOWS
    if (m_file == INVALID_HANDLE_VALUE)
    {
        auto e = ::GetLastError();
        ErThrowWin32Error(Er::format("Failed to acquire the lockfile {}", m_path), e);
    }
#endif
}

void LockFile::put(std::string_view data)
{
#if ER_POSIX
    ::ftruncate(m_file, 0);

    const ssize_t written = ::write(m_file, data.data(), data.length());

    if (written != static_cast<ssize_t>(data.length()))
        ErThrow("Failed to write to the lockfile");

    ::fdatasync(m_file);

#elif ER_WINDOWS
    DWORD written = 0;
    ::WriteFile(m_file, data.data(), data.length(), &written, nullptr);
    if (written != static_cast<DWORD>(data.length()))
        ErThrow("Failed to write to the lockfile");

    ::FlushFileBuffers(m_file);

#endif
}


} // namespace Er::Util {}
