#include "processdetailsservice.hxx"

#include <erebus/exception.hxx>
#include <erebus/util/format.hxx>
#include <erebus/util/posixerror.hxx>

#include <signal.h>

namespace Erp
{

namespace ProcessMgr
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
{
}

void ProcessDetailsService::registerService(Er::Server::IServiceContainer* container)
{
    container->registerService(Er::ProcessMgr::ProcessRequests::KillProcess, this);
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
    if (request == Er::ProcessMgr::ProcessRequests::KillProcess)
        return killProcess(args);

    throwGenericError(Er::Util::format("Unsupported request %s", std::string(request).c_str()));
}

ProcessDetailsService::StreamId ProcessDetailsService::beginStream(std::string_view request, const Er::PropertyBag& args, SessionId sessionId)
{
    throwGenericError(Er::Util::format("Unsupported request %s", std::string(request).c_str()));
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
    auto pid = Er::getPropertyValue<Er::ProcessMgr::ProcessesGlobal::Pid>(args);
    if (!pid)
        throwGenericError("Process ID expected");

    auto signame = Er::getPropertyValue<Er::ProcessMgr::ProcessesGlobal::Signal>(args);
    if (!signame)
        throwGenericError("Signal name expected");

    
    auto signo = mapSignalNameToSigno(*signame);
    if (signo == -1)
        throwGenericError("Invalid signal name");

    auto r = ::kill(*pid, signo);

    Er::PropertyBag result;
    Er::addProperty<Er::ProcessMgr::ProcessesGlobal::PosixResult>(result, r);

    if (r < 0)
    {
        auto decoded = Er::Util::posixErrorToString(r);
        
        ErLogWarning(m_log, "kill(%zu, %d) -> %d [%s]", *pid, signo, r, decoded.c_str());
        
        if (!decoded.empty())
            Er::addProperty<Er::ProcessMgr::ProcessesGlobal::ErrorText>(result, std::move(decoded));
    }
    else
    {
        ErLogInfo(m_log, "kill(%zu, %d) -> ok", *pid, signo);
    }

    return result;
}

} // namespace ProcessMgr {}

} // namespace Erp {}