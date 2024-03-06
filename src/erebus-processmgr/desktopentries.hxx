#pragma once

#include <erebus/log.hxx>
#include <erebus-processmgr/processmgr.hxx>

#include "pathresolver.hxx"

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
    std::optional<std::string> resolveExePath(std::string_view exe) const;

    Er::Log::ILog* const m_log;
    PathResolver m_pathResolver;
    std::vector<std::string> m_dirs;
    std::vector<std::string> m_files;
    std::unordered_map<std::string, Entry> m_entries;
};


} // Private {}

} // namespace Er {}
