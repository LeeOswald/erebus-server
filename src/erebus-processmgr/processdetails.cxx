#include "processdetails.hxx"

#include <erebus/exception.hxx>
#include <erebus/util/format.hxx>
#include <erebus/util/posixerror.hxx>

#include <signal.h>

namespace Er
{

namespace Private
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
    { "SIGTERM", SIGTERM },
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

ProcessDetails::~ProcessDetails()
{
    LogDebug(m_log, LogInstance("ProcessDetails"), "~ProcessDetails()");
}

ProcessDetails::ProcessDetails(Er::Log::ILog* log)
    : m_log(log)
{
    LogDebug(m_log, LogInstance("ProcessDetails"), "ProcessDetails()");
}

void ProcessDetails::registerService(Er::Server::IServiceContainer* container)
{
    container->registerService(Er::ProcessRequests::KillProcess, this);
}

void ProcessDetails::unregisterService(Er::Server::IServiceContainer* container)
{
    container->unregisterService(this);
}

ProcessDetails::SessionId ProcessDetails::allocateSession()
{
    return SessionId(1);
}

void ProcessDetails::deleteSession(SessionId id)
{
}

Er::PropertyBag ProcessDetails::request(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId)
{
    if (request == Er::ProcessRequests::KillProcess)
        return killProcess(args);

    throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported request %s", std::string(request).c_str()));
}

ProcessDetails::StreamId ProcessDetails::beginStream(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId)
{
    throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported request %s", std::string(request).c_str()));
}

void ProcessDetails::endStream(StreamId id, std::optional<SessionId> sessionId)
{
}

Er::PropertyBag ProcessDetails::next(StreamId id, std::optional<SessionId> sessionId)
{
    return Er::PropertyBag();
}

Er::PropertyBag ProcessDetails::killProcess(const Er::PropertyBag& args)
{
    auto pid = Er::getProperty<Er::ProcessesGlobal::Pid::ValueType>(args, Er::ProcessesGlobal::Pid::Id::value);
    if (!pid)
        throw Er::Exception(ER_HERE(), "Process ID expected");

    auto signame = Er::getProperty<Er::ProcessesGlobal::Signal::ValueType>(args, Er::ProcessesGlobal::Signal::Id::value);
    if (!signame)
        throw Er::Exception(ER_HERE(), "Signal name expected");

    
    auto signo = mapSignalNameToSigno(*signame);
    if (signo == -1)
        throw Er::Exception(ER_HERE(), "Invalid signal name");

    auto r = ::kill(*pid, signo);

    Er::PropertyBag result;
    result.insert({ Er::ProcessesGlobal::PosixResult::Id::value, Er::Property(Er::ProcessesGlobal::PosixResult::Id::value, Er::ProcessesGlobal::PosixResult::ValueType(r)) });

    if (r < 0)
    {
        auto decoded = Er::Util::posixErrorToString(r);
        
        LogWarning(m_log, LogInstance("ProcessDetails"), "kill(%zu, %d) -> %d [%s]", *pid, signo, r, decoded.c_str());
        
        if (!decoded.empty())
            result.insert({ Er::ProcessesGlobal::ErrorText::Id::value, Er::Property(Er::ProcessesGlobal::ErrorText::Id::value, std::move(decoded)) });
    }
    else
    {
        LogInfo(m_log, LogInstance("ProcessDetails"), "kill(%zu, %d) -> ok", *pid, signo);
    }

    return result;
}

} // namespace Private {}

} // namespace Er {}