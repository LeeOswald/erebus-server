#include "common.hpp"

#include <erebus/proctree/linux/procfs.hxx>
#include <erebus/rtl/util/string_util.hxx>

using namespace Er;
using namespace Er::ProcessTree;
using namespace Er::ProcessTree::Linux;


static void dumpStat(Pid pid, ProcFs& proc)
{
    ErLogIndent(Er::Log::Level::Info, "[{}] --------------", pid);
    auto stat_ = proc.readStat(pid);
    if (!stat_.has_value())
    {
        ErLogError("Failed to read /proc/{}/stat: {}", pid, stat_.error().message());
        return;
    }

    auto& stat = stat_.value();

    ErLogInfo("pid: {}", stat.pid);
    ErLogInfo("ppid: {}", stat.ppid);
    ErLogInfo("comm: {}", stat.comm);
    ErLogInfo("state: {}", stat.state);
    ErLogInfo("pgrp: {}", stat.pgrp);
    ErLogInfo("session: {}", stat.session);
    ErLogInfo("tpgid: {}", stat.tpgid);
    ErLogInfo("tty: {}", stat.tty_nr);
    ErLogInfo("ruid: {}", stat.ruid);
    ErLogInfo("utime: {:.2f}", proc.timeFromTicks(stat.utime).toMilliseconds() / 1000.0);
    ErLogInfo("stime: {:.2f}", proc.timeFromTicks(stat.stime).toMilliseconds() / 1000.0);
    auto tm = stat.startTime.toLocalTime();
    ErLogInfo("start time: {:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);

    auto comm_ = proc.readComm(pid);
    if (!comm_.has_value())
    {
        ErLogError("Failed to read /proc/{}/comm: {}", pid, comm_.error().message());
    }
    else
    {
        ErLogInfo("/comm: [{}]", comm_.value());
    }

    auto exe_ = proc.readExePath(pid);
    if (!exe_.has_value())
    {
        if (exe_.error() == ENOENT)
            ErLogInfo("/exe: [n/a]");
        else
            ErLogError("Failed to read /proc/{}/exe: {}", pid, exe_.error().message());
    }
    else
    {
        ErLogInfo("/exe: [{}]", exe_.value());
    }

    auto cmd_ = proc.readCmdLine(pid);
    if (!cmd_.has_value())
    {
        ErLogError("Failed to read /proc/{}/cmdline: {}", pid, cmd_.error().message());
    }
    else
    {
        auto cmd = cmd_.value().coalesce(' ');
        cmd = Er::Util::rtrim(cmd);

        ErLogInfo("/cmdline: [{}]", cmd);
    }

    auto env_ = proc.readEnv(pid);
    if (!env_.has_value())
    {
        ErLogError("Failed to read /proc/{}/env: {}", pid, env_.error().message());
    }
    else
    {
        auto& env = env_.value();
        if (env.raw.empty())
        {
            ErLogInfo("/env: []");
        }
        else
        {
            ErLogIndent(Er::Log::Level::Info, "Environment --------------", pid);
            
            env.split([](const char* str, std::size_t len)
            {
                ErLogInfo("{}", std::string_view(str, len));
            });
        }
    }
}

TEST(ProcFs, enumeratePids)
{
    ProcFs proc;

    EXPECT_GT(proc.bootTime().value(), 0);
    EXPECT_LT(proc.bootTime().value(), Er::Time::now());
    auto tm = proc.bootTime().toLocalTime();
    ErLogInfo("System boot time {:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);

    auto pids_ = proc.enumeratePids();
    EXPECT_TRUE(pids_.has_value());
    auto& pids = pids_.value();
    EXPECT_GT(pids.size(), 1);

    for (auto pid: pids)
    {
        dumpStat(pid, proc);
    }
}