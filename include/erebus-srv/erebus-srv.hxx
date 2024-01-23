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

struct ServerParams
{
    std::string address;
    Er::Log::ILog* log = nullptr;
    Er::Util::Condition* exitCondition = nullptr;
    bool* needRestart = nullptr;

    ServerParams() noexcept = default;

    explicit ServerParams(
        std::string_view address,
        Er::Log::ILog* log,
        Er::Util::Condition* exitCondition,
        bool* needRestart
    )
        : address(address)
        , log(log)
        , exitCondition(exitCondition)
        , needRestart(needRestart)
    {
    }

    ServerParams(const ServerParams&) = default;
    ServerParams& operator=(const ServerParams&) = default;
};


struct IErebusSrv
{
    virtual void stop() = 0;
    virtual void wait() = 0;

    virtual ~IErebusSrv() {}
};


std::shared_ptr<IErebusSrv> EREBUSSRV_EXPORT startServer(const ServerParams* params);


} // namespace Private {}
    
} // namespace Er {}