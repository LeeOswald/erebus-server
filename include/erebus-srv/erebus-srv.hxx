#pragma once

#include <erebus/erebus.hxx>
#include <erebus/log.hxx>
#include <erebus/util/condition.hxx>


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
    
namespace Private
{

namespace Server
{

struct Params
{
    std::string endpoint;
    Er::Log::ILog* log = nullptr;
    Er::Util::Condition* exitCondition = nullptr;
    bool* needRestart = nullptr;
    std::string root;
    std::string certificate;
    std::string key;

    Params() noexcept = default;

    explicit Params(
        std::string_view endpoint,
        Er::Log::ILog* log,
        Er::Util::Condition* exitCondition,
        bool* needRestart,
        std::string_view root,
        std::string_view certificate,
        std::string_view key
    )
        : endpoint(endpoint)
        , log(log)
        , exitCondition(exitCondition)
        , needRestart(needRestart)
        , root(root)
        , certificate(certificate)
        , key(key)
    {
    }
};


struct IServer
{
    virtual void stop() = 0;
    virtual void wait() = 0;

    virtual ~IServer() {}
};


EREBUSSRV_EXPORT void initialize();
EREBUSSRV_EXPORT void finalize();

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


std::shared_ptr<IServer> EREBUSSRV_EXPORT start(const Params* params);


} // namespace Server {}

} // namespace Private {}
    
} // namespace Er {}