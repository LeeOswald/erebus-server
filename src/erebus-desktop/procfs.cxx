#include "procfs.hxx"

#include <erebus/exception.hxx>
#include <erebus/util/stringutil.hxx>

#include <fstream>
#include <vector>

namespace Erp::Desktop
{


ProcFs::ProcFs(const std::string& root, Er::Log::ILog* const log)
    : m_root(root)
    , m_log(log)
{
    if (::access(root.c_str(), R_OK) == -1)
        ErThrowPosixError("Failed to access /proc", errno);
}

std::optional<uint64_t> ProcFs::getUid(uint64_t pid) const noexcept
{
    auto path = m_root;
    path.append("/");
    path.append(std::to_string(pid));
    
    struct ::stat64 fileStat;
    if (::stat64(path.c_str(), &fileStat) == -1)
    {
        return std::nullopt;
    }

    return fileStat.st_uid;
}

std::string ProcFs::readComm(uintptr_t pid) const noexcept
{
    try
    {
        auto path = m_root;
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
        Er::Log::debug(m_log, "comm for process {} could not be read: {}", pid, e.what());
    }

    return std::string();
}

std::string ProcFs::readExePath(uintptr_t pid) const noexcept
{
    try
    {
        auto path = m_root;
        path.append("/");
        path.append(std::to_string(pid));
        path.append("/exe");
    
        struct stat sb = { 0 };
        if (::lstat(path.c_str(), &sb) == -1)
        {
            auto e = errno;
            Er::Log::debug(m_log, "exe link for process {} could not be opened: {} {}", pid, e, Er::Util::posixErrorToString(e));
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
                Er::Log::debug(m_log, "Failed to read exe link for process {}: {} {}", pid, e, Er::Util::posixErrorToString(e));
                return std::string();
            }

            exe.resize(std::strlen(exe.c_str())); // cut extra '\0'

            return exe;
        }
    }
    catch (std::exception& e)
    {
        Er::Log::debug(m_log, "exe link for process {} could not be read: {}", pid, e.what());
    }

    return std::string();
}

std::unordered_map<std::string, std::string> ProcFs::readEnviron(uint64_t pid) const noexcept
{
    std::unordered_map<std::string, std::string> env;

    try
    {
        auto path = m_root;
        path.append("/");
        path.append(std::to_string(pid));
        path.append("/environ");

        std::ifstream file;
        file.exceptions(std::ifstream::badbit);
        file.open(path.c_str(), std::ifstream::in);

        std::string line;
        std::vector<std::string> v;
        v.reserve(2);

        while (std::getline(file, line, '\0'))
        {
            v.clear();
            Er::Util::split2(
                line, 
                std::string_view("="), 
                Er::Util::SplitKeepEmptyParts, 
                v
            );

            if (v.size() != 2)
                continue;

            env.insert({ std::move(v[0]), std::move(v[1]) });
        }
    }
    catch (std::exception& e)
    {
        Er::Log::debug(m_log, "environ for process {} could not be read: {}", pid, e.what());
    }

    return env;
}



} // namespace Erp::Desktop {}
