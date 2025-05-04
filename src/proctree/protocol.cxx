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

} // namespace Er::ProcessTree {}