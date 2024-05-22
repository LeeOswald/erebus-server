#pragma once

#include <erebus/log.hxx>
#include <erebus/system/pathresolver.hxx>

#include <erebus-processmgr/processmgr.hxx>

#include <shared_mutex>
#include <unordered_map>
#include <vector>


namespace Er
{

namespace Private
{


class DesktopEntries final
    : public Er::NonCopyable
{
public:
    struct Entry
    {
        std::string name;
        std::string exec;
        std::string icon;
    };

    explicit DesktopEntries(Er::Log::ILog* log);

    std::shared_ptr<Entry> lookup(const std::string& exec) const noexcept;
    std::vector<std::string> iconList() const;

private:
    void addXdgDataDirs();
    void addUserDirs();
    void parseFiles(const std::string& dir);
    std::shared_ptr<Entry> parseFile(const std::string& path);
    std::optional<std::string> resolveExePath(std::string_view exe) const;

    Er::Log::ILog* const m_log;
    Er::System::PathResolver m_pathResolver;
    mutable std::shared_mutex m_dirsLock;
    std::vector<std::string> m_dirs;
    mutable std::shared_mutex m_entriesLock;
    std::unordered_map<std::string, std::shared_ptr<Entry>> m_entries; // exe -> Entry
};


} // Private {}

} // namespace Er {}
