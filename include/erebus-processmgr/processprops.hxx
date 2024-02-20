#pragma once

#include <erebus/flags.hxx>
#include <erebus/knownprops.hxx>
#include <erebus-processmgr/processmgr.hxx>


namespace Er
{

namespace ProcessRequests
{

static const std::string ListProcesses = "ListProcesses";
static const std::string ProcessDetails = "ProcessDetails";

} // namespace ProcessRequests {}

namespace ProcessProps
{

using RequiredFields = PropertyInfo<uint64_t, ER_PROPID("process.fields"), "__Fields", PropertyFormatter<uint64_t>>;
using Error = PropertyInfo<std::string, ER_PROPID("process.error"), "__Error", PropertyFormatter<std::string>>;
using Valid = PropertyInfo<bool, ER_PROPID("process.valid"), "__Valid", PropertyFormatter<bool>>;
using Pid = PropertyInfo<uint64_t, ER_PROPID("process.pid"), "PID", PropertyFormatter<uint64_t>>;
using PPid = PropertyInfo<uint64_t, ER_PROPID("process.ppid"), "Parent PID", PropertyFormatter<uint64_t>>;
using PGrp = PropertyInfo<uint64_t, ER_PROPID("process.pgrp"), "Process Group ID", PropertyFormatter<uint64_t>>;
using Tpgid = PropertyInfo<uint64_t, ER_PROPID("process.tpgid"), "Process Group ID of the Terminal", PropertyFormatter<uint64_t>>;
using Session = PropertyInfo<uint64_t, ER_PROPID("process.session"), "Session ID", PropertyFormatter<uint64_t>>;
using Ruid = PropertyInfo<uint64_t, ER_PROPID("process.ruid"), "User ID", PropertyFormatter<uint64_t>>;
using StatComm = PropertyInfo<std::string, ER_PROPID("process.stat_comm"), "Command Name", PropertyFormatter<std::string>>;
using Comm = PropertyInfo<std::string, ER_PROPID("process.comm"), "Program Name", PropertyFormatter<std::string>>;
using CmdLine = PropertyInfo<std::string, ER_PROPID("process.cmdline"), "Command Line", PropertyFormatter<std::string>>;
using Exe = PropertyInfo<std::string, ER_PROPID("process.exe"), "Executable Name", PropertyFormatter<std::string>>;


constexpr PropId IndexToProp[] =
{
    /* 0*/ Pid::Id::value,
    /* 1*/ PPid::Id::value,
    /* 2*/ PGrp::Id::value,
    /* 3*/ Tpgid::Id::value,
    /* 4*/ Session::Id::value,
    /* 5*/ Ruid::Id::value,
    /* 6*/ Comm::Id::value,
    /* 7*/ CmdLine::Id::value,
    /* 8*/ Exe::Id::value,
};


struct PropIndices
{
    static constexpr Flag Pid = 0;
    static constexpr Flag PPid = 1;
    static constexpr Flag PGrp = 2;
    static constexpr Flag Tpgid = 3;
    static constexpr Flag Session = 4;
    static constexpr Flag Ruid = 5;
    static constexpr Flag Comm = 6;
    static constexpr Flag CmdLine = 7;
    static constexpr Flag Exe = 8;

    static constexpr size_t FlagsCount = 8;
};


using PropMask = Flags<PropIndices>;


namespace Private
{

inline void registerAll()
{
    registerProperty(std::make_shared<PropertyInfoWrapper<RequiredFields>>());
    registerProperty(std::make_shared<PropertyInfoWrapper<Error>>());
    registerProperty(std::make_shared<PropertyInfoWrapper<Valid>>());
    registerProperty(std::make_shared<PropertyInfoWrapper<Pid>>());
    registerProperty(std::make_shared<PropertyInfoWrapper<PPid>>());
    registerProperty(std::make_shared<PropertyInfoWrapper<PGrp>>());
    registerProperty(std::make_shared<PropertyInfoWrapper<Tpgid>>());
    registerProperty(std::make_shared<PropertyInfoWrapper<Session>>());
    registerProperty(std::make_shared<PropertyInfoWrapper<Ruid>>());
    registerProperty(std::make_shared<PropertyInfoWrapper<StatComm>>());
    registerProperty(std::make_shared<PropertyInfoWrapper<Comm>>());
    registerProperty(std::make_shared<PropertyInfoWrapper<CmdLine>>());
    registerProperty(std::make_shared<PropertyInfoWrapper<Exe>>());
}

inline void unregisterAll()
{
    unregisterProperty(lookupProperty(ProcessProps::RequiredFields::Id::value));
    unregisterProperty(lookupProperty(ProcessProps::Error::Id::value));
    unregisterProperty(lookupProperty(ProcessProps::Valid::Id::value));
    unregisterProperty(lookupProperty(ProcessProps::Pid::Id::value));
    unregisterProperty(lookupProperty(ProcessProps::PPid::Id::value));
    unregisterProperty(lookupProperty(ProcessProps::PGrp::Id::value));
    unregisterProperty(lookupProperty(ProcessProps::Tpgid::Id::value));
    unregisterProperty(lookupProperty(ProcessProps::Session::Id::value));
    unregisterProperty(lookupProperty(ProcessProps::Ruid::Id::value));
    unregisterProperty(lookupProperty(ProcessProps::StatComm::Id::value));
    unregisterProperty(lookupProperty(ProcessProps::Comm::Id::value));
    unregisterProperty(lookupProperty(ProcessProps::CmdLine::Id::value));
    unregisterProperty(lookupProperty(ProcessProps::Exe::Id::value));
}

} // namespace Private {}

} // namespace ProcessProps {}

} // namespace Er {}
