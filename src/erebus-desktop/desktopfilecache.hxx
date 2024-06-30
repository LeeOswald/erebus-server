#pragma once


#include <erebus-desktop/erebus-desktop.hxx>
#include <erebus/log.hxx>

#include <shared_mutex>
#include <thread>
#include <unordered_map>

#include <boost/filesystem/path.hpp>


namespace Er
{

namespace Desktop
{

namespace Private
{


struct DesktopFile
{
    using Ptr = std::shared_ptr<DesktopFile>;

    std::string name;
    std::string exec;
    std::string real_exec;
    std::string icon;
};


class DesktopFileCache final
    : public Er::NonCopyable
{
public:
    explicit DesktopFileCache(Er::Log::ILog* log);

    DesktopFile::Ptr lookupByExec(const std::string& exec) const;
    DesktopFile::Ptr lookupByPath(const std::string& path) const;

private:
    void worker(std::stop_token stop);
    void addXdgDataDirs(std::stop_token stop);
    void addUserDirs(std::stop_token stop);
    void parseFiles(const std::string& dir, std::stop_token stop);
    DesktopFile::Ptr parseFile(const std::string& filePath) const;
    std::optional<std::string> findExePath(const std::string& exe) const;
    
    std::vector<boost::filesystem::path> m_searchPaths;
    Er::Log::ILog* const m_log;
    std::jthread m_worker; 
    mutable std::shared_mutex m_dirsLock;
    std::vector<std::string> m_dirs;
    mutable std::shared_mutex m_entriesLock;
    mutable std::unordered_map<std::string, DesktopFile::Ptr> m_entriesByExec; // key = exe path
    mutable std::unordered_map<std::string, DesktopFile::Ptr> m_entriesByPath; // key = .desktop file path
 };


} // namespace Private {}

} // namespace Desktop {}

} // namespace Er {}