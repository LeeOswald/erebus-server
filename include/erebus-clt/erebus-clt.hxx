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
    using Ptr = std::unique_ptr<IClient>;
    using SessionId = uint32_t;

    virtual SessionId beginSession(std::string_view request) = 0;
    virtual void endSession(std::string_view request, SessionId id) = 0;
    virtual Er::PropertyBag request(std::string_view request, const Er::PropertyBag& args, SessionId) = 0;
    virtual std::vector<Er::PropertyBag> requestStream(std::string_view request, const Er::PropertyBag& args, SessionId sessionId) = 0;

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


struct ChannelParams
{
    std::string endpoint;
    bool ssl;
    std::string rootCertificate;
    std::string certificate;
    std::string key;

    ChannelParams() noexcept = default;

    explicit ChannelParams(
        std::string_view endpoint,
        bool ssl,
        std::string_view rootCertificate,
        std::string_view certificate,
        std::string_view key
    )
        : endpoint(endpoint)
        , ssl(ssl)
        , rootCertificate(rootCertificate)
        , certificate(certificate)
        , key(key)
    {
    }
};

using ChannelPtr = std::shared_ptr<void>;


EREBUSCLT_EXPORT ChannelPtr createChannel(const ChannelParams& params);

EREBUSCLT_EXPORT IClient::Ptr createClient(ChannelPtr channel, Log::ILog* log);


} // namespace Client {}
    
} // namespace Er {}