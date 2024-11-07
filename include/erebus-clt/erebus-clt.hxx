#pragma once

#include <erebus/exception.hxx>
#include <erebus/log.hxx>
#include <erebus/propertybag.hxx>

#include <chrono>
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
    using CallId = uintptr_t;

    struct IReceiver
    {
        virtual void receive(CallId callId, Er::PropertyBag&& result) = 0;
        virtual void receive(CallId callId, Er::Exception&& exception) = 0;
        virtual void receive(CallId callId, Er::Result result, std::string&& message) = 0;

    protected:
        virtual ~IReceiver() = default;
    };

    struct IStreamReceiver
    {
        enum class Result
        {
            Continue,
            Cancel
        };

        virtual Result receive(CallId callId, Er::PropertyBag&& result) = 0;
        virtual Result receive(CallId callId, Er::Exception&& exception) = 0;
        virtual void finish(CallId callId, Er::Result result, std::string&& message) = 0;
        virtual void finish(CallId callId) = 0;

    protected:
        virtual ~IStreamReceiver() = default;
    };

    using Ptr = std::unique_ptr<IClient>;

    virtual void request(CallId callId, std::string_view request, const Er::PropertyBag& args, IReceiver* receiver, std::optional<std::chrono::milliseconds> timeout = std::nullopt) = 0;
    virtual void requestStream(CallId callId, std::string_view request, const Er::PropertyBag& args, IStreamReceiver* receiver) = 0;

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