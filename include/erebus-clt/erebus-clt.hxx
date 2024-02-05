#pragma once

#include <erebus/erebus.hxx>
#include <erebus/exception.hxx>
#include <erebus/log.hxx>

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
    case ResultCode::InvalidArgument: stream << "Invalid argument"; break;
    case ResultCode::DeadlineExceeded: stream << "Deadline exceeded"; break;
    case ResultCode::NotFound: stream << "Not found"; break;
    case ResultCode::AlreadyExists: stream << "Already exists"; break;
    case ResultCode::PermissionDenied: stream << "Permission denied"; break;
    case ResultCode::Unauthenticated: stream << "Unauthenticated"; break;
    case ResultCode::ResourceExhausted: stream << "Resource exhausted"; break;
    case ResultCode::FailedPrecondition: stream << "Failed precondition"; break;
    case ResultCode::Aborted: stream << "Aborted"; break;
    case ResultCode::OutOfRange: stream << "Out of range"; break;
    case ResultCode::Unimplemented: stream << "Unimplemented"; break;
    case ResultCode::Internal: stream << "Internal"; break;
    case ResultCode::Unavailable: stream << "Unavailable"; break;
    case ResultCode::DataLoss: stream << "Data loss"; break;
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


struct UserInfo
{
    std::string name;

    UserInfo() noexcept = default;
    explicit UserInfo(std::string_view name)
        : name(name)
    {}
};

struct IStub
{
    virtual void addUser(std::string_view name, std::string_view password) = 0;
    virtual void removeUser(std::string_view name) = 0;
    virtual std::vector<UserInfo> listUsers() = 0;
    virtual void exit(bool restart) = 0;
    virtual Version version() = 0;

    virtual ~IStub() {}
};


EREBUSCLT_EXPORT void initialize();
EREBUSCLT_EXPORT void finalize();

class Scope
    : public boost::noncopyable
{
public:
    ~Scope()
    {
        finalize();
    }

    Scope()
    {
        initialize();
    }
};


struct Params
{
    Log::ILog* log = nullptr;
    std::string endpoint;
    bool ssl;
    std::string rootCA;
    std::string user;
    std::string password;

    Params() noexcept = default;

    explicit Params(
        Log::ILog* log,
        std::string_view endpoint,
        bool ssl,
        std::string_view rootCA,
        std::string_view user,
        std::string_view password
    )
        : log(log)
        , endpoint(endpoint)
        , ssl(ssl)
        , rootCA(rootCA)
        , user(user)
        , password(password)
    {
    }
};

EREBUSCLT_EXPORT std::shared_ptr<IStub> create(const Params& params);

} // namespace Client {}
    
} // namespace Er {}