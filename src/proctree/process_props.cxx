#include <erebus/proctree/process_props.hxx>


namespace Er::ProcessTree
{

const ReflectableProcessProperties::FieldInfo ReflectableProcessProperties::Fields[FieldCount] =
{
    {
        ProcessProperties::FieldIndices::Pid,
        "Pid",
        [](const ProcessProperties& l, const ProcessProperties& r) noexcept { return l.pid == r.pid; },
        [](HashValueType& seed, const ProcessProperties& o) noexcept { boost::hash_combine(seed, o.pid); }
    },
    {
        ProcessProperties::FieldIndices::PPid,
        "PPid",
        [](const ProcessProperties& l, const ProcessProperties& r) noexcept { return l.ppid == r.ppid; },
        [](HashValueType& seed, const ProcessProperties& o) noexcept { boost::hash_combine(seed, o.ppid); }
    },
    {
        ProcessProperties::FieldIndices::PGrp,
        "PGrp",
        [](const ProcessProperties& l, const ProcessProperties& r) noexcept { return l.pgrp == r.pgrp; },
        [](HashValueType& seed, const ProcessProperties& o) noexcept { boost::hash_combine(seed, o.pgrp); }
    },
    {
        ProcessProperties::FieldIndices::Tpgid,
        "Tpgid",
        [](const ProcessProperties& l, const ProcessProperties& r) noexcept { return l.tpgid == r.tpgid; },
        [](HashValueType& seed, const ProcessProperties& o) noexcept { boost::hash_combine(seed, o.tpgid); }
    },
    {
        ProcessProperties::FieldIndices::Session,
        "Session",
        [](const ProcessProperties& l, const ProcessProperties& r) noexcept { return l.session == r.session; },
        [](HashValueType& seed, const ProcessProperties& o) noexcept { boost::hash_combine(seed, o.session); }
    },
    {
        ProcessProperties::FieldIndices::Ruid,
        "Ruid",
        [](const ProcessProperties& l, const ProcessProperties& r) noexcept { return l.ruid == r.ruid; },
        [](HashValueType& seed, const ProcessProperties& o) noexcept { boost::hash_combine(seed, o.ruid); }
    },
    {
        ProcessProperties::FieldIndices::Comm,
        "Comm",
        [](const ProcessProperties& l, const ProcessProperties& r) noexcept { return l.comm == r.comm; },
        [](HashValueType& seed, const ProcessProperties& o) noexcept { boost::hash_combine(seed, o.comm); }
    },
    {
        ProcessProperties::FieldIndices::CmdLine,
        "CmdLine",
        [](const ProcessProperties& l, const ProcessProperties& r) noexcept { return l.cmdLine == r.cmdLine; },
        [](HashValueType& seed, const ProcessProperties& o) noexcept { boost::hash_combine(seed, o.cmdLine); }
    },
    {
        ProcessProperties::FieldIndices::Exe,
        "Exe",
        [](const ProcessProperties& l, const ProcessProperties& r) noexcept { return l.exe == r.exe; },
        [](HashValueType& seed, const ProcessProperties& o) noexcept { boost::hash_combine(seed, o.exe); }
    },
    {
        ProcessProperties::FieldIndices::StartTime,
        "StartTime",
        [](const ProcessProperties& l, const ProcessProperties& r) noexcept { return l.startTime == r.startTime; },
        [](HashValueType& seed, const ProcessProperties& o) noexcept { boost::hash_combine(seed, o.startTime); }
    },
    {
        ProcessProperties::FieldIndices::State,
        "State",
        [](const ProcessProperties& l, const ProcessProperties& r) noexcept { return l.state == r.state; },
        [](HashValueType& seed, const ProcessProperties& o) noexcept { boost::hash_combine(seed, o.state); }
    },
    {
        ProcessProperties::FieldIndices::User,
        "User",
        [](const ProcessProperties& l, const ProcessProperties& r) noexcept { return l.userName == r.userName; },
        [](HashValueType& seed, const ProcessProperties& o) noexcept { boost::hash_combine(seed, o.userName); }
    },
    {
        ProcessProperties::FieldIndices::ThreadCount,
        "ThreadCount",
        [](const ProcessProperties& l, const ProcessProperties& r) noexcept { return l.threadCount == r.threadCount; },
        [](HashValueType& seed, const ProcessProperties& o) noexcept { boost::hash_combine(seed, o.threadCount); }
    },
    {
        ProcessProperties::FieldIndices::STime,
        "SystemTime",
        [](const ProcessProperties& l, const ProcessProperties& r) noexcept { return std::abs(l.sTime - r.sTime) < 0.01; },
        [](HashValueType& seed, const ProcessProperties& o) noexcept { boost::hash_combine(seed, o.sTime); }
    },
    {
        ProcessProperties::FieldIndices::UTime,
        "UserTime",
        [](const ProcessProperties& l, const ProcessProperties& r) noexcept { return std::abs(l.uTime - r.uTime) < 0.01; },
        [](HashValueType& seed, const ProcessProperties& o) noexcept { boost::hash_combine(seed, o.uTime); }
    },
    {
        ProcessProperties::FieldIndices::CpuUsage,
        "CpuUsage",
        [](const ProcessProperties& l, const ProcessProperties& r) noexcept { return std::abs(l.cpuUsage - r.cpuUsage) < 0.01; },
        [](HashValueType& seed, const ProcessProperties& o) noexcept { boost::hash_combine(seed, o.cpuUsage); }
    },
    {
        ProcessProperties::FieldIndices::Tty,
        "Tty",
        [](const ProcessProperties& l, const ProcessProperties& r) noexcept { return l.tty == r.tty; },
        [](HashValueType& seed, const ProcessProperties& o) noexcept { boost::hash_combine(seed, o.tty); }
    }
};

} // Er::ProcessTree {}