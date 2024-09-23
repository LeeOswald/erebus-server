#include "processdetailsservice.hxx"

#include <erebus/exception.hxx>
#include <erebus/util/format.hxx>
#include <erebus/util/posixerror.hxx>

#include <signal.h>

namespace Erp::ProcessMgr
{

namespace
{

struct SignalName
{
    std::string_view name;
    int signo;

    constexpr SignalName(std::string_view name, int signo) noexcept
        : name(name)
        , signo(signo)
    {}
};

static const SignalName g_SignalNames[] = 
{
    { "SIGKILL", SIGKILL },
    { "SIGINT", SIGINT },
    { "SIGTERM", SIGTERM },
    { "SIGQUIT", SIGQUIT },
    { "SIGABRT", SIGABRT },
    { "SIGCONT", SIGCONT },
    { "SIGSTOP", SIGSTOP },
    { "SIGTSTP", SIGTSTP },
    { "SIGHUP", SIGHUP },
    { "SIGUSR1", SIGUSR1 },
    { "SIGUSR2", SIGUSR2 },
    { "SIGSEGV", SIGSEGV },
};

inline int mapSignalNameToSigno(std::string_view name) noexcept
{
    for (auto& m: g_SignalNames)
    {
        if (m.name == name)
            return m.signo;
    }

    return -1;
}

} // namespace {}

ProcessDetailsService::~ProcessDetailsService()
{
}

ProcessDetailsService::ProcessDetailsService(Er::Log::ILog* log)
    : m_log(log)
    , m_procFs(log)
{
}

void ProcessDetailsService::registerService(Er::Server::IServiceContainer* container)
{
    container->registerService(Er::ProcessMgr::Requests::KillProcess, this);
    container->registerService(Er::ProcessMgr::Requests::ProcessPropsExt, this);
}

void ProcessDetailsService::unregisterService(Er::Server::IServiceContainer* container)
{
    container->unregisterService(this);
}

ProcessDetailsService::SessionId ProcessDetailsService::allocateSession()
{
    return SessionId(1);
}

void ProcessDetailsService::deleteSession(SessionId id)
{
}

Er::PropertyBag ProcessDetailsService::request(std::string_view request, const Er::PropertyBag& args, SessionId sessionId)
{
    if (request == Er::ProcessMgr::Requests::KillProcess)
        return killProcess(args);
    else if (request == Er::ProcessMgr::Requests::ProcessPropsExt)
        return processPropsExt(args);

    ErThrow(Er::Util::format("Unsupported request %s", std::string(request).c_str()));
}

ProcessDetailsService::StreamId ProcessDetailsService::beginStream(std::string_view request, const Er::PropertyBag& args, SessionId sessionId)
{
    ErThrow(Er::Util::format("Unsupported request %s", std::string(request).c_str()));
}

void ProcessDetailsService::endStream(StreamId id, SessionId sessionId)
{
}

Er::PropertyBag ProcessDetailsService::next(StreamId id, SessionId sessionId)
{
    return Er::PropertyBag();
}

Er::PropertyBag ProcessDetailsService::killProcess(const Er::PropertyBag& args)
{
    auto pid = Er::getPropertyValue<Er::ProcessMgr::Props::Pid>(args);
    if (!pid)
        ErThrow("Process ID expected");

    auto signame = Er::getPropertyValue<Er::ProcessMgr::Props::SignalName>(args);
    if (!signame)
        ErThrow("Signal name expected");

    
    auto signo = mapSignalNameToSigno(*signame);
    if (signo == -1)
        ErThrow("Invalid signal name");

    auto r = ::kill(*pid, signo);

    Er::PropertyBag result;
    Er::addProperty<Er::ProcessMgr::Props::PosixResult>(result, r);

    if (r < 0)
    {
        auto decoded = Er::Util::posixErrorToString(r);
        
        ErLogWarning(m_log, "kill(%zu, %d) -> %d [%s]", *pid, signo, r, decoded.c_str());
        
        if (!decoded.empty())
            Er::addProperty<Er::ProcessMgr::Props::ErrorText>(result, std::move(decoded));
    }
    else
    {
        ErLogInfo(m_log, "kill(%zu, %d) -> ok", *pid, signo);
    }

    return result;
}

Er::PropertyBag ProcessDetailsService::processPropsExt(const Er::PropertyBag& args)
{
    Er::PropertyBag result;

    // defaut is 'include everything'
    auto mask = Er::getPropertyValueOr<Er::ProcessMgr::ProcessPropsExt::RequiredFields>(args, 0xffffffffffffffff);
    auto required = Er::ProcessMgr::ProcessPropsExt::PropMask(mask, Er::ProcessMgr::ProcessPropsExt::PropMask::FromBits);

    auto pid = Er::getPropertyValue<Er::ProcessMgr::Props::Pid>(args);
    if (!pid)
        ErThrow("Process ID expected");

    if (required[Er::ProcessMgr::ProcessPropsExt::PropIndices::Env])
    {
        auto env = m_procFs.readEnv(*pid);
        Er::addProperty<Er::ProcessMgr::ProcessPropsExt::Env>(result, std::move(env));
    }

    return result;
}


} // namespace Erp::ProcessMgr {}