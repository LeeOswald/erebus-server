#include "common.hpp"

#include <erebus/proctree/linux/procfs.hxx>
#include <erebus/rtl/system/posix_error.hxx>

using namespace Er;
using namespace Er::ProcessTree;
using namespace Er::ProcessTree::Linux;


static void dumpStat(Pid pid, ProcFs::Stat const& stat)
{
    ErLogIndent(Er::Log::Level::Info, "[{}] --------------", pid);
    ErLogInfo("pid: {}", stat.pid);
    ErLogInfo("ppid: {}", stat.ppid);
    ErLogInfo("comm: {}", stat.comm);
    ErLogInfo("state: {}", stat.state);
    ErLogInfo("pgrp: {}", stat.pgrp);
    ErLogInfo("session: {}", stat.session);
    ErLogInfo("tpgid: {}", stat.tpgid);
    ErLogInfo("tty: {}", stat.tty_nr);
    ErLogInfo("ruid: {}", stat.ruid);
    ErLogInfo("utime: {:.2f}", stat.uTime);
    ErLogInfo("stime: {:.2f}", stat.sTime);
    auto tm = stat.startTime.toLocalTime();
    ErLogInfo("start time: {:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

TEST(ProcFs, enumeratePids)
{
    ProcFs proc;

    EXPECT_GT(proc.bootTime().value, 0);
    EXPECT_LT(proc.bootTime().value, Er::System::PackedTime::now());
    auto tm = proc.bootTime().toLocalTime();
    ErLogInfo("System boot time {:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);

    auto pids_ = proc.enumeratePids();
    EXPECT_TRUE(pids_.has_value());
    auto& pids = pids_.value();
    EXPECT_GT(pids.size(), 1);

    for (auto pid: pids)
    {
        auto stat_ = proc.readStat(pid);
        if (!stat_.has_value())
        {
            ErLogError("Failed to read /proc/{}/stat: {} ({})", pid, stat_.error(), System::posixErrorToString(stat_.error()));
        }
        else
        {
            dumpStat(pid, stat_.value());
        }
    }
}