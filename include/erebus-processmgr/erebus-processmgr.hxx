#pragma once

#include <erebus/flags.hxx>
#include <erebus/knownprops.hxx>
#include <erebus-processmgr/erebus-processmgr.hxx>

#include <iomanip>
#include <vector>


#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef ER_PROCESSMGR_EXPORTS
        #define ER_PROCESSMGR_EXPORT __declspec(dllexport)
    #else
        #define ER_PROCESSMGR_EXPORT __declspec(dllimport)
    #endif
#else
    #define ER_PROCESSMGR_EXPORT __attribute__((visibility("default")))
#endif

namespace Er::ProcessMgr
{

constexpr const std::string_view Domain = "process";

namespace Requests
{

static const std::string_view ListProcessesDiff = "process_list";
static const std::string_view ProcessProps = "process_props";
static const std::string_view ProcessPropsExt = "process_props_ext";
static const std::string_view GlobalProps = "global_props";
static const std::string_view KillProcess = "kill_process";

} // namespace Requests {}


struct ProcessStateFormatter
{
    std::string operator()(ConstPropertyRef v)
    {
        auto pp = std::get_if<const uint32_t*>(&v);
        if (!pp || !*pp)
            return std::string("<null>");

        auto val = **pp;
        return std::string(1, char(val));
    }
};

struct CpuTimeFormatter
{
    std::string operator()(ConstPropertyRef v)
    {
        auto pp = std::get_if<const double*>(&v);
        if (!pp || !*pp)
            return std::string("<null>");
        
        auto val = **pp;
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << val;
        return ss.str();
    }
};

struct CpuLoadFormatter
{
    std::string operator()(ConstPropertyRef v)
    {
        auto pp = std::get_if<const double*>(&v);
        if (!pp || !*pp)
            return std::string("<null>");
        
        auto val = **pp;
        std::ostringstream ss;

        val *= 100; 
        val = std::clamp(val, 0.0, 100.0);
        ss << static_cast<unsigned>(val);
        return ss.str();
    }
};

struct MemUnitFormatter
{
    std::string operator()(ConstPropertyRef v)
    {
        auto pp = std::get_if<const uint64_t*>(&v);
        if (!pp || !*pp)
            return std::string("<null>");
        
        auto val = **pp;
        std::ostringstream ss;
 
        if (val < 10ULL * 1024)
            ss << val << " B";
        else if (val < 10ULL * 1024 * 1024)
            ss << (val / 1024ULL) << " kB";
        else if (val < 10ULL * 1024 * 1024 * 1024)
            ss << (val / (1024ULL * 1024)) << " MB";
        else
            ss << (val / (1024ULL * 1024 * 1024)) << " GB";

        return ss.str();
    }
};


namespace Props
{

using Error = PropertyValue<std::string, ER_PROPID("__error"), "__Error">;
using Valid = PropertyValue<Bool, ER_PROPID("__valid"), "__Valid">;
using IsNew = PropertyValue<Bool, ER_PROPID("__new"), "__New">;
using IsDeleted = PropertyValue<Bool, ER_PROPID("__deleted"), "__Deleted">;
using Pid = PropertyValue<uint64_t, ER_PROPID("pid"), "PID">;
using SignalName = PropertyValue<std::string, ER_PROPID("signal_name"), "Signal">;
using PosixResult = PropertyValue<int32_t, ER_PROPID("posix_result"), "POSIX Result">;
using ErrorText = PropertyValue<std::string, ER_PROPID("error_text"), "Error Message">;



} // namespace Props {}

namespace ProcessProps
{

using RequiredFields = PropertyValue<uint64_t, ER_PROPID("__processprops_required"), "__Fields">;
using PPid = PropertyValue<uint64_t, ER_PROPID("ppid"), "Parent PID">;
using PGrp = PropertyValue<uint64_t, ER_PROPID("pgrp"), "Process Group ID">;
using Tpgid = PropertyValue<uint64_t, ER_PROPID("tpgid"), "Process Group ID of the Terminal">;
using Tty = PropertyValue<int32_t, ER_PROPID("tty"), "Terminal">;
using Session = PropertyValue<uint64_t, ER_PROPID("session"), "Session ID">;
using Ruid = PropertyValue<uint64_t, ER_PROPID("ruid"), "User ID">;
using User = PropertyValue<std::string, ER_PROPID("user"), "User Name">;
using Comm = PropertyValue<std::string, ER_PROPID("comm"), "Program Name">;
using CmdLine = PropertyValue<std::string, ER_PROPID("cmdline"), "Command Line">;
using Exe = PropertyValue<std::string, ER_PROPID("exe"), "Executable">;
using StartTime = PropertyValue<uint64_t, ER_PROPID("starttime"), "Start Time", TimeFormatter<"%H:%M:%S %d %b %y", TimeZone::Utc>>;
using State = PropertyValue<uint32_t, ER_PROPID("state"), "State", ProcessStateFormatter>;
using ThreadCount = PropertyValue<int64_t, ER_PROPID("nthreads"), "Thread Count">;
using STime = PropertyValue<double, ER_PROPID("stime"), "CPU Time (System)", CpuTimeFormatter>;
using UTime = PropertyValue<double, ER_PROPID("utime"), "CPU Time (User)", CpuTimeFormatter>;
using CpuUsage = PropertyValue<double, ER_PROPID("cpu_usage"), "%CPU", CpuLoadFormatter>;


constexpr PropId IndexToProp[] =
{
    /* 0*/ Props::Pid::Id::value,
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
    /*12*/ ThreadCount::Id::value,
    /*13*/ STime::Id::value,
    /*14*/ UTime::Id::value,
    /*15*/ CpuUsage::Id::value,
    /*16*/ Tty::Id::value,
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
    static constexpr Flag ThreadCount = 12;
    static constexpr Flag STime = 13;
    static constexpr Flag UTime = 14;
    static constexpr Flag CpuUsage = 15;
    static constexpr Flag Tty = 16;
};


using PropMask = FlagsPack<64, PropIndices>;

} // namespace ProcessProps {}


namespace ProcessPropsExt
{


using RequiredFields = PropertyValue<uint64_t, ER_PROPID("__processpropsext_required"), "__Fields">;
using Env = PropertyValue<StringVector, ER_PROPID("env"), "Environment", PropertyFormatter<std::string>>;

constexpr PropId IndexToProp[] =
{
    /* 0*/ Env::Id::value,
};

struct PropIndices
{
    static constexpr Flag Env = 0;
};

using PropMask = FlagsPack<64, PropIndices>;

} // namespace ProcessPropsExt {}


namespace GlobalProps
{

using RequiredFields = PropertyValue<uint64_t, ER_PROPID("__global_required"), "__Fields">;
using Global = PropertyValue<Bool, ER_PROPID("__global"), "__Global">;

using ProcessCount = PropertyValue<uint64_t, ER_PROPID("process_count"), "Total Processes">;
using RealTime = PropertyValue<double, ER_PROPID("real_time"), "Real Time", CpuTimeFormatter>;
using IdleTime = PropertyValue<double, ER_PROPID("idle_time"), "CPU Time (Idle)", CpuTimeFormatter>;
using UserTime = PropertyValue<double, ER_PROPID("user_time"), "CPU Time (User)", CpuTimeFormatter>;
using SystemTime = PropertyValue<double, ER_PROPID("system_time"), "CPU Time (System)", CpuTimeFormatter>;
using VirtualTime = PropertyValue<double, ER_PROPID("virtual_time"), "CPU Time (Virtual)", CpuTimeFormatter>;
using TotalTime = PropertyValue<double, ER_PROPID("total_time"), "Total CPU Time", CpuTimeFormatter>;

using TotalMem = PropertyValue<uint64_t, ER_PROPID("total_mem"), "Total Mem", MemUnitFormatter>;
using UsedMem = PropertyValue<uint64_t, ER_PROPID("used_mem"), "Used Mem", MemUnitFormatter>;
using BuffersMem = PropertyValue<uint64_t, ER_PROPID("buffers_mem"), "Buffers", MemUnitFormatter>;
using CachedMem = PropertyValue<uint64_t, ER_PROPID("cached_mem"), "Cached Mem", MemUnitFormatter>;
using SharedMem = PropertyValue<uint64_t, ER_PROPID("shared_mem"), "Shared Mem", MemUnitFormatter>;
using AvailableMem = PropertyValue<uint64_t, ER_PROPID("avail_mem"), "Available Mem", MemUnitFormatter>;

using TotalSwap = PropertyValue<uint64_t, ER_PROPID("total_swap"), "Swap Total", MemUnitFormatter>;
using UsedSwap = PropertyValue<uint64_t, ER_PROPID("used_swap"), "Swap Used", MemUnitFormatter>;
using CachedSwap = PropertyValue<uint64_t, ER_PROPID("cached_swap"), "Swap Cached", MemUnitFormatter>;
using ZSwapComp = PropertyValue<uint64_t, ER_PROPID("comp_zswap"), "ZSwap Compressed", MemUnitFormatter>;
using ZSwapOrig = PropertyValue<uint64_t, ER_PROPID("orig_zswap"), "ZSwap Original", MemUnitFormatter>;

constexpr PropId IndexToProp[] =
{
    /* 0*/ ProcessCount::Id::value,
    /* 1*/ RealTime::Id::value,
    /* 2*/ IdleTime::Id::value,
    /* 3*/ UserTime::Id::value,
    /* 4*/ SystemTime::Id::value,
    /* 5*/ VirtualTime::Id::value,
    /* 6*/ TotalTime::Id::value,
    /* 7*/ TotalMem::Id::value,
    /* 8*/ UsedMem::Id::value,
    /* 9*/ BuffersMem::Id::value,
    /*10*/ CachedMem::Id::value,
    /*11*/ SharedMem::Id::value,
    /*12*/ AvailableMem::Id::value,
    /*13*/ TotalSwap::Id::value,
    /*14*/ UsedSwap::Id::value,
    /*15*/ CachedSwap::Id::value,
    /*16*/ ZSwapComp::Id::value,
    /*17*/ ZSwapOrig::Id::value,
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
    static constexpr Flag TotalMem = 7;
    static constexpr Flag UsedMem = 8;
    static constexpr Flag BuffersMem = 9;
    static constexpr Flag CachedMem = 10;
    static constexpr Flag SharedMem = 11;
    static constexpr Flag AvailableMem = 12;
    static constexpr Flag TotalSwap = 13;
    static constexpr Flag UsedSwap = 14;
    static constexpr Flag CachedSwap = 15;
    static constexpr Flag ZSwapComp = 16;
    static constexpr Flag ZSwapOrig = 17;
};


using PropMask = FlagsPack<64, PropIndices>;

} // namespace GlobalProps {}

namespace Private
{

inline void registerAll(Er::Log::ILog* log)
{
    registerProperty(Domain, Props::Error::make_info(), log);
    registerProperty(Domain, Props::Valid::make_info(), log);
    registerProperty(Domain, Props::IsNew::make_info(), log);
    registerProperty(Domain, Props::IsDeleted::make_info(), log);
    registerProperty(Domain, Props::Pid::make_info(), log);
    registerProperty(Domain, Props::SignalName::make_info(), log);
    registerProperty(Domain, Props::PosixResult::make_info(), log);
    registerProperty(Domain, Props::ErrorText::make_info(), log);
    
    registerProperty(Domain, ProcessProps::RequiredFields::make_info(), log);
    registerProperty(Domain, ProcessProps::PPid::make_info(), log);
    registerProperty(Domain, ProcessProps::PGrp::make_info(), log);
    registerProperty(Domain, ProcessProps::Tpgid::make_info(), log);
    registerProperty(Domain, ProcessProps::Session::make_info(), log);
    registerProperty(Domain, ProcessProps::Ruid::make_info(), log);
    registerProperty(Domain, ProcessProps::User::make_info(), log);
    registerProperty(Domain, ProcessProps::Comm::make_info(), log);
    registerProperty(Domain, ProcessProps::CmdLine::make_info(), log);
    registerProperty(Domain, ProcessProps::Exe::make_info(), log);
    registerProperty(Domain, ProcessProps::StartTime::make_info(), log);
    registerProperty(Domain, ProcessProps::State::make_info(), log);
    registerProperty(Domain, ProcessProps::ThreadCount::make_info(), log);
    registerProperty(Domain, ProcessProps::STime::make_info(), log);
    registerProperty(Domain, ProcessProps::UTime::make_info(), log);
    registerProperty(Domain, ProcessProps::CpuUsage::make_info(), log);
    registerProperty(Domain, ProcessProps::Tty::make_info(), log);

    registerProperty(Domain, ProcessPropsExt::RequiredFields::make_info(), log);
    registerProperty(Domain, ProcessPropsExt::Env::make_info(), log);

    registerProperty(Domain, GlobalProps::RequiredFields::make_info(), log);
    registerProperty(Domain, GlobalProps::Global::make_info(), log);
    registerProperty(Domain, GlobalProps::ProcessCount::make_info(), log);
    registerProperty(Domain, GlobalProps::RealTime::make_info(), log);
    registerProperty(Domain, GlobalProps::IdleTime::make_info(), log);
    registerProperty(Domain, GlobalProps::UserTime::make_info(), log);
    registerProperty(Domain, GlobalProps::SystemTime::make_info(), log);
    registerProperty(Domain, GlobalProps::VirtualTime::make_info(), log);
    registerProperty(Domain, GlobalProps::TotalTime::make_info(), log);
    registerProperty(Domain, GlobalProps::TotalMem::make_info(), log);
    registerProperty(Domain, GlobalProps::UsedMem::make_info(), log);
    registerProperty(Domain, GlobalProps::BuffersMem::make_info(), log);
    registerProperty(Domain, GlobalProps::CachedMem::make_info(), log);
    registerProperty(Domain, GlobalProps::SharedMem::make_info(), log);
    registerProperty(Domain, GlobalProps::AvailableMem::make_info(), log);
    registerProperty(Domain, GlobalProps::TotalSwap::make_info(), log);
    registerProperty(Domain, GlobalProps::UsedSwap::make_info(), log);
    registerProperty(Domain, GlobalProps::CachedSwap::make_info(), log);
    registerProperty(Domain, GlobalProps::ZSwapComp::make_info(), log);
    registerProperty(Domain, GlobalProps::ZSwapOrig::make_info(), log);

}

inline void unregisterAll(Er::Log::ILog* log)
{
    unregisterProperty(Domain, lookupProperty(Domain, Props::Error::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, Props::Valid::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, Props::IsNew::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, Props::IsDeleted::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, Props::Pid::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, Props::SignalName::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, Props::PosixResult::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, Props::ErrorText::Id::value), log);
    
    unregisterProperty(Domain, lookupProperty(Domain, ProcessProps::RequiredFields::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, ProcessProps::PPid::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, ProcessProps::PGrp::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, ProcessProps::Tpgid::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, ProcessProps::Session::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, ProcessProps::Ruid::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, ProcessProps::User::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, ProcessProps::Comm::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, ProcessProps::CmdLine::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, ProcessProps::Exe::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, ProcessProps::StartTime::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, ProcessProps::State::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, ProcessProps::ThreadCount::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, ProcessProps::STime::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, ProcessProps::UTime::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, ProcessProps::CpuUsage::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, ProcessProps::Tty::Id::value), log);

    unregisterProperty(Domain, lookupProperty(Domain, ProcessPropsExt::RequiredFields::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, ProcessPropsExt::Env::Id::value), log);

    unregisterProperty(Domain, lookupProperty(Domain, GlobalProps::RequiredFields::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, GlobalProps::Global::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, GlobalProps::ProcessCount::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, GlobalProps::RealTime::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, GlobalProps::IdleTime::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, GlobalProps::UserTime::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, GlobalProps::SystemTime::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, GlobalProps::VirtualTime::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, GlobalProps::TotalTime::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, GlobalProps::TotalMem::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, GlobalProps::UsedMem::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, GlobalProps::BuffersMem::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, GlobalProps::CachedMem::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, GlobalProps::SharedMem::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, GlobalProps::AvailableMem::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, GlobalProps::TotalSwap::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, GlobalProps::UsedSwap::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, GlobalProps::CachedSwap::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, GlobalProps::ZSwapComp::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, GlobalProps::ZSwapOrig::Id::value), log);

}

} // namespace Private {}


} // namespace Er::ProcessMgr {}

