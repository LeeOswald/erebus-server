#include <erebus-processmgr/procfs.hxx>

#include <erebus/exception.hxx>
#include <erebus/util/autoptr.hxx>
#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/posixerror.hxx>

#include <boost/algorithm/string.hpp>

#include <fstream>
#include <vector>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>


namespace Er
{

namespace ProcFs
{

namespace 
{

struct DirCloser
{
    void operator()(DIR* d) noexcept
    {
        ::closedir(d);
    }
};

using DirHolder = Util::AutoPtr<DIR, DirCloser>;

} // namespace {}


ProcFs::ProcFs(Er::Log::ILog* log)
    : m_log(log)
    , m_clkTck(::sysconf(_SC_CLK_TCK))
    , m_cpusMax(::get_nprocs_conf())
{
    ErAssert(m_clkTck > 0);
    ErAssert(m_cpusMax > 0);

    auto rootPath = root();
    if (::access(rootPath.c_str(), R_OK) == -1)
    {   
        auto e = errno;
        throw Er::Exception(ER_HERE(), "Failed to access /proc", Er::ExceptionProps::PosixErrorCode(e), Er::ExceptionProps::DecodedError(Er::Util::posixErrorToString(e)));
    }
}

std::string ProcFs::root()
{
    static std::string s_path("/proc");
    return s_path;
}

Stat ProcFs::readStat(uintptr_t pid) noexcept
{
    ErAssert(pid != KernelPid);

    Stat result;
    result.pid = pid; // Stat::pid is always valid

    try
    {
        auto path = root();
        path.append("/");
        path.append(std::to_string(pid));
        
        struct ::stat64 fileStat;
        if (::stat64(path.c_str(), &fileStat) == -1)
        {
            auto e = errno;
            ErLogDebug(m_log, ErLogNowhere(), "Process %zu not found: %d %s", pid, e, Er::Util::posixErrorToString(e).c_str());
            result.error = "Process not found";
            return result;
        }

        result.ruid = fileStat.st_uid;

        path.append("/stat");

        std::ifstream stat(path);
        if (!stat.good())
        {
            auto e = errno;
            ErLogDebug(m_log, ErLogNowhere(), "Process %zu could not be opened: %d %s", pid, e, Er::Util::posixErrorToString(e).c_str());
            result.error = "Failed to open process";
            return result;
        }

        std::string s;
        std::getline(stat, s);
    
        auto start = s.c_str();
        auto pEnd = start + s.length();
        auto end = start;
        size_t index = 0;
        while (start < pEnd)
        {
            // look for the field end
            while ((end < pEnd) && *end && !std::isspace(*end))
            {
                if (*end == '(')
                {
                    end = std::strrchr(end, ')'); // avoid process names like ":-) 1 2 3"
                    if (!end || !*end)
                    {
                        ErLogDebug(m_log, ErLogNowhere(), "Invalid stat record for process %zu: [%s]", pid, s.c_str());
                        result.error = "Invalid process stat record";
                        return result; 
                    }
                }

                ++end;
            }

            if (end > start)
            {
                char* tmp = nullptr;
                switch (index)
                {
                case 0:
                    result.pid = std::strtol(start, &tmp, 10);
                    break;
                case 1:
                    if (end > start + 3)
                        result.comm.assign(start + 2, end - 1);
                    break;
                case 2:
                    result.state = *(start + 1);
                    break;
                case 3:
                    result.ppid = std::strtol(start + 1, &tmp, 10);
                    break;
                case 4:
                    result.pgrp = std::strtoull(start + 1, &tmp, 10);
                    break;
                case 5:
                    result.session = std::strtoull(start + 1, &tmp, 10);
                    break;
                case 6:
                    result.tty_nr = std::strtol(start + 1, &tmp, 10);
                    break;
                case 7:
                    result.tpgid = std::strtoull(start + 1, &tmp, 10);
                    break;
                case 8:
                    result.flags = (unsigned)std::strtoul(start + 1, &tmp, 10);
                    break;
                case 9:
                    result.minflt = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 10:
                    result.cminflt = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 11:
                    result.majflt = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 12:
                    result.cmajflt = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 13:
                    result.utime = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 14:
                    result.stime = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 15:
                    result.cutime = std::strtol(start + 1, &tmp, 10);
                    break;
                case 16:
                    result.cstime = std::strtol(start + 1, &tmp, 10);
                    break;
                case 17:
                    result.priority = std::strtol(start + 1, &tmp, 10);
                    break;
                case 18:
                    result.nice = std::strtol(start + 1, &tmp, 10);
                    break;
                case 19:
                    result.num_threads = std::strtol(start + 1, &tmp, 10);
                    break;
                case 20:
                    result.itrealvalue = std::strtol(start + 1, &tmp, 10);
                    break;
                case 21:
                    result.starttime = std::strtoull(start + 1, &tmp, 10);
                    break;
                case 22:
                    result.vsize = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 23:
                    result.rss = std::strtol(start + 1, &tmp, 10);
                    break;
                case 24:
                    result.rsslim = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 25:
                    result.startcode = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 26:
                    result.endcode = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 27:
                    result.startstack = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 28:
                    result.kstkesp = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 29:
                    result.kstkeip = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 30:
                    result.signal = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 31:
                    result.blocked = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 32:
                    result.sigignore = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 33:
                    result.sigcatch = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 34:
                    result.wchan = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 35:
                    result.nswap = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 36:
                    result.cnswap = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 37:
                    result.exit_signal = std::strtol(start + 1, &tmp, 10);
                    break;
                case 38:
                    result.processor = std::strtol(start + 1, &tmp, 10);
                    break;
                case 39:
                    result.rt_priority = (unsigned)std::strtoul(start + 1, &tmp, 10);
                    break;
                case 40:
                    result.policy = (unsigned)std::strtoul(start + 1, &tmp, 10);
                    break;
                case 41:
                    result.delayacct_blkio_ticks = std::strtoull(start + 1, &tmp, 10);
                    break;
                case 42:
                    result.guest_time = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 43:
                    result.cguest_time = std::strtol(start + 1, &tmp, 10);
                    break;
                case 44:
                    result.start_data = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 45:
                    result.end_data = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 46:
                    result.start_brk = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 47:
                    result.arg_start = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 48:
                    result.arg_end = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 49:
                    result.env_start = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 50:
                    result.env_end = std::strtoul(start + 1, &tmp, 10);
                    break;
                case 51:
                    result.exit_code = std::strtol(start + 1, &tmp, 10);
                    break;
                }
            }

            ++index;
            start = end;
            ++end;
        }

        result.startTime = fromRelativeTime(result.starttime);
        result.uTime = double(result.utime) / m_clkTck;
        result.sTime = double(result.stime) / m_clkTck;

        result.valid = true;
    }
    catch (std::exception& e)
    {
        ErLogDebug(m_log, ErLogNowhere(), "stat for process %zu could not be read: %s", pid, e.what());
        result.error = e.what();
    }

    return result;
}

std::string ProcFs::readComm(uintptr_t pid) noexcept
{
    if (pid == KernelPid)
        return std::string();

    try
    {
        auto path = root();
        path.append("/");
        path.append(std::to_string(pid));
        path.append("/comm");
    
        std::ifstream file(path.c_str(), std::ifstream::in);
        std::string comm;
        if (std::getline(file, comm))
            return comm;
    }
    catch (std::exception& e)
    {
        ErLogDebug(m_log, ErLogNowhere(), "comm for process %zu could not be read: %s", pid, e.what());
    }

    return std::string();
}

std::string ProcFs::readExePath(uintptr_t pid) noexcept
{
    if (pid == KernelPid || pid == KThreadDPid)
        return std::string();

    try
    {
        auto path = root();
        path.append("/");
        path.append(std::to_string(pid));
        path.append("/exe");
    
        struct stat sb = { 0 };
        if (::lstat(path.c_str(), &sb) == -1)
        {
            auto e = errno;
            ErLogDebug(m_log, ErLogNowhere(), "exe link for process %zu could not be opened: %d %s", pid, e, Er::Util::posixErrorToString(e).c_str());
            return std::string();
        }
        else
        {
            size_t size = sb.st_size;
            if (size == 0) // lstat can yield sb.st_size = 0
                size = PATH_MAX;

            std::string exe;
            exe.resize(size + 1, '\0');
            auto r = ::readlink(path.c_str(), exe.data(), size); // readlink does not append '\0'
            if (r < 0)
            {
                auto e = errno;
                ErLogDebug(m_log, ErLogNowhere(), "Failed to read exe link for process %zu: %d %s", pid, e, Er::Util::posixErrorToString(e).c_str());
                return std::string();
            }

            exe.resize(std::strlen(exe.c_str())); // cut extra '\0'

            return exe;
        }
    }
    catch (std::exception& e)
    {
        ErLogDebug(m_log, ErLogNowhere(), "exe link for process %zu could not be read: %s", pid, e.what());
    }

    return std::string();
}

std::string ProcFs::readCmdLine(uintptr_t pid) noexcept
{
    try
    {
        auto path = root();
        if (pid != KernelPid)
        {
            path.append("/");
            path.append(std::to_string(pid));
        }

        path.append("/cmdline");

        std::ifstream stream(path);
        if (!stream.good())
        {
            ErLogDebug(m_log, ErLogNowhere(), "Failed to open cmdline for process %zu", pid);
        }
        else
        {
            std::string cmdLine;
            std::string a;
            while (std::getline(stream, a, '\0'))
            {
                if (!a.empty())
                {
                    if (!cmdLine.empty())
                        cmdLine.append(" ");

                    cmdLine.append(std::move(a));
                }
            }

            // trim right
            while (cmdLine.size() && std::isspace(cmdLine[cmdLine.size() - 1]))
            {
                cmdLine.erase(cmdLine.size() - 1);
            }

            return cmdLine;
        }
    }
    catch (std::exception& e)
    {
        ErLogDebug(m_log, ErLogNowhere(), "cmdline for process %zu could not be read: %s", pid, e.what());
    }

    return std::string();
}

std::vector<uintptr_t> ProcFs::enumeratePids() noexcept
{
    std::vector<uintptr_t> result;
    auto reserve = m_pidCountMax;
    if (!reserve)
        reserve = 512;
    result.reserve(reserve);

    try
    {
        auto path = root();
        DirHolder dir(::opendir(path.c_str()));
        if (!dir)
        {
            auto e = errno;
            throw Er::Exception(ER_HERE(), "Failed to open /proc", Er::ExceptionProps::PosixErrorCode(e), Er::ExceptionProps::DecodedError(Er::Util::posixErrorToString(e)));
        }
    
        for (auto ent = ::readdir(dir); ent != nullptr; ent = ::readdir(dir))
        {
            if (!std::isdigit(ent->d_name[0]))
                continue;

            uintptr_t pid = InvalidPid;
            try
            {
                pid = std::stoul(ent->d_name);
            }
            catch (std::exception& e)
            {
                ErLogDebug(m_log, ErLogNowhere(), "Failed to parse PID %s: %s", ent->d_name, e.what());
                continue;
            }

            result.push_back(pid);
        }
    }
    catch (Er::Exception& e)
    {
        Er::Util::logException(m_log, Er::Log::Level::Error, e);
    }
    catch (std::exception& e)
    {
        Er::Util::logException(m_log, Er::Log::Level::Error, e);
    }

    if (result.size() > reserve)
        m_pidCountMax = result.size();

    return result;
}

uint64_t ProcFs::getBootTimeImpl() noexcept
{
    std::string path = root();
    path.append("/stat");

    std::ifstream stream(path);
    if (!stream.good())
    {
        ErLogError(m_log, ErLogNowhere(), "Failed to open %s", path.c_str());
        return 0;
    }
    else
    {
        std::string s;
        while (std::getline(stream, s))
        {
            if (s.find("btime") == 0)
            {
                auto remainder = s.substr(6);
                if (!remainder.empty())
                {
                    char* end = nullptr;
                    return std::strtoull(remainder.c_str(), &end, 10);
                }

                break;
            }
        }
    }

    return 0;
}

uint64_t ProcFs::bootTime() noexcept
{
    static uint64_t bootTime = getBootTimeImpl();
    return bootTime;
}

uint64_t ProcFs::fromRelativeTime(uint64_t relative) noexcept
{
    auto boot = bootTime();
    auto time = relative / m_clkTck;

    return boot + time;
}

CpuTimesAll ProcFs::readCpuTimes() noexcept
{
    CpuTimesAll all;

    try
    {
        std::string path = root();
        path.append("/stat");

        std::ifstream stream(path);
        if (!stream.good())
        {
            ErLogError(m_log, ErLogNowhere(), "Failed to open %s", path.c_str());
            return all;
        }

        all.cores.reserve(m_cpusMax);

        auto lineParser = [this](const std::string& line, CpuTimes& t)
        {
            std::vector<std::string> parts;
            parts.reserve(13);
            boost::split(parts, line, [](char c) { return (c == ' '); });
    
            if (parts.size() > 1)
                t.user = ::strtoull(parts[1].c_str(), nullptr, 10) / double(m_clkTck);

            if (parts.size() > 2)
                t.user_nice = ::strtoull(parts[2].c_str(), nullptr, 10) / double(m_clkTck);

            if (parts.size() > 3)
                t.system = ::strtoull(parts[3].c_str(), nullptr, 10) / double(m_clkTck);

            if (parts.size() > 4)
                t.idle = ::strtoull(parts[4].c_str(), nullptr, 10) / double(m_clkTck);

            if (parts.size() > 5)
                t.iowait = ::strtoull(parts[5].c_str(), nullptr, 10) / double(m_clkTck);

            if (parts.size() > 6)
                t.irq = ::strtoull(parts[6].c_str(), nullptr, 10) / double(m_clkTck);

            if (parts.size() > 7)
                t.softirq = ::strtoull(parts[7].c_str(), nullptr, 10) / double(m_clkTck);

            if (parts.size() > 8)
                t.steal = ::strtoull(parts[8].c_str(), nullptr, 10) / double(m_clkTck);

            if (parts.size() > 9)
                t.guest = ::strtoull(parts[9].c_str(), nullptr, 10) / double(m_clkTck);

            if (parts.size() > 10)
                t.guest_nice = ::strtoull(parts[10].c_str(), nullptr, 10) / double(m_clkTck);

        };

        std::string line;
        int cpuno = -1;
        char cpuname[16];
        
        while (std::getline(stream, line))
        {
            if ((cpuno == -1) && (line.find("cpu ") == 0))
            {
                lineParser(line, all.all);

                cpuno = 0;
                std::snprintf(cpuname, _countof(cpuname), "cpu%d", cpuno);
            }
            else if ((cpuno >= 0) && (line.find(cpuname) == 0))
            {
                CpuTimes times;
                lineParser(line, times);
                all.cores.push_back(times);

                ++cpuno;
                std::snprintf(cpuname, _countof(cpuname), "cpu%d", cpuno);
            }
        }

    }
    catch (Er::Exception& e)
    {
        Er::Util::logException(m_log, Er::Log::Level::Error, e);
    } 
    catch (std::exception& e)
    {
        Er::Util::logException(m_log, Er::Log::Level::Error, e);
    }

    return all;
}

MemStats ProcFs::readMemStats() noexcept
{
    MemStats mem;

    auto getUnit = [](const std::string& unit) -> std::size_t
    {
        if (unit.length() < 2) [[unlikely]]
            return 1;
        else if ((unit[0] == 'k') && (unit[1] == 'B')) [[likely]]
            return 1024;
        else if (((unit[0] == 'm') || (unit[0] == 'M')) && (unit[1] == 'B'))
            return 1024 * 1024;
        else if (((unit[0] == 'g') || (unit[0] == 'G')) && (unit[1] == 'B'))
            return 1024 * 1024 * 1024;

        return 1;
    };

    try
    {
        std::string path = root();
        path.append("/meminfo");

        std::ifstream stream(path);
        if (!stream.good())
        {
            ErLogError(m_log, ErLogNowhere(), "Failed to open %s", path.c_str());
            return mem;
        }

        uint64_t availableMem = 0;
        uint64_t freeMem = 0;
        uint64_t totalMem = 0;
        uint64_t buffersMem = 0;
        uint64_t cachedMem = 0;
        uint64_t sharedMem = 0;
        uint64_t swapTotalMem = 0;
        uint64_t swapCachedMem = 0;
        uint64_t swapFreeMem = 0;
        uint64_t reclaimable = 0;
        uint64_t zswapCompMem = 0;
        uint64_t zswapOrigMem = 0;

        std::string line;
        while (std::getline(stream, line))
        {
            std::vector<std::string> parts;
            parts.reserve(3);
            boost::split(parts, line, [](char c) { return (c == ' '); });
            if (parts.size() < 3)
                continue;

            auto unit = getUnit(parts[2]);

            if (parts[0].length() < 1)
                continue;

            if (parts[0][0] == 'M')
            {
                if (parts[0] == std::string_view("MemAvailable:"))
                    availableMem = unit * std::strtoull(parts[1].c_str(), nullptr, 10);
                else if (parts[0] == std::string_view("MemFree:"))
                    freeMem = unit * std::strtoull(parts[1].c_str(), nullptr, 10);
                else if (parts[0] == std::string_view("MemTotal:"))
                    totalMem = unit * std::strtoull(parts[1].c_str(), nullptr, 10);
            }
            else if (parts[0][0] == 'B')
            {
                if (parts[0] == std::string_view("Buffers:"))
                    buffersMem = unit * std::strtoull(parts[1].c_str(), nullptr, 10);
            }
            else if (parts[0][0] == 'C')
            {
                if (parts[0] == std::string_view("Cached:"))
                    cachedMem = unit * std::strtoull(parts[1].c_str(), nullptr, 10);
            }
            else if (parts[0][0] == 'S')
            {
                if (parts[0] == std::string_view("Shmem:"))
                    sharedMem = unit * std::strtoull(parts[1].c_str(), nullptr, 10);
                else if (parts[0] == std::string_view("SwapTotal:"))
                    swapTotalMem = unit * std::strtoull(parts[1].c_str(), nullptr, 10);
                else if (parts[0] == std::string_view("SwapCached:"))
                    swapCachedMem = unit * std::strtoull(parts[1].c_str(), nullptr, 10);
                else if (parts[0] == std::string_view("SwapFree:"))
                    swapFreeMem = unit * std::strtoull(parts[1].c_str(), nullptr, 10);
                else if (parts[0] == std::string_view("SReclaimable:"))
                    reclaimable = unit * std::strtoull(parts[1].c_str(), nullptr, 10);
            }
            else if (parts[0][0] == 'Z')
            {
                if (parts[0] == std::string_view("Zswap:"))
                    zswapOrigMem = unit * std::strtoull(parts[1].c_str(), nullptr, 10);
                else if (parts[0] == std::string_view("Zswapped:"))
                    zswapCompMem = unit * std::strtoull(parts[1].c_str(), nullptr, 10);
            }
        }

        mem.totalMem = totalMem;
        mem.cachedMem = cachedMem + reclaimable - sharedMem;
        mem.sharedMem = sharedMem;
        auto usedDiff = freeMem + cachedMem + reclaimable + buffersMem;
        mem.usedMem = (totalMem >= usedDiff) ? totalMem - usedDiff : totalMem - freeMem;
        mem.buffersMem = buffersMem;
        mem.availableMem = availableMem != 0 ? std::min(availableMem, totalMem) : freeMem;
        mem.totalSwap = swapTotalMem;
        mem.usedSwap = swapTotalMem - swapFreeMem - swapCachedMem;
        mem.cachedSwap = swapCachedMem;
        mem.zswapComp = zswapCompMem;
        mem.zswapOrig = zswapOrigMem;
    }
    catch (Er::Exception& e)
    {
        Er::Util::logException(m_log, Er::Log::Level::Error, e);
    } 
    catch (std::exception& e)
    {
        Er::Util::logException(m_log, Er::Log::Level::Error, e);
    }

    return mem;
}

    
} // namespace ProcFs {}

} // namespace Er {}