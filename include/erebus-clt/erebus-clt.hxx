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

struct ServerInfo
{
    std::string version;
    std::string platform;

    ServerInfo() noexcept = default;
    
    explicit ServerInfo(std::string_view version, std::string_view platform)
        : version(version)
        , platform(platform)
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

struct IServerCtl;

struct IClient
{
    using SessionId = uint32_t;

    virtual IServerCtl* getCtl() = 0;
    virtual SessionId beginSession(std::string_view request) = 0;
    virtual void endSession(std::string_view request, SessionId id) = 0;
    virtual Er::PropertyBag request(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId = std::nullopt) = 0;
    virtual std::vector<Er::PropertyBag> requestStream(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId = std::nullopt) = 0;

    virtual ~IClient() {}
};

struct IServerCtl
{
    virtual ServerInfo serverInfo() = 0;
    virtual void addUser(std::string_view name, std::string_view password) = 0;
    virtual void removeUser(std::string_view name) = 0;
    virtual std::vector<UserInfo> listUsers() = 0;
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

EREBUSCLT_EXPORT std::shared_ptr<IClient> create(const Params& params);

} // namespace Client {}
    
} // namespace Er {}