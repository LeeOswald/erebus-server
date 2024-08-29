#pragma once

#include <erebus/lrucache.hxx>
#include <erebus-desktop/ic.hxx>
#include <erebus-processmgr/erebus-processmgr.hxx>

#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_map>


namespace Erp
{

namespace Desktop
{



enum class IconSize : unsigned
{
    Small = 16,
    Large = 32
};


class IconCache final
    : public Er::NonCopyable
{
public:
    struct IconData
    {
        Er::Desktop::IconState state;
        Er::Binary raw;

        IconData(Er::Desktop::IconState state)
            : state(state)
        {}

        template <typename RawT>
        IconData(RawT&& raw)
            : state(Er::Desktop::IconState::Found)
            , raw(std::forward<RawT>(raw))
        {}
    };

    ~IconCache();
    explicit IconCache(
        Er::Log::ILog* log, 
        std::shared_ptr<Er::Desktop::IIconCacheIpc> iconCacheIpc, 
        const std::string& cacheDir, 
        size_t cacheSize
        );

    std::shared_ptr<IconData> lookupByName(const std::string& name, IconSize size);
    
private:
    struct IconInfo
    {
        using Clock = std::chrono::steady_clock;

        Er::Desktop::IconState state;
        Clock::time_point timestamp;
        std::string path;

        explicit IconInfo(Er::Desktop::IconState state) noexcept
            : state(state)
            , timestamp(Clock::now())
        {}

        template <typename PathT>
        explicit IconInfo(PathT&& path) noexcept
            : state(Er::Desktop::IconState::Found)
            , timestamp(Clock::now())
            , path(std::forward<PathT>(path))
        {}
    };


    void iconWorker(std::stop_token stop) noexcept;
    std::shared_ptr<IconInfo> searchCacheDir(const std::string& name, IconSize size) noexcept;
    std::shared_ptr<IconInfo> requestIcon(const std::string& name, IconSize size) noexcept;
    void receiveIcon() noexcept;

    static constexpr size_t MaxAppQueue = 1024;
    static constexpr std::chrono::milliseconds Timeout = std::chrono::milliseconds(2000);
    static constexpr std::chrono::minutes IconRequestExpired = std::chrono::minutes(10);

    Er::Log::ILog* const m_log;
    std::shared_ptr<Er::Desktop::IIconCacheIpc> m_iconCacheIpc;
    std::string m_cacheDir;
    
    std::shared_mutex m_appIconsLock;
    std::unordered_map<std::string, std::shared_ptr<IconInfo>> m_appIcons16; // icon name -> icon path
    std::unordered_map<std::string, std::shared_ptr<IconInfo>> m_appIcons32;

    std::mutex m_cacheLock;
    Er::LruCache<std::string, std::shared_ptr<IconData>> m_cache16; // icon path -> icon bytes
    Er::LruCache<std::string, std::shared_ptr<IconData>> m_cache32;
    
    std::jthread m_iconWorker;
};
    

} // namespace Desktop {}

} // namespace Erp {}