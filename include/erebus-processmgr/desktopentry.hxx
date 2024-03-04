#pragma once

#include <erebus/log.hxx>
#include <erebus-processmgr/processmgr.hxx>


#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>


namespace Er
{

namespace DesktopEnv
{


class ER_PROCESSMGR_EXPORT DesktopEntries final
    : public Er::NonCopyable
{
public:
    explicit DesktopEntries(Er::Log::ILog* log);

private:
    struct Entry
    {
        std::string name;
        std::string exec;
        std::string icon;
    };

    void addXdgDataDirs();
    void addUserDirs();
    void enumerateFiles(const std::string& dir);
    void parseFiles();
    std::optional<Entry> parseFile(const std::string& path);

    Er::Log::ILog* const m_log;
    std::shared_mutex m_mutex;
    std::unordered_set<std::string> m_dirs;
    std::unordered_set<std::string> m_files;
    std::unordered_map<std::string, Entry> m_entries;
};


} // DesktopEnv {}

} // namespace Er {}