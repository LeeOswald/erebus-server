#include "procfs.hxx"

#include <erebus/exception.hxx>
#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/posixerror.hxx>
#include <erebus/util/stringutil.hxx>

#include <fstream>
#include <vector>

namespace Er
{

namespace Desktop
{

namespace Private
{

ProcFs::ProcFs(const std::string& root, Er::Log::ILog* const log)
    : m_root(root)
    , m_log(log)
{
    if (::access(root.c_str(), R_OK) == -1)
    {   
        auto e = errno;
        throw Er::Exception(ER_HERE(), "Failed to access /proc", Er::ExceptionProps::PosixErrorCode(e), Er::ExceptionProps::DecodedError(Er::Util::posixErrorToString(e)));
    }
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
        ErLogDebug(m_log, ErLogNowhere(), "comm for process %zu could not be read: %s", pid, e.what());
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
        ErLogDebug(m_log, ErLogNowhere(), "environ for process %zu could not be read: %s", pid, e.what());
    }

    return env;
}


} // namespace Private {}

} // namespace Desktop {}

} // namespace Er {}
