#include <erebus/proctree/linux/procfs.hxx>
#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/format.hxx>
#include <erebus/rtl/system/posix_error.hxx>
#include <erebus/rtl/util/file.hxx>

#include <sstream>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>

namespace Er::ProcessTree::Util
{

ProcFs::ProcFs(std::string_view procFsRoot)
    : m_procFsRoot(procFsRoot)
    , m_bootTime(getBootTimeImpl())
    , m_clkTck(::sysconf(_SC_CLK_TCK))
    , m_cpusMax(::get_nprocs_conf())
{
    ErAssert(m_clkTck > 0);
    ErAssert(m_cpusMax > 0);

    if (::access(m_procFsRoot.c_str(), R_OK) == -1)
    {
        auto err = errno;
        ErThrowPosixError(Er::format("Failed to access {}", m_procFsRoot), err);
    }
}

std::expected<ProcFs::Stat, int> ProcFs::readStat(Pid pid) noexcept
{
    ErAssert(pid != KernelPid);

    Stat result;
    result.pid = pid; // Stat::pid is always valid

    try
    {
        auto path = m_procFsRoot;
        path.append("/");
        path.append(std::to_string(pid));
        
        struct ::stat64 fileStat;
        if (::stat64(path.c_str(), &fileStat) == -1)
        {
            return std::unexpected(errno);
        }

        result.ruid = fileStat.st_uid;

        path.append("/stat");

        auto loaded = Er::Util::tryLoadFile(path);
        if (!loaded.has_value())
        {
            return std::unexpected(loaded.error());
        }

        auto& s = loaded.value().bytes();
    
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
                        return std::unexpected(EINVAL);
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
    }
    catch (std::exception& e)
    {
        return std::unexpected(EINVAL);
    }

    return result;
}

std::uint64_t ProcFs::getBootTimeImpl()
{
    std::string path = m_procFsRoot;
    path.append("/stat");

    auto buffer = Er::Util::loadFile(path);

    std::istringstream ss(buffer.bytes());
    std::string s;
    while (std::getline(ss, s))
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

    ErThrow(Er::format("No \'btime\' field in {}", path));
    return 0;
}

std::uint64_t ProcFs::fromRelativeTime(std::uint64_t relative) noexcept
{
    auto boot = m_bootTime;
    auto time = relative / m_clkTck;

    return boot + time;
}

std::expected<std::string, int> ProcFs::readComm(Pid pid) noexcept
{
    return std::unexpected(-1);
}

} // namespace Er::ProcessTree::Util {}
