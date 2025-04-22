#pragma once

#include <erebus/proctree/proctree.hxx>
#include <erebus/rtl/reflectable.hxx>


namespace Er::ProcessTree
{

struct ProcessProperties;

struct ProcessPropertiesTraits
{
    using SelfType = ProcessProperties;

    struct FieldIds
    {
        enum : unsigned
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
            User,
            ThreadCount,
            STime,
            UTime,
            CpuUsage,
            Tty,

            _FieldCount
        };
    };
};

struct ProcessProperties
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
};



} // Er::ProcessTree {}