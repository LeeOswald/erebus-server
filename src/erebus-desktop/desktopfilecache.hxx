#pragma once


#include <erebus-desktop/erebus-desktop.hxx>
#include <erebus/log.hxx>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

#include <boost/filesystem/path.hpp>


namespace Erp::Desktop
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

    void addXdgDataDirs(const std::string& dirs);
    bool waitIdle(std::chrono::milliseconds timeout);
    DesktopFile::Ptr lookupByExec(const std::string& exec);
    DesktopFile::Ptr lookupByPath(const std::string& path);

private:
    void worker(std::stop_token stop);
    void addUserDirs(std::stop_token stop);
    void parseFiles(const std::string& dir, std::stop_token stop);
    DesktopFile::Ptr parseFile(const std::string& filePath) const;
    std::optional<std::string> findExePath(const std::string& exe) const;
    
    std::vector<boost::filesystem::path> m_searchPaths;
    Er::Log::ILog* const m_log;
    std::jthread m_worker; 

    mutable std::mutex m_dirsLock;
    std::condition_variable_any m_dirsCv;
    std::queue<std::string> m_dirs;
    bool m_idle = true;
    
    mutable std::mutex m_entriesLock;
    std::unordered_map<std::string, DesktopFile::Ptr> m_entriesByExec; // key = exe path
    std::unordered_map<std::string, DesktopFile::Ptr> m_entriesByPath; // key = .desktop file path
 };



} // namespace Erp::Desktop {}