#include "processdetailsservice.hxx"

#include <erebus/exception.hxx>
#include <erebus/system/user.hxx>
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

void ProcessDetailsService::registerService(Er::Server::IServer* container)
{
    container->registerService(Er::ProcessMgr::Requests::KillProcess, shared_from_this());
    container->registerService(Er::ProcessMgr::Requests::ProcessProps, shared_from_this());
    container->registerService(Er::ProcessMgr::Requests::ProcessPropsExt, shared_from_this());
}

void ProcessDetailsService::unregisterService(Er::Server::IServer* container)
{
    container->unregisterService(this);
}

Er::PropertyBag ProcessDetailsService::request(std::string_view request, std::string_view cookie, const Er::PropertyBag& args)
{
    if (request == Er::ProcessMgr::Requests::KillProcess)
        return killProcess(args);
    else if (request == Er::ProcessMgr::Requests::ProcessProps)
        return processProps(args);
    else if (request == Er::ProcessMgr::Requests::ProcessPropsExt)
        return processPropsExt(args);

    ErThrow(Er::format("Unsupported request {}", request));
}

ProcessDetailsService::StreamId ProcessDetailsService::beginStream(std::string_view request, std::string_view cookie, const Er::PropertyBag& args)
{
    ErThrow(Er::format("Unsupported request {}", request));
}

void ProcessDetailsService::endStream(StreamId id)
{
}

Er::PropertyBag ProcessDetailsService::next(StreamId id)
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
        
        Er::Log::warning(m_log, "kill({}, {}) -> {} [{}]", *pid, signo, r, decoded);
        
        if (!decoded.empty())
            Er::addProperty<Er::ProcessMgr::Props::ErrorText>(result, std::move(decoded));
    }
    else
    {
        Er::Log::info(m_log, "kill({}, {}) -> ok", *pid, signo);
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

    Er::addProperty<Er::ProcessMgr::Props::Pid>(result, *pid);

    if (required[Er::ProcessMgr::ProcessPropsExt::PropIndices::Env])
    {
        auto env = m_procFs.readEnv(*pid);
        Er::addProperty<Er::ProcessMgr::ProcessPropsExt::Env>(result, std::move(env));
    }

    return result;
}

Er::PropertyBag ProcessDetailsService::processProps(const Er::PropertyBag& args)
{
    Er::PropertyBag result;

    // defaut is 'include everything'
    auto mask = Er::getPropertyValueOr<Er::ProcessMgr::ProcessProps::RequiredFields>(args, 0xffffffffffffffff);
    auto required = Er::ProcessMgr::ProcessProps::PropMask(mask, Er::ProcessMgr::ProcessProps::PropMask::FromBits);

    auto pid = Er::getPropertyValue<Er::ProcessMgr::Props::Pid>(args);
    if (!pid)
        ErThrow("Process ID expected");

    auto stat = m_procFs.readStat(*pid);
    ErAssert(stat.pid != InvalidPid); // PID is always valid

    Er::addProperty<Er::ProcessMgr::Props::Pid>(result, *pid);

    if (required[Er::ProcessMgr::ProcessProps::PropIndices::PPid])
        Er::addProperty<Er::ProcessMgr::ProcessProps::PPid>(result, stat.ppid);
    
    if (required[Er::ProcessMgr::ProcessProps::PropIndices::PGrp])
        Er::addProperty<Er::ProcessMgr::ProcessProps::PGrp>(result, stat.pgrp);

    if (required[Er::ProcessMgr::ProcessProps::PropIndices::Tpgid])
        Er::addProperty<Er::ProcessMgr::ProcessProps::Tpgid>(result, stat.tpgid);
    
    if (required[Er::ProcessMgr::ProcessProps::PropIndices::Session])
        Er::addProperty<Er::ProcessMgr::ProcessProps::Session>(result, stat.session);
    
    if (required[Er::ProcessMgr::ProcessProps::PropIndices::Ruid])
        Er::addProperty<Er::ProcessMgr::ProcessProps::Ruid>(result, stat.ruid);
    
    if (required[Er::ProcessMgr::ProcessProps::PropIndices::StartTime])
        Er::addProperty<Er::ProcessMgr::ProcessProps::StartTime>(result, stat.startTime);

    if (required[Er::ProcessMgr::ProcessProps::PropIndices::Tty])
        Er::addProperty<Er::ProcessMgr::ProcessProps::Tty>(result, stat.tty_nr);
    
    if (required[Er::ProcessMgr::ProcessProps::PropIndices::State])
        Er::addProperty<Er::ProcessMgr::ProcessProps::State>(result, Er::ProcessMgr::ProcessProps::State::ValueType(stat.state));

    if (required[Er::ProcessMgr::ProcessProps::PropIndices::Comm])
    {
        auto comm = m_procFs.readComm(*pid);
        if (!comm.empty())
        {
            Er::addProperty<Er::ProcessMgr::ProcessProps::Comm>(result, std::move(comm));
        }
    }

    if (required[Er::ProcessMgr::ProcessProps::PropIndices::CmdLine])
    {
        auto cmdLine = m_procFs.readCmdLine(*pid);
        if (!cmdLine.empty())
            Er::addProperty<Er::ProcessMgr::ProcessProps::CmdLine>(result, std::move(cmdLine));
    }

    if (stat.ppid != KThreadDPid)
    {
        if (required[Er::ProcessMgr::ProcessProps::PropIndices::Exe])
        {
            auto exe = m_procFs.readExePath(*pid);
            if (!exe.empty())
            {
                Er::addProperty<Er::ProcessMgr::ProcessProps::Exe>(result, std::move(exe));
            }
        }
    }

    if (required[Er::ProcessMgr::ProcessProps::PropIndices::User])
    {
        auto user = Er::System::User::lookup(stat.ruid);
        if (user)
            Er::addProperty<Er::ProcessMgr::ProcessProps::User>(result, std::move(user->name));
    }

    if (required[Er::ProcessMgr::ProcessProps::PropIndices::ThreadCount])
        Er::addProperty<Er::ProcessMgr::ProcessProps::ThreadCount>(result, stat.num_threads);

    if (required[Er::ProcessMgr::ProcessProps::PropIndices::UTime])
        Er::addProperty<Er::ProcessMgr::ProcessProps::UTime>(result, stat.uTime);

    if (required[Er::ProcessMgr::ProcessProps::PropIndices::STime])
        Er::addProperty<Er::ProcessMgr::ProcessProps::STime>(result, stat.sTime);

    return result;
}

} // namespace Erp::ProcessMgr {}