#pragma once

#include <erebus/proctree/proctree.hxx>
#include <erebus/rtl/reflectable.hxx>


namespace Er::ProcessTree
{

ER_REFLECTABLE_TRAITS_BEGIN(ProcessProperties, ProcessPropertiesTraits)
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
    Tty
ER_REFLECTABLE_TRAITS_END()


struct ProcessProperties
    : public Reflectable<ProcessPropertiesTraits>
{
    std::uint64_t pid;
    std::uint64_t ppid;
    std::uint64_t pgrp;
    std::uint64_t tpgid;
    std::uint64_t session;
    std::uint64_t ruid;
    std::string comm;
    std::string cmdLine;
    std::string exe;
    std::uint64_t startTime;
    std::uint32_t state;
    std::string userName;
    std::int64_t threadCount;
    double sTime;
    double uTime;
    double cpuUsage;
    std::int32_t tty;

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
        ER_REFLECTABLE_FIELD(ProcessProperties, StartTime, Semantics::Default, startTime),
        ER_REFLECTABLE_FIELD(ProcessProperties, State, Semantics::Default, state),
        ER_REFLECTABLE_FIELD(ProcessProperties, UserName, Semantics::Default, userName),
        ER_REFLECTABLE_FIELD(ProcessProperties, ThreadCount, Semantics::Default, threadCount),
        ER_REFLECTABLE_FIELD(ProcessProperties, STime, Semantics::Default, sTime),
        ER_REFLECTABLE_FIELD(ProcessProperties, UTime, Semantics::Default, uTime),
        ER_REFLECTABLE_FIELD(ProcessProperties, CpuUsage, Semantics::Default, cpuUsage),
        ER_REFLECTABLE_FIELD(ProcessProperties, Tty, Semantics::Default, tty)
    ER_REFLECTABLE_FILEDS_END()
};



} // Er::ProcessTree {}