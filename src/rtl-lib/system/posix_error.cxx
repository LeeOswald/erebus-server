#include <erebus/rtl/system/posix_error.hxx>

#include <cstring>

namespace Er::System
{
    
ER_RTL_EXPORT std::string posixErrorToString(int e)
{
    constexpr size_t required = 256;
    char result[required];
    result[0] = 0;
    
#if ER_LINUX
    auto s = ::strerror_r(e, result, required); // 's' may be or not be the same as 'result'
    return std::string(s);
#elif ER_WINDOWS
    if (::strerror_s(result, e) == 0)
        return std::string(result);
#endif

    return std::string();
}

ER_RTL_EXPORT std::optional<ResultCode> resultFromPosixError(int code) noexcept
{
    switch (code)
    {
    case 0: return Result::Ok;
    case ENOMEM: return Result::OutOfMemory;
    case EPERM: return Result::AccessDenied;
    case EACCES: return Result::AccessDenied;
    case EEXIST: return Result::AlreadyExists;
    case EINVAL: return Result::InvalidArgument;
    case ENOENT: return Result::NotFound;
    case ENOSPC: return Result::InsufficientResources;
    case ENOTDIR: return Result::NotFound;
    }

    return std::nullopt;
}

ER_RTL_EXPORT Exception makePosixException(std::source_location location, std::string&& message, int code)
{
    Exception x(location, std::move(message));

    auto result = resultFromPosixError(code);
    if (result)
        x.add(Property(ExceptionProps::ResultCode, std::int32_t(*result)));
        
    x.add(Property(ExceptionProps::PosixError, std::int32_t(code)));

    auto decoded = posixErrorToString(code);
    if (!decoded.empty())
        x.add(Property(ExceptionProps::DecodedError, std::move(decoded)));

    return x;
}

} // namespace Er::System {}

