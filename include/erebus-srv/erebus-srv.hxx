#pragma once

#include <erebus/log.hxx>
#include <erebus/propertybag.hxx>



#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef EREBUSSRV_EXPORTS
        #define EREBUSSRV_EXPORT __declspec(dllexport)
    #else
        #define EREBUSSRV_EXPORT __declspec(dllimport)
    #endif
#else
    #define EREBUSSRV_EXPORT __attribute__((visibility("default")))
#endif


namespace Er::Server
{

struct IServer;

struct IService
{
    using StreamId = uintptr_t;
    using Ptr = std::shared_ptr<IService>;

    virtual ~IService() = default;

    virtual void registerService(IServer* container) = 0;
    virtual void unregisterService(IServer* container) = 0;

    virtual Er::PropertyBag request(std::string_view request, std::string_view cookie, const Er::PropertyBag& args) = 0; 
    [[nodiscard]] virtual StreamId beginStream(std::string_view request, std::string_view cookie, const Er::PropertyBag& args) = 0;
    virtual void endStream(StreamId id) = 0;
    virtual Er::PropertyBag next(StreamId id) = 0;
};


struct Params
{
    struct Endpoint
    {
        std::string endpoint;
        bool ssl = false;
        std::string rootCA;
        std::string certificate;
        std::string privateKey;
        
        Endpoint(const std::string& endpoint)
            : endpoint(endpoint)
        {
        }

        Endpoint(const std::string& endpoint, const std::string& rootCA, const std::string& certificate, const std::string& privateKey)
            : endpoint(endpoint)
            , ssl(true)
            , rootCA(rootCA)
            , certificate(certificate)
            , privateKey(privateKey)
        {
        }
    };

    Er::Log::ILog* log = nullptr;
    std::vector<Endpoint> endpoints;
    bool keepAlive = true;

    explicit Params(Er::Log::ILog* log) noexcept
        : log(log)
    {
    }
};


EREBUSSRV_EXPORT void initialize(Er::Log::ILog* log);
EREBUSSRV_EXPORT void finalize();


struct IServer
{
    using Ptr = std::unique_ptr<IServer>;

    virtual ~IServer() = default;
    virtual void registerService(std::string_view request, IService::Ptr service) = 0;
    virtual void unregisterService(IService* service) = 0;
};

[[nodiscard]] IServer::Ptr EREBUSSRV_EXPORT create(const Params& params);


} // namespace Er::Server {}