#pragma once

#include <erebus/proctree/proctree.hxx>
#include <erebus/rtl/error.hxx>
#include <erebus/rtl/log.hxx>
#include <erebus/rtl/multi_string.hxx>
#include <erebus/rtl/time.hxx>

#include <expected>
#include <vector>

#include <boost/noncopyable.hpp>


namespace Er::ProcessTree::Linux
{

class ER_PROCTREE_EXPORT ProcFs final
    : public boost::noncopyable
{
public:
    struct Stat
    {
        /* 0*/ std::int64_t pid = -1;
        /* 1*/ std::string comm;
        /* 2*/ char state = '?';
        /* 3*/ std::int64_t ppid = -1;
        /* 4*/ std::int64_t pgrp = -1;
        /* 5*/ std::int64_t session = -1;
        /* 6*/ std::int32_t tty_nr = -1;
        /* 7*/ std::int64_t tpgid = -1;
        /* 8*/ std::int32_t flags = 0;
        /* 9*/ std::uint64_t minflt = 0;                     // minor faults
        /*10*/ std::uint64_t cminflt = 0;                    // minor faults of waited children
        /*11*/ std::uint64_t majflt = 0;                     // major faults
        /*12*/ std::uint64_t cmajflt = 0;                    // major faults of waited children
        /*13*/ std::uint64_t utime = 0;                      // clock ticks
        /*14*/ std::uint64_t stime = 0;                      // clock ticks
        /*15*/ std::int64_t cutime = 0;
        /*16*/ std::int64_t cstime = 0;
        /*17*/ std::int64_t priority = 0;
        /*18*/ std::int64_t nice = 0;
        /*19*/ std::int64_t num_threads = 0;
        /*20*/ std::int64_t itrealvalue = 0;                  // obsolete
        /*21*/ std::uint64_t starttime = 0;                   // start time (seconds since reboot)
        /*22*/ std::uint64_t vsize = 0;                       // virtual memory size
        /*23*/ std::int64_t rss = 0;                          // resident size
        /*24*/ std::uint64_t rsslim = 0;
        /*25*/ std::uint64_t startcode = 0;
        /*26*/ std::uint64_t endcode = 0;
        /*27*/ std::uint64_t startstack = 0;
        /*28*/ std::uint64_t kstkesp = 0;
        /*29*/ std::uint64_t kstkeip = 0;
        /*30*/ std::uint64_t signal = 0;
        /*31*/ std::uint64_t blocked = 0;
        /*32*/ std::uint64_t sigignore = 0;
        /*33*/ std::uint64_t sigcatch = 0;
        /*34*/ std::uint64_t wchan = 0;
        /*35*/ std::uint64_t nswap = 0;
        /*36*/ std::uint64_t cnswap = 0;
        /*37*/ std::int32_t exit_signal = -1;
        /*38*/ std::int32_t processor = -1;
        /*39*/ std::int32_t rt_priority = 0;
        /*40*/ std::int32_t policy = 0;
        /*41*/ std::uint64_t delayacct_blkio_ticks = 0;
        /*42*/ std::uint64_t guest_time = 0;
        /*43*/ std::int64_t cguest_time = 0;
        /*44*/ std::uint64_t start_data = 0;
        /*45*/ std::uint64_t end_data = 0;
        /*46*/ std::uint64_t start_brk = 0;
        /*47*/ std::uint64_t arg_start = 0;
        /*48*/ std::uint64_t arg_end = 0;
        /*49*/ std::uint64_t env_start = 0;
        /*50*/ std::uint64_t env_end = 0;
        /*51*/ std::int32_t exit_code = 0;

        Time startTime;                                       // start time (absolute)
        std::uint64_t ruid = std::uint64_t(-1);               // real user ID of process owner

        Stat() noexcept = default;
    };

    ~ProcFs() = default;

    explicit ProcFs(std::string_view procFsRoot = std::string_view("/proc"));

    constexpr Time bootTime() noexcept
    {
        return Time::fromSeconds(m_bootTime);
    }

    static Time timeFromTicks(std::uint64_t ticks) noexcept;

    std::expected<std::vector<Pid>, Error> enumeratePids();
    std::expected<Stat, Error> readStat(Pid pid);
    std::expected<std::string, Error> readComm(Pid pid);
    std::expected<std::string, Error> readExePath(Pid pid);
    std::expected<MultiStringZ, Error> readCmdLine(Pid pid);
    std::expected<MultiStringZ, Error> readEnv(Pid pid);

private:
    std::uint64_t getBootTimeImpl();
    
    const std::string m_procFsRoot;
    const std::uint64_t m_bootTime; // seconds
    const int m_cpusMax;
    std::size_t m_pidCountMax = 0;
};

} // namespace Er::ProcessTree::Linux {}