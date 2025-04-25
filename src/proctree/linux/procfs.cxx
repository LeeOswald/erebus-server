#include <erebus/proctree/linux/procfs.hxx>
#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/format.hxx>
#include <erebus/rtl/system/posix_error.hxx>
#include <erebus/rtl/util/file.hxx>
#include <erebus/rtl/util/auto_ptr.hxx>

#include <fstream>
#include <sstream>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>

namespace Er::ProcessTree::Linux
{

namespace 
{

using DirHolder = Er::Util::AutoPtr<DIR, decltype([](DIR* d) { ::closedir(d); })>;

} // namespace {}


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

std::expected<std::vector<Pid>, int> ProcFs::enumeratePids()
{
    std::vector<Pid> result;

    auto reserve = m_pidCountMax;
    if (!reserve)
        reserve = 512;
    result.reserve(reserve);

    DirHolder dir(::opendir(m_procFsRoot.c_str()));
    if (!dir)
    {
        return std::unexpected(errno);
    }

    for (auto ent = ::readdir(dir); ent != nullptr; ent = ::readdir(dir))
    {
        if (!std::isdigit(ent->d_name[0]))
            continue;

        Pid pid = std::strtoull(ent->d_name, nullptr, 10);
        result.push_back(pid);
    }

    if (result.size() > reserve)
        m_pidCountMax = result.size();

    return {std::move(result)};
}

std::expected<ProcFs::Stat, int> ProcFs::readStat(Pid pid)
{
    ErAssert(pid != KernelPid);

    Stat result;
    result.pid = pid; // Stat::pid is always valid

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

    std::ifstream stat(path);
    if (!stat.good())
    {
        return std::unexpected(errno);
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
                    return std::unexpected(EINVAL);
                }
            }

            ++end;
        }

        if (end > start)
        {
            switch (index)
            {
            case 0:
                result.pid = std::strtoll(start, nullptr, 10);
                break;
            case 1:
                if (end > start + 3)
                    result.comm.assign(start + 2, end - 1);
                break;
            case 2:
                result.state = *(start + 1);
                break;
            case 3:
                result.ppid = std::strtoll(start + 1, nullptr, 10);
                break;
            case 4:
                result.pgrp = std::strtoll(start + 1, nullptr, 10);
                break;
            case 5:
                result.session = std::strtoll(start + 1, nullptr, 10);
                break;
            case 6:
                result.tty_nr = std::strtol(start + 1, nullptr, 10);
                break;
            case 7:
                result.tpgid = std::strtoll(start + 1, nullptr, 10);
                break;
            case 8:
                result.flags = (unsigned)std::strtoul(start + 1, nullptr, 10);
                break;
            case 9:
                result.minflt = std::strtoul(start + 1, nullptr, 10);
                break;
            case 10:
                result.cminflt = std::strtoul(start + 1, nullptr, 10);
                break;
            case 11:
                result.majflt = std::strtoul(start + 1, nullptr, 10);
                break;
            case 12:
                result.cmajflt = std::strtoul(start + 1, nullptr, 10);
                break;
            case 13:
                result.utime = std::strtoul(start + 1, nullptr, 10);
                break;
            case 14:
                result.stime = std::strtoul(start + 1, nullptr, 10);
                break;
            case 15:
                result.cutime = std::strtol(start + 1, nullptr, 10);
                break;
            case 16:
                result.cstime = std::strtol(start + 1, nullptr, 10);
                break;
            case 17:
                result.priority = std::strtol(start + 1, nullptr, 10);
                break;
            case 18:
                result.nice = std::strtol(start + 1, nullptr, 10);
                break;
            case 19:
                result.num_threads = std::strtol(start + 1, nullptr, 10);
                break;
            case 20:
                result.itrealvalue = std::strtol(start + 1, nullptr, 10);
                break;
            case 21:
                result.starttime = std::strtoull(start + 1, nullptr, 10);
                break;
            case 22:
                result.vsize = std::strtoul(start + 1, nullptr, 10);
                break;
            case 23:
                result.rss = std::strtol(start + 1, nullptr, 10);
                break;
            case 24:
                result.rsslim = std::strtoul(start + 1, nullptr, 10);
                break;
            case 25:
                result.startcode = std::strtoul(start + 1, nullptr, 10);
                break;
            case 26:
                result.endcode = std::strtoul(start + 1, nullptr, 10);
                break;
            case 27:
                result.startstack = std::strtoul(start + 1, nullptr, 10);
                break;
            case 28:
                result.kstkesp = std::strtoul(start + 1, nullptr, 10);
                break;
            case 29:
                result.kstkeip = std::strtoul(start + 1, nullptr, 10);
                break;
            case 30:
                result.signal = std::strtoul(start + 1, nullptr, 10);
                break;
            case 31:
                result.blocked = std::strtoul(start + 1, nullptr, 10);
                break;
            case 32:
                result.sigignore = std::strtoul(start + 1, nullptr, 10);
                break;
            case 33:
                result.sigcatch = std::strtoul(start + 1, nullptr, 10);
                break;
            case 34:
                result.wchan = std::strtoul(start + 1, nullptr, 10);
                break;
            case 35:
                result.nswap = std::strtoul(start + 1, nullptr, 10);
                break;
            case 36:
                result.cnswap = std::strtoul(start + 1, nullptr, 10);
                break;
            case 37:
                result.exit_signal = std::strtol(start + 1, nullptr, 10);
                break;
            case 38:
                result.processor = std::strtol(start + 1, nullptr, 10);
                break;
            case 39:
                result.rt_priority = (unsigned)std::strtoul(start + 1, nullptr, 10);
                break;
            case 40:
                result.policy = (unsigned)std::strtoul(start + 1, nullptr, 10);
                break;
            case 41:
                result.delayacct_blkio_ticks = std::strtoull(start + 1, nullptr, 10);
                break;
            case 42:
                result.guest_time = std::strtoul(start + 1, nullptr, 10);
                break;
            case 43:
                result.cguest_time = std::strtol(start + 1, nullptr, 10);
                break;
            case 44:
                result.start_data = std::strtoul(start + 1, nullptr, 10);
                break;
            case 45:
                result.end_data = std::strtoul(start + 1, nullptr, 10);
                break;
            case 46:
                result.start_brk = std::strtoul(start + 1, nullptr, 10);
                break;
            case 47:
                result.arg_start = std::strtoul(start + 1, nullptr, 10);
                break;
            case 48:
                result.arg_end = std::strtoul(start + 1, nullptr, 10);
                break;
            case 49:
                result.env_start = std::strtoul(start + 1, nullptr, 10);
                break;
            case 50:
                result.env_end = std::strtoul(start + 1, nullptr, 10);
                break;
            case 51:
                result.exit_code = std::strtol(start + 1, nullptr, 10);
                break;
            }
        }

        ++index;
        start = end;
        ++end;
    }

    result.startTime = System::PackedTime::fromPosixTime(fromRelativeTime(result.starttime));
    result.uTime = double(result.utime) / m_clkTck;
    result.sTime = double(result.stime) / m_clkTck;

    return {std::move(result)};
}

std::uint64_t ProcFs::getBootTimeImpl()
{
    auto path = m_procFsRoot;
    path.append("/stat");

    std::ifstream stream(path);
    if (!stream.good())
    {
        ErThrow(Er::format("Failed to open {}", path));
    }

    std::string s;
    while (std::getline(stream, s))
    {
        if (s.find("btime") == 0)
        {
            auto remainder = s.substr(6);
            if (!remainder.empty())
            {
                return std::strtoull(remainder.c_str(), nullptr, 10);
            }

            break;
        }
    }

    ErThrow(Er::format("No \'btime\' field in {}", path));
    return 0;
}

std::expected<std::string, int> ProcFs::readComm(Pid pid)
{
    if (pid == KernelPid)
        return std::string();

    auto path = m_procFsRoot;
    path.append("/");
    path.append(std::to_string(pid));
    path.append("/comm");

    auto loaded = Er::Util::tryLoadFile(path);
    if (!loaded.has_value())
    {
        return std::unexpected(loaded.error());
    }

    return loaded.value().release();
}

std::expected<std::string, int> ProcFs::readExePath(Pid pid)
{
    if (pid == KernelPid || pid == KThreadDPid)
        return std::string();

    auto path = m_procFsRoot;
    path.append("/");
    path.append(std::to_string(pid));
    path.append("/exe");

    struct stat sb = { 0 };
    if (::lstat(path.c_str(), &sb) == -1)
    {
        return std::unexpected(errno);
    }

    size_t size = sb.st_size;
    if (size == 0) // lstat can yield sb.st_size = 0
        size = PATH_MAX;

    std::string exe;
    exe.resize(size + 1, '\0');
    auto r = ::readlink(path.c_str(), exe.data(), size); // readlink does not append '\0'
    if (r < 0)
    {
        return std::unexpected(errno);
    }

    exe.resize(std::strlen(exe.c_str())); // cut extra '\0'

    return {std::move(exe)};
}

std::expected<MultiStringZ, int> ProcFs::readCmdLine(Pid pid)
{
    auto path = m_procFsRoot;
    if (pid != KernelPid)
    {
        path.append("/");
        path.append(std::to_string(pid));
    }

    path.append("/cmdline");

    auto loaded = Er::Util::tryLoadFile(path);
    if (!loaded.has_value())
    {
        return std::unexpected(loaded.error());
    }

    return loaded.value().release();
}

std::expected<MultiStringZ, int> ProcFs::readEnv(Pid pid)
{
    if (pid == KernelPid)
        return MultiStringZ{};

    auto path = m_procFsRoot;
    path.append("/");
    path.append(std::to_string(pid));
    path.append("/environ");

    auto loaded = Er::Util::tryLoadFile(path);
    if (!loaded.has_value())
    {
        return std::unexpected(loaded.error());
    }

    return loaded.value().release();
}

} // namespace Er::ProcessTree::Linux {}
