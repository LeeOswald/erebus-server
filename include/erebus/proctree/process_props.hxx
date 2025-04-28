#pragma once

#include <erebus/proctree/proctree.hxx>
#include <erebus/rtl/multi_string.hxx>
#include <erebus/rtl/reflectable.hxx>
#include <erebus/rtl/time.hxx>

namespace Er::ProcessTree
{

struct ProcessProperties
    : public Reflectable<ProcessProperties, 18>
{
    enum Field : FieldId
    {
        Pid,
        PPid,
        PGrp,
        Tpgid,
        Session,
        Ruid,
        Comm,
        CmdLine,
        Exe,
        StartTime,
        State,
        UserName,
        ThreadCount,
        STime,
        UTime,
        CpuUsage,
        Tty,
        Env,
        _FieldCount
    };

    std::uint64_t pid;
    std::uint64_t ppid;
    std::uint64_t pgrp;
    std::uint64_t tpgid;
    std::uint64_t session;
    std::uint64_t ruid;
    std::string comm;
    MultiStringZ cmdLine;
    std::string exe;
    Time startTime;
    std::uint32_t state;
    std::string userName;
    std::uint32_t threadCount;
    Time sTime;
    Time uTime;
    double cpuUsage;
    std::int32_t tty;
    MultiStringZ env;

    ER_REFLECTABLE_FILEDS_BEGIN(ProcessProperties)
        ER_REFLECTABLE_FIELD(ProcessProperties, Pid, Semantics::Default, pid),
        ER_REFLECTABLE_FIELD(ProcessProperties, PPid, Semantics::Default, ppid),
        ER_REFLECTABLE_FIELD(ProcessProperties, PGrp, Semantics::Default, pgrp),
        ER_REFLECTABLE_FIELD(ProcessProperties, Tpgid, Semantics::Default, tpgid),
        ER_REFLECTABLE_FIELD(ProcessProperties, Session, Semantics::Default, session),
        ER_REFLECTABLE_FIELD(ProcessProperties, Ruid, Semantics::Default, ruid),
        ER_REFLECTABLE_FIELD(ProcessProperties, Comm, Semantics::Default, comm),
        ER_REFLECTABLE_FIELD(ProcessProperties, CmdLine, Semantics::Default, cmdLine),
        ER_REFLECTABLE_FIELD(ProcessProperties, Exe, Semantics::Default, exe),
        ER_REFLECTABLE_FIELD(ProcessProperties, StartTime, Semantics::AbsoluteTime, startTime),
        ER_REFLECTABLE_FIELD(ProcessProperties, State, Semantics::Default, state),
        ER_REFLECTABLE_FIELD(ProcessProperties, UserName, Semantics::Default, userName),
        ER_REFLECTABLE_FIELD(ProcessProperties, ThreadCount, Semantics::Default, threadCount),
        ER_REFLECTABLE_FIELD(ProcessProperties, STime, Semantics::Duration, sTime),
        ER_REFLECTABLE_FIELD(ProcessProperties, UTime, Semantics::Duration, uTime),
        ER_REFLECTABLE_FIELD(ProcessProperties, CpuUsage, Semantics::Percent, cpuUsage),
        ER_REFLECTABLE_FIELD(ProcessProperties, Tty, Semantics::Default, tty),
        ER_REFLECTABLE_FIELD(ProcessProperties, Env, Semantics::Default, env)
    ER_REFLECTABLE_FILEDS_END()
};



} // Er::ProcessTree {}