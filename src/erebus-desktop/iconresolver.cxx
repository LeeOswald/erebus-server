#include "desktopfilecache.hxx"
#include "iconresolver.hxx"

#include <regex>


namespace Er
{

namespace Desktop
{

namespace Private
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
        ErLogDebug(m_log, "No comm for PID %zu", pid);
        return defaultExeIcon();
    }

    auto knownIcon = knownAppIcon(comm);
    if (!knownIcon.empty())
    {
        ErLogDebug(m_log, "Found known icon [%s] for PID %zu [%s]", knownIcon.c_str(), pid, comm.c_str());
        return knownIcon;
    }

    auto exec = m_procFs.readExePath(pid);
    if (exec.empty())
    {
        ErLogDebug(m_log, "No exe path for PID %zu [%s]", pid, comm.c_str());
        return defaultExeIcon();
    }

    auto desktopFilePath = desktopFileFor(pid, comm, exec);
    if (!desktopFilePath.empty())
    {
        auto desktopEntry = m_desktopFileCache->lookupByPath(desktopFilePath);
        if (desktopEntry && !desktopEntry->icon.empty())
        {
            ErLogDebug(m_log, "Found icon [%s] for PID %zu [%s] [%s]", desktopEntry->icon.c_str(), pid,comm.c_str(), exec.c_str()); 
            return desktopEntry->icon;
        }
    }

    auto desktopEntry = m_desktopFileCache->lookupByExec(exec);
    if (desktopEntry && !desktopEntry->icon.empty())
    {
        ErLogDebug(m_log, "Found icon [%s] for PID %zu [%s] [%s]", desktopEntry->icon.c_str(), pid,comm.c_str(), exec.c_str()); 
        return desktopEntry->icon;
    }

    ErLogDebug(m_log, "No icon for PID %zu [%s] [%s]", pid,comm.c_str(), exec.c_str()); 
    return defaultExeIcon();
}

std::string IconResolver::desktopFileFor(uint64_t pid, const std::string& comm, const std::string& exec)
{
    auto uid = m_procFs.getUid(pid);
    if (!uid)
    {
        ErLogDebug(m_log, "No UID for PID %zu [%s] [%s]", pid, comm.c_str(), exec.c_str()); 
        return std::string();
    }

    auto environ = m_procFs.readEnviron(pid);
    if (environ.empty())
    {
        ErLogWarning(m_log, "Empty environment for PID %zu [%s] [%s]", pid, comm.c_str(), exec.c_str());
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
            ErLogDebug(m_log, "No XDG_DATA_DIRS for PID %zu [%s] [%s]", pid, comm.c_str(), exec.c_str());
        }
    }

    std::string bamfDesktopFileHint;
    auto it = environ.find("BAMF_DESKTOP_FILE_HINT");
    if (it != environ.end())
    {
        bamfDesktopFileHint = it->second;
        ErLogDebug(m_log, "BAMF_DESKTOP_FILE_HINT=[%s] for PID %zu [%s] [%s]", bamfDesktopFileHint.c_str(), pid, comm.c_str(), exec.c_str()); 
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
                ErLogDebug(m_log, "GIO_LAUNCHED_DESKTOP_FILE=[%s] for PID %zu [%s] [%s]", gioDesktopFile.c_str(), pid, comm.c_str(), exec.c_str()); 
    
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

        ErLogDebug(m_log, "XDG_DATA_DIRS=[%s] for UID %zu", dirs.c_str(), uid);
    }
}

} // namespace Private {}

} // namespace Desktop {}

} // namespace Er {}