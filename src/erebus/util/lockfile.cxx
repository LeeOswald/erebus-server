#include <erebus/exception.hxx>
#include <erebus/util/lockfile.hxx>
#if ER_POSIX
    #include <fcntl.h>
    #include <sys/file.h>

    #include <erebus/util/posixerror.hxx>
#elif ER_WINDOWS
    #include <erebus/util/utf16.hxx>
    #include <erebus/util/win32error.hxx>

    #include <erebus/util/win32error.hxx>
#endif


namespace Er
{

namespace Util
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
    . m_file(::CreateFileW(m_wpath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0))
#endif
{
#if ER_POSIX
    if (m_file == -1)
    {
        auto e = errno;
        throw Er::Exception(
            ER_HERE(), 
            "Failed to create the lockfile", 
            Er::ExceptionProps::FileName(m_path),
            Er::ExceptionProps::PosixErrorCode(e),
            Er::ExceptionProps::DecodedError(Er::Util::posixErrorToString(e))
        );
    }

    if (::flock(m_file, LOCK_EX | LOCK_NB) == -1)
    {
        auto e = errno;
        if (e == EWOULDBLOCK)
        {
            throw Er::Exception(
                ER_HERE(), 
                "Failed to acuire the lockfile", 
                Er::ExceptionProps::FileName(m_path),
                Er::ExceptionProps::ResultCode(int32_t(Er::Result::SharingViolation))
            );
        }

        throw Er::Exception(
            ER_HERE(), 
            "Failed to acquire the lockfile", 
            Er::ExceptionProps::FileName(m_path),
            Er::ExceptionProps::PosixErrorCode(e),
            Er::ExceptionProps::DecodedError(Er::Util::posixErrorToString(e))
        );
    }

 #elif ER_WINDOWS
    if (m_file == INVALID_HANDLE_VALUE)
    {
        auto e = ::GetLastError();
        if (e == ERROR_SHARING_VIOLATION)
        {
            throw Er::Exception(
                ER_HERE(), 
                "Failed to acuire the lockfile", 
                Er::ExceptionProps::FileName(m_path),
                Er::ExceptionProps::ResultCode(int32_t(Er::Result::SharingViolation))
            );
        }
        else
        {
            throw Er::Exception(
                ER_HERE(), 
                "Failed to create the lockfile", 
                Er::ExceptionProps::FileName(m_path),
                Er::ExceptionProps::Win32ErrorCode(e),
                Er::ExceptionProps::DecodedError(Er::Util::win32ErrorToString(e))
            );
        }
            
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
        throw Er::Exception(ER_HERE(), "Failed to write to the lockfile", Er::ExceptionProps::FileName(m_path));
    }

    ::fdatasync(m_file);

#elif ER_WINDOWS
    DWORD written = 0;
    ::WriteFile(m_file, data.data(), data.length(), &written, nullptr);
    if (written != static_cast<DWORD>(data.length()))
    {
        throw Er::Exception(ER_HERE(), "Failed to write to the lockfile", Er::ExceptionProps::FileName(m_path));
    }

    ::FlushFileBuffers(m_file);

#endif
}


} // namespace Util {}

} // namespace Er {}
