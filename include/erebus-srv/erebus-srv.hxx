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


struct IService
{
    using StreamId = uint32_t;
    
    virtual Er::PropertyBag request(std::string_view request, const Er::PropertyBag& args) = 0; 
    virtual StreamId beginStream(std::string_view request, const Er::PropertyBag& args) = 0;
    virtual void endStream(StreamId id) = 0;
    virtual Er::PropertyBag next(StreamId id) = 0;

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
    unsigned workerThreads = 2;
    std::vector<Endpoint> endpoints;
    bool keepAlive = true;

    explicit Params(Er::Log::ILog* log)
        : log(log)
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

IServer::Ptr EREBUSSRV_EXPORT create(const Params& params);


} // namespace Er::Server {}