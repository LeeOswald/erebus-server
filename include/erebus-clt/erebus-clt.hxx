#pragma once

#include <erebus/exception.hxx>
#include <erebus/log.hxx>
#include <erebus/propertybag.hxx>

#include <functional>

#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef EREBUSCLT_EXPORTS
        #define EREBUSCLT_EXPORT __declspec(dllexport)
    #else
        #define EREBUSCLT_EXPORT __declspec(dllimport)
    #endif
#else
    #define EREBUSCLT_EXPORT __attribute__((visibility("default")))
#endif


namespace Er::Client
{


struct IClient
{
    using Ptr = std::unique_ptr<IClient>;
    using StreamReader = std::function<bool(Er::PropertyBag&&)>;

    virtual Er::PropertyBag request(std::string_view request, const Er::PropertyBag& args) = 0;
    virtual void requestStream(std::string_view request, const Er::PropertyBag& args, StreamReader reader) = 0;

    virtual ~IClient() = default;
};


EREBUSCLT_EXPORT void initialize(Er::Log::ILog* log);
EREBUSCLT_EXPORT void finalize();


struct ChannelParams
{
    std::string endpoint;
    bool ssl;
    std::string rootCa;
    std::string certificate;
    std::string privateKey;
    bool keepAlive = true;

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
        , rootCa(rootCa)
        , certificate(certificate)
        , privateKey(privateKey)
    {
    }
};

using ChannelPtr = std::shared_ptr<void>;


EREBUSCLT_EXPORT ChannelPtr createChannel(const ChannelParams& params);

EREBUSCLT_EXPORT IClient::Ptr createClient(ChannelPtr channel, Log::ILog* log);


} // namespace Er::Client {}