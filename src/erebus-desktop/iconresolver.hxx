#pragma once

#include <erebus-desktop/erebus-desktop.hxx>
#include <erebus/log.hxx>

#include "procfs.hxx"

#include <mutex>
#include <unordered_map>


namespace Erp
{

namespace Desktop
{


class DesktopFileCache;


class IconResolver final
    : public Er::NonCopyable
{
public:
    explicit IconResolver(Er::Log::ILog* log, std::shared_ptr<DesktopFileCache> m_desktopFileCache);

    std::string lookupIcon(uint64_t pid);

private:
    std::string desktopFileFor(uint64_t pid, const std::string& comm, const std::string& exec);
    bool haveXdgDataDirsFor(uint64_t uid) const;
    void addXdgDataDirsFor(uint64_t uid, const std::string& dirs);

    Er::Log::ILog* const m_log;
    std::shared_ptr<DesktopFileCache> m_desktopFileCache;
    ProcFs m_procFs;
    mutable std::mutex m_xdgDataDirsLock;
    std::unordered_map<uint64_t, std::string> m_xdgDataDirs; // UID -> $XDG_DATA_DIRS
};



} // namespace Desktop {}

} // namespace Erp {}