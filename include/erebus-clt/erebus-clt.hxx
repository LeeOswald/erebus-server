#pragma once

#include <erebus/exception.hxx>
#include <erebus/log.hxx>
#include <erebus/propertybag.hxx>

#include <vector>

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


struct IClient
{
    using SessionId = uint32_t;

    virtual SessionId beginSession(std::string_view request) = 0;
    virtual void endSession(std::string_view request, SessionId id) = 0;
    virtual Er::PropertyBag request(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId = std::nullopt) = 0;
    virtual std::vector<Er::PropertyBag> requestStream(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId = std::nullopt) = 0;

    virtual ~IClient() {}
};

struct LibParams
{
    Er::Log::ILog* log = nullptr;
    Er::Log::Level level = Log::Level::Debug;

    constexpr explicit LibParams() noexcept = default;

    constexpr explicit LibParams(Er::Log::ILog* log, Er::Log::Level level) noexcept
        : log(log)
        , level(level)
    {
    }
};

EREBUSCLT_EXPORT void initialize(const LibParams& params);
EREBUSCLT_EXPORT void finalize();

class LibScope
    : public Er::NonCopyable
{
public:
    ~LibScope()
    {
        finalize();
    }

    LibScope(const LibParams& params)
    {
        initialize(params);
    }
};


struct Params
{
    Log::ILog* log = nullptr;
    std::string endpoint;
    bool ssl;
    std::string rootCertificate;
    std::string certificate;
    std::string key;

    Params() noexcept = default;

    explicit Params(
        Log::ILog* log,
        std::string_view endpoint,
        bool ssl,
        std::string_view rootCertificate,
        std::string_view certificate,
        std::string_view key
    )
        : log(log)
        , endpoint(endpoint)
        , ssl(ssl)
        , rootCertificate(rootCertificate)
        , certificate(certificate)
        , key(key)
    {
    }
};

EREBUSCLT_EXPORT std::shared_ptr<IClient> create(const Params& params);

} // namespace Client {}
    
} // namespace Er {}