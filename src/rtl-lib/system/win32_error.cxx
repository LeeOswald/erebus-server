#include <erebus/rtl/util/auto_ptr.hxx>
#include <erebus/rtl/system/win32_error.hxx>
#include <erebus/rtl/util/utf16.hxx>



namespace Er::System
{
    
ER_RTL_EXPORT std::string win32ErrorToString(DWORD r, HMODULE module)
{
    if (r == 0)
        return std::string();

    Util::AutoPtr<wchar_t, decltype([](void* ptr) { ::HeapFree(::GetProcessHeap(), 0, ptr); })> buffer;
    auto cch = ::FormatMessageW(
        (module ? FORMAT_MESSAGE_FROM_HMODULE : FORMAT_MESSAGE_FROM_SYSTEM) | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        module,
        r,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
        reinterpret_cast<wchar_t*>(buffer.writeable()),
        0,
        nullptr
    );

    std::wstring s;

    if (cch > 0)
    {
        s = std::wstring(buffer.get(), cch);

        // Windows appends ".\r\n" to error messages for some reason
        while (s.size() && (s[s.size() - 1] == L'\n' || s[s.size() - 1] == L'\r' || s[s.size() - 1] == L'.'))
        {
            s.erase(s.size() - 1);
        }
    }

    return Util::utf16To8bit(CP_UTF8, s.data(), s.length());
}

ER_RTL_EXPORT std::optional<ResultCode> resultFromWin32Error(DWORD e) noexcept
{
    switch (e)
    {
    case ERROR_SUCCESS: return Result::Ok;
    case ERROR_NOT_ENOUGH_MEMORY: return Result::OutOfMemory;
    case ERROR_ACCESS_DENIED: return Result::AccessDenied;
    case ERROR_GEN_FAILURE: return Result::Failure;
    case ERROR_TIMEOUT: return Result::Timeout;
    case ERROR_OPERATION_ABORTED: return Result::Canceled;
    case ERROR_FILE_EXISTS: return Result::AlreadyExists;
    case ERROR_ALREADY_EXISTS: return Result::AlreadyExists;
    case ERROR_INVALID_PARAMETER: return Result::InvalidArgument;
    case ERROR_BAD_ARGUMENTS: return Result::InvalidArgument;
    case ERROR_NOT_SUPPORTED: return Result::Unsupported;
    case ERROR_INVALID_FUNCTION: return Result::Unsupported;
    case ERROR_NOT_FOUND: return Result::NotFound;
    case ERROR_FILE_NOT_FOUND: return Result::NotFound;
    case ERROR_BAD_NETPATH: return Result::NotFound;
    case ERROR_PATH_NOT_FOUND: return Result::NotFound;
    case ERROR_PROC_NOT_FOUND: return Result::NotFound;
    case ERROR_DISK_FULL: return Result::InsufficientResources;
    case ERROR_NO_SYSTEM_RESOURCES: return Result::InsufficientResources;
    case ERROR_SHARING_VIOLATION: return Result::SharingViolation;
    case ERROR_LOCK_VIOLATION: return Result::SharingViolation;
    case ERROR_DIRECTORY: return Result::NotFound;
    case ERROR_MOD_NOT_FOUND: return Result::NotFound;
    }

    return std::nullopt;
}


ER_RTL_EXPORT Exception makeWin32Exception(std::source_location location, std::string&& message, DWORD code)
{
    Exception x(location, std::move(message));
    
    auto result = resultFromWin32Error(code);
    if (result)
        x.add(Property(ExceptionProps::ResultCode, std::int32_t(*result)));

    x.add(Property(ExceptionProps::Win32Error, std::uint32_t(code)));

    auto decoded = win32ErrorToString(code);
    if (!decoded.empty())
        x.add(Property(ExceptionProps::DecodedError, std::move(decoded)));

    return x;
}



} // namespace Er::System {}

