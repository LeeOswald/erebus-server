#pragma once

#include <erebus/flags.hxx>
#include <erebus/knownprops.hxx>
#include <erebus-processmgr/processmgr.hxx>

#include <vector>

namespace Er
{

namespace ProcessRequests
{

static const std::string_view ListProcesses = "ListProcesses";
static const std::string_view ListProcessesDiff = "ListProcessesDiff";
static const std::string_view ProcessDetails = "ProcessDetails";
static const std::string_view ProcessesGlobal = "ProcessesGlobal";

} // namespace ProcessRequests {}

namespace ProcessProps
{

struct IconFormatter
{
    void operator()(const Property& v, std::ostream& s) 
    { 
        auto ico = std::any_cast<Bytes>(v.value); 
        if (ico.empty())
            s << "[null icon]";
        else
            s << "[icon (" << ico.size() << " bytes)]";
    }
};

using RequiredFields = PropertyValue<uint64_t, ER_PROPID("process.fields"), "__Fields">;
using Error = PropertyValue<std::string, ER_PROPID("process.error"), "__Error">;
using Valid = PropertyValue<bool, ER_PROPID("process.valid"), "__Valid">;
using IsNew = PropertyValue<bool, ER_PROPID("process.new"), "__New">;
using IsDeleted = PropertyValue<bool, ER_PROPID("process.deleted"), "__Deleted">;
using Pid = PropertyValue<uint64_t, ER_PROPID("process.pid"), "PID">;
using PPid = PropertyValue<uint64_t, ER_PROPID("process.ppid"), "Parent PID">;
using PGrp = PropertyValue<uint64_t, ER_PROPID("process.pgrp"), "Process Group ID">;
using Tpgid = PropertyValue<uint64_t, ER_PROPID("process.tpgid"), "Process Group ID of the Terminal">;
using Session = PropertyValue<uint64_t, ER_PROPID("process.session"), "Session ID">;
using Ruid = PropertyValue<uint64_t, ER_PROPID("process.ruid"), "User ID">;
using User = PropertyValue<std::string, ER_PROPID("process.user"), "User Name">;
using Comm = PropertyValue<std::string, ER_PROPID("process.comm"), "Program Name">;
using CmdLine = PropertyValue<std::string, ER_PROPID("process.cmdline"), "Command Line">;
using Exe = PropertyValue<std::string, ER_PROPID("process.exe"), "Executable Name">;
using StartTime = PropertyValue<uint64_t, ER_PROPID("process.starttime"), "Start Time", PropertyComparator<uint64_t>, TimeFormatter<"%H:%M:%S %d %b %y", TimeZone::Utc>>;
using State = PropertyValue<std::string, ER_PROPID("process.state"), "State">;
using Icon = PropertyValue<Bytes, ER_PROPID("process.icon"), "Icon", BytesComparator, IconFormatter>;


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
    /* 9*/ StartTime::Id::value,
    /*10*/ State::Id::value,
    /*11*/ User::Id::value,
    /*12*/ Icon::Id::value,
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
    static constexpr Flag StartTime = 9;
    static constexpr Flag State = 10;
    static constexpr Flag User = 11;
    static constexpr Flag Icon = 12;

    static constexpr size_t FlagsCount = 64;
};


using PropMask = Flags<PropIndices>;


namespace Private
{

inline void registerAll(Er::Log::ILog* log)
{
    registerProperty(std::make_shared<PropertyInfoWrapper<RequiredFields>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<Error>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<Valid>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<IsNew>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<IsDeleted>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<Pid>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<PPid>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<PGrp>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<Tpgid>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<Session>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<Ruid>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<User>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<Comm>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<CmdLine>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<Exe>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<StartTime>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<State>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<Icon>>(), log);
}

inline void unregisterAll(Er::Log::ILog* log)
{
    unregisterProperty(lookupProperty(ProcessProps::RequiredFields::Id::value), log);
    unregisterProperty(lookupProperty(ProcessProps::Error::Id::value), log);
    unregisterProperty(lookupProperty(ProcessProps::Valid::Id::value), log);
    unregisterProperty(lookupProperty(ProcessProps::IsNew::Id::value), log);
    unregisterProperty(lookupProperty(ProcessProps::IsDeleted::Id::value), log);
    unregisterProperty(lookupProperty(ProcessProps::Pid::Id::value), log);
    unregisterProperty(lookupProperty(ProcessProps::PPid::Id::value), log);
    unregisterProperty(lookupProperty(ProcessProps::PGrp::Id::value), log);
    unregisterProperty(lookupProperty(ProcessProps::Tpgid::Id::value), log);
    unregisterProperty(lookupProperty(ProcessProps::Session::Id::value), log);
    unregisterProperty(lookupProperty(ProcessProps::Ruid::Id::value), log);
    unregisterProperty(lookupProperty(ProcessProps::User::Id::value), log);
    unregisterProperty(lookupProperty(ProcessProps::Comm::Id::value), log);
    unregisterProperty(lookupProperty(ProcessProps::CmdLine::Id::value), log);
    unregisterProperty(lookupProperty(ProcessProps::Exe::Id::value), log);
    unregisterProperty(lookupProperty(ProcessProps::StartTime::Id::value), log);
    unregisterProperty(lookupProperty(ProcessProps::State::Id::value), log);
    unregisterProperty(lookupProperty(ProcessProps::Icon::Id::value), log);
}

} // namespace Private {}

} // namespace ProcessProps {}

namespace ProcessesGlobal
{

using RequiredFields = PropertyValue<uint64_t, ER_PROPID("processes.global.fields"), "__Fields">;
using Lazy = PropertyValue<bool, ER_PROPID("processes.global.lazy"), "__Lazy">;
using ProcessCount = PropertyValue<uint64_t, ER_PROPID("processes.global.process_count"), "Total Processes">;

constexpr PropId IndexToProp[] =
{
    /* 0*/ ProcessCount::Id::value,
};


struct PropIndices
{
    static constexpr Flag ProcessCount = 0;

    static constexpr size_t FlagsCount = 64;
};


using PropMask = Flags<PropIndices>;

namespace Private
{

inline void registerAll(Er::Log::ILog* log)
{
    registerProperty(std::make_shared<PropertyInfoWrapper<RequiredFields>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<Lazy>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<ProcessCount>>(), log);
}

inline void unregisterAll(Er::Log::ILog* log)
{
    unregisterProperty(lookupProperty(ProcessesGlobal::RequiredFields::Id::value), log);
    unregisterProperty(lookupProperty(ProcessesGlobal::Lazy::Id::value), log);
    unregisterProperty(lookupProperty(ProcessesGlobal::ProcessCount::Id::value), log);
}

} // namespace Private {}

} // namespace ProcessesGlobal {}

} // namespace Er {}
