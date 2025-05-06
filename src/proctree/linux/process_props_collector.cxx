#include "process_props_collector.hxx"

#include <erebus/rtl/system/user.hxx>


namespace Er::ProcessTree::Linux
{

namespace
{

std::string lookupUserName(std::uint64_t uid) 
{
    try
    {
        auto info = System::User::lookup(uid);
        if (info)
            return info->name;
    }
    catch (...)
    {
    }

    return {};
}

} // namespace {}


std::expected<ProcessProperties, int> collectProcessProps(Linux::ProcFs& procFs, Pid pid, const ProcessProperties::Mask& mask, Log::ILogger* log)
{
    ProcessProperties out;

    auto stat_ = procFs.readStat(pid);
    if (!stat_.has_value())
    {
        ErLogWarning2(log, "Could not read /proc/{}/stat: {}", pid, stat_.error());
        return std::unexpected(stat_.error());
    }

    auto& stat = stat_.value();

    ErSet(ProcessProperties, Pid, out, pid, stat.pid);
    
    if (mask[ProcessProperties::PPid])
        ErSet(ProcessProperties, PPid, out, ppid, stat.ppid);
    
    if (mask[ProcessProperties::PGrp])
        ErSet(ProcessProperties, PGrp, out, pgrp, stat.pgrp);

    if (mask[ProcessProperties::Tpgid])
        ErSet(ProcessProperties, Tpgid, out, tpgid, stat.tpgid);

    if (mask[ProcessProperties::Session])
        ErSet(ProcessProperties, Session, out, session, stat.session);

    if (mask[ProcessProperties::Ruid])
        ErSet(ProcessProperties, Ruid, out, ruid, stat.ruid);

    if (mask[ProcessProperties::Comm])
    {
        auto comm_ = procFs.readComm(pid);
        if (!comm_.has_value())
        {
            ErLogWarning2(log, "Could not read /proc/{}/comm: {}", pid, comm_.error());
            ErSet(ProcessProperties, Comm, out, comm, std::move(stat.comm));
        }
        else
        {
            ErSet(ProcessProperties, Comm, out, comm, std::move(comm_.value()));
        }
    }

    if (mask[ProcessProperties::CmdLine])
    {
        auto cmd_ = procFs.readCmdLine(pid);
        if (!cmd_.has_value())
        {
            ErLogWarning2(log, "Could not read /proc/{}/cmdline: {}", pid, cmd_.error());
        }
        else
        {
            ErSet(ProcessProperties, CmdLine, out, cmdLine, std::move(cmd_.value()));
        }
    }

    if (mask[ProcessProperties::Exe])
    {
        auto exe_ = procFs.readExePath(pid);
        if (!exe_.has_value())
        {
            ErLogWarning2(log, "Could not read /proc/{}/exe: {}", pid, exe_.error());
        }
        else
        {
            ErSet(ProcessProperties, Exe, out, exe, std::move(exe_.value()));
        }
    }

    if (mask[ProcessProperties::StartTime])
        ErSet(ProcessProperties, StartTime, out, startTime, stat.startTime);    

    if (mask[ProcessProperties::State])
        ErSet(ProcessProperties, State, out, state, stat.state); 

    if (mask[ProcessProperties::UserName])
    {
        auto user = lookupUserName(stat.ruid);
        if (!user.empty())
            ErSet(ProcessProperties, UserName, out, userName, std::move(user));
    }

    if (mask[ProcessProperties::ThreadCount])
        ErSet(ProcessProperties, ThreadCount, out, threadCount, stat.num_threads);

    if (mask[ProcessProperties::STime])
        ErSet(ProcessProperties, STime, out, sTime, Linux::ProcFs::timeFromTicks(stat.stime));

    if (mask[ProcessProperties::UTime])
        ErSet(ProcessProperties, UTime, out, uTime, Linux::ProcFs::timeFromTicks(stat.utime));

    if (mask[ProcessProperties::Tty])
        ErSet(ProcessProperties, Tty, out, tty, stat.tty_nr);
        
    if (mask[ProcessProperties::Env])
    {
        auto env_ = procFs.readEnv(pid);
        if (!env_.has_value())
        {
            ErLogWarning2(log, "Could not read /proc/{}/env: {}", pid, env_.error());
        }
        else
        {
            ErSet(ProcessProperties, Env, out, env, std::move(env_.value()));
        }
    }

    return { std::move(out) };
}

} // namespace Er::ProcessTree::Linux {}