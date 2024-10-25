#include "desktopfilecache.hxx"
#include "iconresolver.hxx"

#include <regex>


namespace Erp::Desktop
{

namespace
{

struct KnownApp
{
    std::regex command;
    std::string icon;
};

const KnownApp g_KnownApps[] =
{
    { std::regex("^((ba|z|tc|c|k)?sh)$"), "utilities-terminal" },
    { std::regex("^chrome$"), "google-chrome" }
};

std::string defaultExeIcon()
{
    return std::string("application-x-executable");
}

std::string knownAppIcon(const std::string& comm)
{
    for (auto& k: g_KnownApps)
    {
        if (regex_match(comm, k.command))
            return k.icon;
    }

    return std::string();
}


} // namespace {}


IconResolver::IconResolver(Er::Log::ILog* log, std::shared_ptr<DesktopFileCache> desktopFileCache)
    : m_log(log)
    , m_desktopFileCache(desktopFileCache)
    , m_procFs("/proc", log)
{
}

std::string IconResolver::lookupIcon(uint64_t pid)
{
    auto comm = m_procFs.readComm(pid);
    if (comm.empty())
    {
        Er::Log::debug(m_log, "No comm for PID {}", pid);
        return defaultExeIcon();
    }

    auto knownIcon = knownAppIcon(comm);
    if (!knownIcon.empty())
    {
        Er::Log::debug(m_log, "Found known icon [{}] for PID {} [{}]", knownIcon, pid, comm);
        return knownIcon;
    }

    auto exec = m_procFs.readExePath(pid);
    if (exec.empty())
    {
        Er::Log::debug(m_log, "No exe path for PID {} [{}]", pid, comm);
        return defaultExeIcon();
    }

    auto desktopFilePath = desktopFileFor(pid, comm, exec);
    if (!desktopFilePath.empty())
    {
        auto desktopEntry = m_desktopFileCache->lookupByPath(desktopFilePath);
        if (desktopEntry && !desktopEntry->icon.empty())
        {
            Er::Log::debug(m_log, "Found icon [{}] for PID {} [{}] [{}]", desktopEntry->icon, pid, comm, exec); 
            return desktopEntry->icon;
        }
    }

    auto desktopEntry = m_desktopFileCache->lookupByExec(exec);
    if (desktopEntry && !desktopEntry->icon.empty())
    {
        Er::Log::debug(m_log, "Found icon [{}] for PID {} [{}] [{}]", desktopEntry->icon, pid, comm, exec); 
        return desktopEntry->icon;
    }

    Er::Log::debug(m_log, "No icon for PID {} [{}] [{}]", pid, comm, exec); 
    return defaultExeIcon();
}

std::string IconResolver::desktopFileFor(uint64_t pid, const std::string& comm, const std::string& exec)
{
    auto uid = m_procFs.getUid(pid);
    if (!uid)
    {
        Er::Log::debug(m_log, "No UID for PID {} [{}] [{}]", pid, comm, exec); 
        return std::string();
    }

    auto environ = m_procFs.readEnviron(pid);
    if (environ.empty())
    {
        Er::Log::warning(m_log, "Empty environment for PID {} [{}] [{}]", pid, comm, exec);
        return std::string();
    }

    if (!haveXdgDataDirsFor(*uid))
    {
        auto it = environ.find("XDG_DATA_DIRS");
        if (it != environ.end() && !it->second.empty())
        {
            addXdgDataDirsFor(*uid, it->second);
        }
        else
        {
            Er::Log::debug(m_log, "No XDG_DATA_DIRS for PID {} [{}] [{}]", pid, comm, exec);
        }
    }

    std::string bamfDesktopFileHint;
    auto it = environ.find("BAMF_DESKTOP_FILE_HINT");
    if (it != environ.end())
    {
        bamfDesktopFileHint = it->second;
        Er::Log::debug(m_log, "BAMF_DESKTOP_FILE_HINT=[{}] for PID {} [{}] [{}]", bamfDesktopFileHint, pid, comm, exec); 
    }

    std::string gioDesktopFile;
    it = environ.find("GIO_LAUNCHED_DESKTOP_FILE");
    if (it != environ.end())
    {
        gioDesktopFile = it->second;
        
        it = environ.find("GIO_LAUNCHED_DESKTOP_FILE_PID");
        if (it != environ.end())
        {
            auto pidStr = it->second;
            auto gioDesktopFilePid = std::strtoull(pidStr.c_str(), nullptr, 10);
            if (gioDesktopFilePid == pid)
            {
                Er::Log::debug(m_log, "GIO_LAUNCHED_DESKTOP_FILE=[{}] for PID {} [{}] [{}]", gioDesktopFile, pid, comm, exec); 
    
                return gioDesktopFile;
            }
        }
    }

    return bamfDesktopFileHint;
}

bool IconResolver::haveXdgDataDirsFor(uint64_t uid) const
{
    std::unique_lock l(m_xdgDataDirsLock);
    auto it = m_xdgDataDirs.find(uid);

    return (it != m_xdgDataDirs.end());
}

void IconResolver::addXdgDataDirsFor(uint64_t uid, const std::string& dirs)
{
    bool updated = false;
    {
        std::unique_lock l(m_xdgDataDirsLock);
        auto r = m_xdgDataDirs.insert({ uid, dirs });
        updated = r.second;
    }

    if (updated)
    {
        m_desktopFileCache->addXdgDataDirs(dirs);
        m_desktopFileCache->waitIdle(std::chrono::milliseconds(1000));

        Er::Log::debug(m_log, "XDG_DATA_DIRS=[{}] for UID {}", dirs, uid);
    }
}


} // namespace Erp::Desktop {}