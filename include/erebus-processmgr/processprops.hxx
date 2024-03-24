#pragma once

#include <erebus/flags.hxx>
#include <erebus/knownprops.hxx>
#include <erebus-processmgr/processmgr.hxx>

#include <iomanip>
#include <vector>

namespace Er
{

namespace ProcessRequests
{

static const std::string_view ListProcesses = "ListProcesses";
static const std::string_view ListProcessesDiff = "ListProcessesDiff";
static const std::string_view ProcessDetails = "ProcessDetails";
static const std::string_view ProcessesGlobal = "ProcessesGlobal";
static const std::string_view KillProcess = "KillProcess";

} // namespace ProcessRequests {}


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

struct CpuTimeFormatter
{
    void operator()(const Property& v, std::ostream& s) 
    { 
        auto val = std::any_cast<double>(v.value); 
        s << std::fixed << std::setprecision(2) << val;
    }
};


struct CpuLoadFormatter
{
    void operator()(const Property& v, std::ostream& s) 
    { 
        auto val = std::any_cast<double>(v.value);
        val *= 100; 
        val = std::clamp(val, 0.0, 100.0);
        s << std::fixed << std::setprecision(2) << static_cast<unsigned>(val);
    }
};


namespace ProcessProps
{


using RequiredFields = PropertyValue<uint64_t, ER_PROPID("process.fields"), "__Fields">;
using Error = PropertyValue<std::string, ER_PROPID("process.error"), "__Error">;
using Valid = PropertyValue<bool, ER_PROPID("process.valid"), "__Valid">;
using IsNew = PropertyValue<bool, ER_PROPID("process.new"), "__New">;
using IsDeleted = PropertyValue<bool, ER_PROPID("process.deleted"), "__Deleted">;
using Pid = PropertyValue<uint64_t, ER_PROPID("process.pid"), "PID">;
using PPid = PropertyValue<uint64_t, ER_PROPID("process.ppid"), "Parent PID">;
using PGrp = PropertyValue<uint64_t, ER_PROPID("process.pgrp"), "Process Group ID">;
using Tpgid = PropertyValue<uint64_t, ER_PROPID("process.tpgid"), "Process Group ID of the Terminal">;
using Tty = PropertyValue<int32_t, ER_PROPID("process.tty"), "Terminal">;
using Session = PropertyValue<uint64_t, ER_PROPID("process.session"), "Session ID">;
using Ruid = PropertyValue<uint64_t, ER_PROPID("process.ruid"), "User ID">;
using User = PropertyValue<std::string, ER_PROPID("process.user"), "User Name">;
using Comm = PropertyValue<std::string, ER_PROPID("process.comm"), "Program Name">;
using CmdLine = PropertyValue<std::string, ER_PROPID("process.cmdline"), "Command Line">;
using Exe = PropertyValue<std::string, ER_PROPID("process.exe"), "Executable">;
using StartTime = PropertyValue<uint64_t, ER_PROPID("process.starttime"), "Start Time", PropertyComparator<uint64_t>, TimeFormatter<"%H:%M:%S %d %b %y", TimeZone::Utc>>;
using State = PropertyValue<std::string, ER_PROPID("process.state"), "State">;
using Icon = PropertyValue<Bytes, ER_PROPID("process.icon"), "Icon", BytesComparator, IconFormatter>;
using ThreadCount = PropertyValue<int64_t, ER_PROPID("process.nthreads"), "Thread Count">;
using STime = PropertyValue<double, ER_PROPID("process.stime"), "CPU Time (System)", PropertyComparator<double>, CpuTimeFormatter>;
using UTime = PropertyValue<double, ER_PROPID("process.utime"), "CPU Time (User)", PropertyComparator<double>, CpuTimeFormatter>;
using CpuUsage = PropertyValue<double, ER_PROPID("process.cpu_usage"), "%CPU", PropertyComparator<double>, CpuLoadFormatter>;

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
    /*13*/ ThreadCount::Id::value,
    /*14*/ STime::Id::value,
    /*15*/ UTime::Id::value,
    /*16*/ CpuUsage::Id::value,
    /*17*/ Tty::Id::value,
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
    static constexpr Flag ThreadCount = 13;
    static constexpr Flag STime = 14;
    static constexpr Flag UTime = 15;
    static constexpr Flag CpuUsage = 16;
    static constexpr Flag Tty = 17;

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
    registerProperty(std::make_shared<PropertyInfoWrapper<ThreadCount>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<STime>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<UTime>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<CpuUsage>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<Tty>>(), log);
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
    unregisterProperty(lookupProperty(ProcessProps::ThreadCount::Id::value), log);
    unregisterProperty(lookupProperty(ProcessProps::STime::Id::value), log);
    unregisterProperty(lookupProperty(ProcessProps::UTime::Id::value), log);
    unregisterProperty(lookupProperty(ProcessProps::CpuUsage::Id::value), log);
    unregisterProperty(lookupProperty(ProcessProps::Tty::Id::value), log);
}

} // namespace Private {}

} // namespace ProcessProps {}

namespace ProcessesGlobal
{

using Global = PropertyValue<bool, ER_PROPID("processes.global"), "__Global">;
using Pid = PropertyValue<uint64_t, ER_PROPID("processes.global.pid"), "PID">;
using Signal = PropertyValue<std::string, ER_PROPID("processes.global.signal"), "Signal">;
using PosixResult = PropertyValue<int32_t, ER_PROPID("processes.global.posix_result"), "POSIX Result">;
using ErrorText = PropertyValue<std::string, ER_PROPID("processes.global.error_text"), "Error Message">;

using RequiredFields = PropertyValue<uint64_t, ER_PROPID("processes.global.fields"), "__Fields">;

using ProcessCount = PropertyValue<uint64_t, ER_PROPID("processes.global.process_count"), "Total Processes">;
using RealTime = PropertyValue<double, ER_PROPID("processes.global.real_time"), "Real Time", PropertyComparator<double>, CpuTimeFormatter>;
using IdleTime = PropertyValue<double, ER_PROPID("processes.global.idle_time"), "CPU Time (Idle)", PropertyComparator<double>, CpuTimeFormatter>;
using UserTime = PropertyValue<double, ER_PROPID("processes.global.user_time"), "CPU Time (User)", PropertyComparator<double>, CpuTimeFormatter>;
using SystemTime = PropertyValue<double, ER_PROPID("processes.global.system_time"), "CPU Time (System)", PropertyComparator<double>, CpuTimeFormatter>;
using VirtualTime = PropertyValue<double, ER_PROPID("processes.global.user_time"), "CPU Time (Virtual)", PropertyComparator<double>, CpuTimeFormatter>;
using TotalTime = PropertyValue<double, ER_PROPID("processes.global.total_time"), "Total CPU Time", PropertyComparator<double>, CpuTimeFormatter>;

constexpr PropId IndexToProp[] =
{
    /* 0*/ ProcessCount::Id::value,
    /* 1*/ RealTime::Id::value,
    /* 2*/ IdleTime::Id::value,
    /* 3*/ UserTime::Id::value,
    /* 4*/ SystemTime::Id::value,
    /* 5*/ VirtualTime::Id::value,
    /* 6*/ TotalTime::Id::value,
};


struct PropIndices
{
    static constexpr Flag ProcessCount = 0;
    static constexpr Flag RealTime = 1;
    static constexpr Flag IdleTime = 2;
    static constexpr Flag UserTime = 3;
    static constexpr Flag SystemTime = 4;
    static constexpr Flag VirtualTime = 5;
    static constexpr Flag TotalTime = 6;

    static constexpr size_t FlagsCount = 64;
};


using PropMask = Flags<PropIndices>;

namespace Private
{

inline void registerAll(Er::Log::ILog* log)
{
    registerProperty(std::make_shared<PropertyInfoWrapper<Pid>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<Signal>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<PosixResult>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<ErrorText>>(), log);

    registerProperty(std::make_shared<PropertyInfoWrapper<RequiredFields>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<Global>>(), log);
    
    registerProperty(std::make_shared<PropertyInfoWrapper<ProcessCount>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<RealTime>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<IdleTime>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<UserTime>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<SystemTime>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<VirtualTime>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<TotalTime>>(), log);
}

inline void unregisterAll(Er::Log::ILog* log)
{
    unregisterProperty(lookupProperty(ProcessesGlobal::Pid::Id::value), log);
    unregisterProperty(lookupProperty(ProcessesGlobal::Signal::Id::value), log);
    unregisterProperty(lookupProperty(ProcessesGlobal::PosixResult::Id::value), log);
    unregisterProperty(lookupProperty(ProcessesGlobal::ErrorText::Id::value), log);
    
    unregisterProperty(lookupProperty(ProcessesGlobal::RequiredFields::Id::value), log);
    unregisterProperty(lookupProperty(ProcessesGlobal::Global::Id::value), log);
    
    unregisterProperty(lookupProperty(ProcessesGlobal::ProcessCount::Id::value), log);
    unregisterProperty(lookupProperty(ProcessesGlobal::RealTime::Id::value), log);
    unregisterProperty(lookupProperty(ProcessesGlobal::IdleTime::Id::value), log);
    unregisterProperty(lookupProperty(ProcessesGlobal::UserTime::Id::value), log);
    unregisterProperty(lookupProperty(ProcessesGlobal::SystemTime::Id::value), log);
    unregisterProperty(lookupProperty(ProcessesGlobal::VirtualTime::Id::value), log);
    unregisterProperty(lookupProperty(ProcessesGlobal::TotalTime::Id::value), log);
}

} // namespace Private {}

} // namespace ProcessesGlobal {}

} // namespace Er {}
