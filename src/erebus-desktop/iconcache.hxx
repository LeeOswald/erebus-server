#pragma once

#include <erebus/log.hxx>
#include <erebus/lrucache.hxx>
#include <erebus-desktop/erebus-desktop.hxx>
#include <erebus-processmgr/processmgr.hxx>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <thread>
#include <unordered_map>


namespace Er
{

namespace Private
{


enum class IconSize : unsigned
{
    Small = 16,
    Large = 32
};


namespace KnownIcons
{

std::string defaultExeIcon();

std::string knownAppIcon(const std::string& comm);

} // namespace KnownIcons {}


class IconManager final
    : public Er::Desktop::IAppEntryCallback
    , public Er::NonCopyable
{
public:
    enum class IconState
    {
        Requested,
        NotFound,
        Found,
        Cached,
        Failure
    };

    struct IconData
    {
        IconState state;
        Bytes raw;

        IconData(IconState state)
            : state(state)
        {}

        template <typename RawT>
        IconData(RawT&& raw)
            : state(IconState::Cached)
            , raw(std::forward<RawT>(raw))
        {}
    };

    ~IconManager();
    explicit IconManager(Er::Log::ILog* log, std::shared_ptr<Er::Desktop::IIconCacheIpc> iconCacheIpc, std::shared_ptr<Er::Desktop::IAppEntryMonitor> appEntryMonitor, size_t cacheSize);

    void appEntryAdded(std::shared_ptr<Er::Desktop::AppEntry> app) override;

    std::shared_ptr<IconData> lookupByExe(const std::string& exe, IconSize size);
    std::shared_ptr<IconData> lookupByName(const std::string& name, IconSize size);
    
private:
    struct IconInfo
    {
        using Clock = std::chrono::steady_clock;

        IconState state;
        Clock::time_point timestamp;
        std::string path;

        explicit IconInfo(IconState state) noexcept
            : state(state)
            , timestamp(Clock::now())
        {}

        explicit IconInfo(std::string path) noexcept
            : state(IconState::Found)
            , timestamp(Clock::now())
            , path(path)
        {}
    };


    void appEntryWorker(std::stop_token stop) noexcept;
    void iconWorker(std::stop_token stop) noexcept;
    IconState requestIcon(const std::string& name, IconSize size) noexcept;
    void receiveIcon() noexcept;

    static constexpr size_t MaxAppQueue = 1024;
    static constexpr std::chrono::milliseconds Timeout = std::chrono::milliseconds(1000);
    static constexpr std::chrono::minutes IconRequestExpired = std::chrono::minutes(10);

    Er::Log::ILog* const m_log;
    std::shared_ptr<Er::Desktop::IIconCacheIpc> m_iconCacheIpc;
    std::shared_ptr<Er::Desktop::IAppEntryMonitor> m_appEntryMonitor;
    
    std::mutex m_addedAppsLock;
    std::queue<std::shared_ptr<Er::Desktop::AppEntry>> m_addedApps;
    std::condition_variable_any m_addedAppsEvent;

    std::shared_mutex m_appIconsLock;
    std::unordered_map<std::string, std::shared_ptr<IconInfo>> m_appIcons16; // icon name -> icon path
    std::unordered_map<std::string, std::shared_ptr<IconInfo>> m_appIcons32;

    std::mutex m_cacheLock;
    Er::LruCache<std::string, std::shared_ptr<IconData>> m_cache16; // icon path -> icon bytes
    Er::LruCache<std::string, std::shared_ptr<IconData>> m_cache32;
    
    std::jthread m_appEntryWorker;
    std::jthread m_iconWorker;
};
    

} // namespace Private {}

} // namespace Er {}