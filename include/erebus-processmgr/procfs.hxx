#pragma once

#include <erebus/log.hxx>
#include <erebus-processmgr/processmgr.hxx>


namespace Er
{

namespace ProcFs
{

constexpr uint64_t InvalidPid = uint64_t(-1);
constexpr uint64_t KernelPid = 0;

//
// parsed /proc/[pid]/stat entry
//

struct Stat
{
    bool valid = false;
    std::string error;

    /* 0*/ uint64_t pid = InvalidPid;
    /* 1*/ std::string comm;
    /* 2*/ char state = '?';
    /* 3*/ uint64_t ppid = InvalidPid;
    /* 4*/ uint64_t pgrp = InvalidPid;
    /* 5*/ uint64_t session = InvalidPid;
    /* 6*/ int32_t tty_nr = -1;
    /* 7*/ uint64_t tpgid = InvalidPid;
    /* 8*/ int32_t flags = 0;
    /* 9*/ uint64_t minflt = 0;                     // minor faults
    /*10*/ uint64_t cminflt = 0;                    // minor faults of waited children
    /*11*/ uint64_t majflt = 0;                     // major faults
    /*12*/ uint64_t cmajflt = 0;                    // major faults of waited children
    /*13*/ uint64_t utime = 0;
    /*14*/ uint64_t stime = 0;
    /*15*/ int64_t cutime = 0;
    /*16*/ int64_t cstime = 0;
    /*17*/ int64_t priority = 0;
    /*18*/ int64_t nice = 0;
    /*19*/ int64_t num_threads = 0;
    /*20*/ int64_t itrealvalue = 0;                  // obsolete
    /*21*/ uint64_t starttime = 0;                   // start time (since reboot)
    /*22*/ uint64_t vsize = 0;                       // virtual memory size
    /*23*/ int64_t rss = 0;                          // resident size
    /*24*/ uint64_t rsslim = 0;
    /*25*/ uint64_t startcode = 0;
    /*26*/ uint64_t endcode = 0;
    /*27*/ uint64_t startstack = 0;
    /*28*/ uint64_t kstkesp = 0;
    /*29*/ uint64_t kstkeip = 0;
    /*30*/ uint64_t signal = 0;
    /*31*/ uint64_t blocked = 0;
    /*32*/ uint64_t sigignore = 0;
    /*33*/ uint64_t sigcatch = 0;
    /*34*/ uint64_t wchan = 0;
    /*35*/ uint64_t nswap = 0;
    /*36*/ uint64_t cnswap = 0;
    /*37*/ int32_t exit_signal = -1;
    /*38*/ int32_t processor = -1;
    /*39*/ int32_t rt_priority = 0;
    /*40*/ int32_t policy = 0;
    /*41*/ uint64_t delayacct_blkio_ticks = 0;
    /*42*/ uint64_t guest_time = 0;
    /*43*/ int64_t cguest_time = 0;
    /*44*/ uint64_t start_data = 0;
    /*45*/ uint64_t end_data = 0;
    /*46*/ uint64_t start_brk = 0;
    /*47*/ uint64_t arg_start = 0;
    /*48*/ uint64_t arg_end = 0;
    /*49*/ uint64_t env_start = 0;
    /*50*/ uint64_t env_end = 0;
    /*51*/ int32_t exit_code = 0;

    uint64_t startTime = 0;                          // start time (absolute)
    uint64_t ruid = uint64_t(-1);                    // real user ID of process owner
    double uTime = 0.0;                              // seconds
    double sTime = 0.0;                              // seconds

    Stat() noexcept = default;
};


//
// /proc parser
// 

class ER_PROCESSMGR_EXPORT ProcFs final
    : public Er::NonCopyable
{
public:
    explicit ProcFs(Er::Log::ILog* log);

    static std::string root();

    Stat readStat(uint64_t pid) noexcept;
    std::string readComm(uint64_t pid) noexcept;
    std::string readExePath(uint64_t pid) noexcept;
    std::string readCmdLine(uint64_t pid) noexcept;

    std::vector<uint64_t> enumeratePids() noexcept;
    
    uint64_t bootTime() noexcept;

private:
    uint64_t getBootTimeImpl() noexcept;
    uint64_t fromRelativeTime(uint64_t relative) noexcept;

    Er::Log::ILog* const m_log;
    uint64_t const m_clkTck; // ticks per second
    std::size_t m_pidCountMax = 0;
};




} // namespace ProcFs {}

} // namespace Er {}