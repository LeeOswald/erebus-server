#pragma once

#include <erebus/knownprops.hxx>
#include <erebus-processmgr/processmgr.hxx>


namespace Er
{

namespace ProcessProps
{


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

namespace Private
{

inline void registerAll()
{
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
    unregisterProperty(lookupProperty(ER_PROPID_("process.error")));
    unregisterProperty(lookupProperty(ER_PROPID_("process.valid")));
    unregisterProperty(lookupProperty(ER_PROPID_("process.pid")));
    unregisterProperty(lookupProperty(ER_PROPID_("process.ppid")));
    unregisterProperty(lookupProperty(ER_PROPID_("process.pgrp")));
    unregisterProperty(lookupProperty(ER_PROPID_("process.tpgid")));
    unregisterProperty(lookupProperty(ER_PROPID_("process.session")));
    unregisterProperty(lookupProperty(ER_PROPID_("process.ruid")));
    unregisterProperty(lookupProperty(ER_PROPID_("process.stat_comm")));
    unregisterProperty(lookupProperty(ER_PROPID_("process.comm")));
    unregisterProperty(lookupProperty(ER_PROPID_("process.cmdline")));
    unregisterProperty(lookupProperty(ER_PROPID_("process.exe")));
}

} // namespace Private {}

} // namespace ProcessProps {}

} // namespace Er {}
