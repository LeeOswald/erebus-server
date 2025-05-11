#include <erebus/proctree/protocol.hxx>


namespace Er::ProcessTree
{

void marshalProcessProperties(const ProcessProperties& source, erebus::ProcessProps& dest)
{
    ErAssert(source.valid(ProcessProperties::Pid));

    dest.set_pid(source.pid);

    if (source.valid(ProcessProperties::PPid))
        dest.set_ppid(source.ppid);

    if (source.valid(ProcessProperties::PGrp))
        dest.set_pgrp(source.pgrp);

    if (source.valid(ProcessProperties::Tpgid))
        dest.set_tpgid(source.tpgid);

    if (source.valid(ProcessProperties::Session))
        dest.set_session(source.session);

    if (source.valid(ProcessProperties::Ruid))
        dest.set_ruid(source.ruid);

    if (source.valid(ProcessProperties::Comm))
        dest.set_comm(source.comm);

    if (source.valid(ProcessProperties::CmdLine))
        dest.set_cmdline(source.cmdLine.raw);

    if (source.valid(ProcessProperties::Exe))
        dest.set_exe(source.exe);

    if (source.valid(ProcessProperties::StartTime))
        dest.set_starttime(source.startTime.value());

    if (source.valid(ProcessProperties::State))
        dest.set_state(source.state);

    if (source.valid(ProcessProperties::UserName))
        dest.set_username(source.userName);

    if (source.valid(ProcessProperties::ThreadCount))
        dest.set_threadcount(source.threadCount);

    if (source.valid(ProcessProperties::STime))
        dest.set_stime(source.sTime.value());

    if (source.valid(ProcessProperties::UTime))
        dest.set_utime(source.uTime.value());

    if (source.valid(ProcessProperties::CpuUsage))
        dest.set_cpuusage(source.cpuUsage);

    if (source.valid(ProcessProperties::Tty))
        dest.set_tty(source.tty);

    if (source.valid(ProcessProperties::Env))
        dest.set_env(source.env.raw);
}

ProcessProperties unmarshalProcessProperties(const erebus::ProcessProps& src)
{
    ProcessProperties dest;
    
    ErSet(ProcessProperties, Pid, dest, pid, src.pid());

    if (src.has_ppid())
        ErSet(ProcessProperties, PPid, dest, ppid, src.ppid());

    if (src.has_pgrp())
        ErSet(ProcessProperties, PGrp, dest, pgrp, src.pgrp());

    if (src.has_tpgid())
        ErSet(ProcessProperties, Tpgid, dest, tpgid, src.tpgid());

    if (src.has_session())
        ErSet(ProcessProperties, Session, dest, session, src.session());

    if (src.has_ruid())
        ErSet(ProcessProperties, Ruid, dest, ruid, src.ruid());

    if (src.has_comm())
        ErSet(ProcessProperties, Comm, dest, comm, src.comm());

    if (src.has_cmdline())
        ErSet(ProcessProperties, CmdLine, dest, cmdLine, src.cmdline());

    if (src.has_exe())
        ErSet(ProcessProperties, Exe, dest, exe, src.exe());

    if (src.has_starttime())
        ErSet(ProcessProperties, StartTime, dest, startTime, src.starttime());

    if (src.has_state())
        ErSet(ProcessProperties, State, dest, state, src.state());

    if (src.has_username())
        ErSet(ProcessProperties, UserName, dest, userName, src.username());

    if (src.has_threadcount())
        ErSet(ProcessProperties, ThreadCount, dest, threadCount, src.threadcount());

    if (src.has_stime())
        ErSet(ProcessProperties, STime, dest, sTime, src.stime());

    if (src.has_utime())
        ErSet(ProcessProperties, UTime, dest, uTime, src.utime());

    if (src.has_cpuusage())
        ErSet(ProcessProperties, CpuUsage, dest, cpuUsage, src.cpuusage());

    if (src.has_tty())
        ErSet(ProcessProperties, Tty, dest, tty, src.tty());

    if (src.has_env())
        ErSet(ProcessProperties, Env, dest, env, src.env());

    return dest;
}

void marshalProcessPropertyMsk(erebus::ProcessPropsRequest& dest, const ProcessProperties::Mask& required)
{
    for (std::uint32_t i = 0; i < required.Size; ++i)
        dest.add_fields(i);
}

ProcessProperties::Mask unmarshalProcessPropertyMask(const erebus::ProcessPropsRequest& req)
{
    ProcessProperties::Mask mask;
    
    auto count = req.fields_size();
    if (count == 0)
    {
        // if no fields explicitly specified, assume 'everything'
        mask.set();
    }
    else
    {
        for (decltype(count) i = 0; i < count; ++i)
        {
            auto f = req.fields()[i];
            mask.set(f);
        }
    }

    return mask;
}

} // namespace Er::ProcessTree {}