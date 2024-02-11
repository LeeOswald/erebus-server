#include <erebus-processmgr/procfs.hxx>

#include <erebus/exception.hxx>
#include <erebus/util/autoptr.hxx>
#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/posixerror.hxx>


#include <fstream>

#include <dirent.h>
#include <sys/stat.h>


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
{
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
            LogDebug(m_log, "Process %zu not found: %d", pid, errno);
            result.error = "Process not found";
            return result;
        }

        result.ruid = fileStat.st_uid;

        path.append("/stat");

        std::ifstream stat(path);
        if (!stat.good())
        {
            LogDebug(m_log, "Process %zu could not be opened: %d", pid, errno);
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
                        LogDebug(m_log, "Invalid stat record for process %zu: [%s]", pid, s.c_str());
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
                    result.pgrp = std::strtod(start + 1, &tmp);
                    break;
                case 5:
                    result.session = std::strtod(start + 1, &tmp);
                    break;
                case 6:
                    result.tty_nr = std::strtod(start + 1, &tmp);
                    break;
                case 7:
                    result.tpgid = std::strtod(start + 1, &tmp);
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
                    result.exit_signal = std::strtod(start + 1, &tmp);
                    break;
                case 38:
                    result.processor = std::strtod(start + 1, &tmp);
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
                    result.exit_code = std::strtod(start + 1, &tmp);
                    break;
                }
            }

            ++index;
            start = end;
            ++end;
        }

        result.startTime = fromRelativeTime(result.starttime);

        result.valid = true;
    }
    catch (std::exception& e)
    {
        LogDebug(m_log, "stat for process %zu could not be read: %s", pid, e.what());
        result.error = e.what();
    }

    return result;
}

std::string ProcFs::readComm(uintptr_t pid) noexcept
{
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
        LogDebug(m_log, "comm for process %zu could not be read: %s", pid, e.what());
    }

    return std::string();
}

std::string ProcFs::readExePath(uintptr_t pid) noexcept
{
    try
    {
        auto path = root();
        path.append("/");
        path.append(std::to_string(pid));
        path.append("/exe");
    
        struct stat sb = { 0 };
        if (::lstat(path.c_str(), &sb) == -1)
        {
            LogDebug(m_log, "exe link for process %zu could not be opened: %d", pid, errno);
            return std::string();
        }
        else
        {
            size_t size = sb.st_size;
            if (size == 0) // lstat can yield sb.st_size = 0
            {
#ifdef PATH_MAX
                size = PATH_MAX;
#else
                size = 4096;
#endif
            }

            std::string exe;
            exe.resize(size + 1, '\0');
            auto r = ::readlink(path.c_str(), exe.data(), size); // readlink does not append '\0'
            if (r < 0)
            {
                LogDebug(m_log, "Failed to read exe link for process %zu: %d", pid, errno);
                return std::string();
            }

            exe.resize(std::strlen(exe.c_str())); // cut extra '\0'

            return exe;
        }
    }
    catch (std::exception& e)
    {
        LogDebug(m_log, "exe link for process %zu could not be read: %s", pid, e.what());
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
            LogDebug(m_log, "Failed to open cmdline for process %zu", pid);
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

            return cmdLine;
        }
    }
    catch (std::exception& e)
    {
        LogDebug(m_log, "cmdline for process %zu could not be read: %s", pid, e.what());
    }

    return std::string();
}

std::vector<uintptr_t> ProcFs::enumeratePids() noexcept
{
    std::vector<uintptr_t> result;

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
                LogDebug(m_log, "Failed to parse PID %s: %s", ent->d_name, e.what());
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

    return result;
}

uint64_t ProcFs::getBootTimeImpl() noexcept
{
    std::string path = root();
    path.append("/stat");

    std::ifstream stream(path);
    if (!stream.good())
    {
        LogError(m_log, "Failed to open %s", path.c_str());
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

uint64_t ProcFs::getBootTime() noexcept
{
    static uint64_t bootTime = getBootTimeImpl();
    return bootTime;
}

uint64_t ProcFs::fromRelativeTime(uint64_t relative) noexcept
{
    static long clockRes = ::sysconf(_SC_CLK_TCK);
    assert(clockRes > 0);

    auto bootTime = getBootTime();
    auto time = relative / clockRes;

    return bootTime + time;
}

    
} // namespace ProcFs {}

} // namespace Er {}