#pragma once

#include <erebus/erebus.hxx>
#include <erebus/log.hxx>
#include <erebus/propertybag.hxx>
#include <erebus/condition.hxx>



#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef EREBUSSRV_EXPORTS
        #define EREBUSSRV_EXPORT __declspec(dllexport)
    #else
        #define EREBUSSRV_EXPORT __declspec(dllimport)
    #endif
#else
    #define EREBUSSRV_EXPORT __attribute__((visibility("default")))
#endif


namespace Er
{

namespace Server
{


struct IService
{
    using SessionId = uint32_t;
    using StreamId = uint32_t;
    
    virtual SessionId allocateSession() = 0;
    virtual void deleteSession(SessionId id) = 0;
    virtual Er::PropertyBag request(std::string_view request, const Er::PropertyBag& args, SessionId sessionId) = 0; 
    virtual StreamId beginStream(std::string_view request, const Er::PropertyBag& args, SessionId sessionId) = 0;
    virtual void endStream(StreamId id, SessionId sessionId) = 0;
    virtual Er::PropertyBag next(StreamId id, SessionId sessionId) = 0;

protected:
    virtual ~IService() {}
};


struct IServiceContainer
{
    virtual void registerService(std::string_view request, IService* service) = 0;
    virtual void unregisterService(IService* service) = 0;

protected:
    virtual ~IServiceContainer() {}
};


struct Params
{
    std::string endpoint;
    Er::Log::ILog* log = nullptr;
    bool ssl = false;
    std::string rootCertificate;
    std::string certificate;
    std::string key;

    Params() noexcept = default;

    explicit Params(
        std::string_view endpoint,
        Er::Log::ILog* log,
        bool ssl,
        std::string_view rootCertificate,
        std::string_view certificate,
        std::string_view key
    )
        : endpoint(endpoint)
        , log(log)
        , ssl(ssl)
        , rootCertificate(rootCertificate)
        , certificate(certificate)
        , key(key)
    {
    }
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

EREBUSSRV_EXPORT void initialize(const LibParams& params);
EREBUSSRV_EXPORT void finalize();

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


struct IServer
{
    using Ptr = std::unique_ptr<IServer>;

    virtual ~IServer() {}
    virtual IServiceContainer* serviceContainer() = 0;
};

IServer::Ptr EREBUSSRV_EXPORT create(const Params* params);


} // namespace Server {}
    
} // namespace Er {}