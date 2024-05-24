#pragma once

#include <erebus/log.hxx>
#include <erebus-desktop/erebus-desktop.hxx>
#include <erebus-processmgr/processmgr.hxx>

#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Er
{

namespace Private
{
    
class IconCache final
    : public Er::NonCopyable
{
public:
    ~IconCache();
    explicit IconCache(Er::Log::ILog* log, const std::string& iconTheme, const std::string& iconCacheAgent, const std::string& iconCacheDir);
    
    void prefetch(const std::vector<std::shared_ptr<Er::Desktop::AppEntry>>& icons, unsigned size);
    std::unordered_map<std::string, std::string> lookup(const std::vector<std::shared_ptr<Er::Desktop::AppEntry>>& icons, unsigned size) const; // icon name -> cache path
    std::optional<std::string> lookup(const std::string& iconName, unsigned size) const;

private:
    std::string makeCachePath(const std::string& name, unsigned size) const;
    int callCacheAgent(const std::string* sourceFile, const std::vector<std::string>* icons, unsigned size, std::stop_token* stop) const noexcept;

    Er::Log::ILog* const m_log;
    const std::string m_iconTheme;
    const std::string m_iconCacheAgent;
    const std::string m_iconCacheDir;
    std::atomic<long> m_workerExited;
    std::mutex m_mutex;
    std::unique_ptr<std::jthread> m_worker;
};


} // namespace Private {}

} // namespace Er {}
