#pragma once

#include <erebus/erebus.hxx>
#include <erebus/exception.hxx>


#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef EREBUSCLT_EXPORTS
        #define EREBUSCLT_EXPORT __declspec(dllexport)
    #else
        #define EREBUSCLT_EXPORT __declspec(dllimport)
    #endif
#else
    #define EREBUSCLT_EXPORT __attribute__((visibility("default")))
#endif


namespace Er
{
    
namespace Client
{

// this one must match ResultCode from protocol/erebus.proto
enum class ResultCode : int32_t
{
    Success = 0,
    Failure = 1,
    Cancelled = 2,
    InvalidArgument = 3,
    DeadlineExceeded = 4,
    NotFound = 5,
    AlreadyExists = 6,
    PermissionDenied = 7,
    Unauthenticated = 8,
    ResourceExhausted = 9,
    FailedPrecondition = 10,
    Aborted = 11,
    OutOfRange = 12,
    Unimplemented = 13,
    Internal = 14,
    Unavailable = 15,
    DataLoss = 16,
    DoNotUse = -1
};

template <typename StreamT>
inline StreamT& operator<<(StreamT& stream, ResultCode code)
{
    switch (code)
    {
    case ResultCode::Success: stream << "Success"; break;
    case ResultCode::Failure: stream << "Failure"; break;
    case ResultCode::Cancelled: stream << "Cancelled"; break;
    case ResultCode::InvalidArgument: stream << "InvalidArgument"; break;
    case ResultCode::DeadlineExceeded: stream << "DeadlineExceeded"; break;
    case ResultCode::NotFound: stream << "NotFound"; break;
    case ResultCode::AlreadyExists: stream << "AlreadyExists"; break;
    case ResultCode::PermissionDenied: stream << "PermissionDenied"; break;
    case ResultCode::Unauthenticated: stream << "Unauthenticated"; break;
    case ResultCode::ResourceExhausted: stream << "ResourceExhausted"; break;
    case ResultCode::FailedPrecondition: stream << "FailedPrecondition"; break;
    case ResultCode::Aborted: stream << "Aborted"; break;
    case ResultCode::OutOfRange: stream << "OutOfRange"; break;
    case ResultCode::Unimplemented: stream << "Unimplemented"; break;
    case ResultCode::Internal: stream << "Internal"; break;
    case ResultCode::Unavailable: stream << "Unavailable"; break;
    case ResultCode::DataLoss: stream << "DataLoss"; break;
    default: stream << "???"; break;
    }
    return stream;
}

struct ResultCodeFormatter
{
    void operator()(const Property& v, std::ostream& s) { s << std::any_cast<ResultCode>(v.value); }
};


namespace Props
{

using ResultCode = PropertyInfo<ResultCode, ER_PROPID("erebus.ResultCode"), "Erebus error code", ResultCodeFormatter, int32_t>;

} // namespace Props {}


struct Version
{
    uint16_t major = 0;
    uint16_t minor = 0;
    uint16_t patch = 0;

    constexpr explicit Version(uint16_t major = 0, uint16_t minor = 0, uint16_t patch = 0) noexcept
        : major(major)
        , minor(minor)
        , patch(patch)
    {}
};

struct IStub
{
    virtual void exit(bool restart) = 0;
    virtual Version version() = 0;

    virtual ~IStub() {}
};


EREBUSCLT_EXPORT void initialize();
EREBUSCLT_EXPORT void finalize();

EREBUSCLT_EXPORT std::shared_ptr<IStub> create(const std::string& address);

} // namespace Client {}
    
} // namespace Er {}