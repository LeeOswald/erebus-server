#pragma once

#include <erebus/log.hxx>
#include <erebus/lrucache.hxx>
#include <erebus-processmgr/processmgr.hxx>

#include <shared_mutex>

namespace Er
{

namespace Desktop
{

class DesktopEntries;

} // namespace Desktop {}

namespace Private
{

class IconCache;

enum class IconSize : unsigned
{
    Small = 16,
    Large = 32
};


class IconManager final
    : public Er::NonCopyable
{
public:
    struct IconData
    {
        bool valid; // no icon exists for this exe
        Bytes data;

        IconData() noexcept
            : valid(false)
        {}

        template <typename BytesT>
        explicit IconData(BytesT&& data) noexcept
            : valid(true)
            , data(std::forward<BytesT>(data))
        {}
    };

    explicit IconManager(Er::Log::ILog* log, IconCache* iconCache, Er::Desktop::DesktopEntries* desktopEntries, size_t cacheSize);

    void prefetch(IconSize size);
    std::shared_ptr<IconData> lookup(const std::string& comm, const std::string& exe, IconSize size) const noexcept;
    std::shared_ptr<IconData> defaultIcon(const std::string& comm, const std::string& exe, IconSize size) const noexcept;

private:
    Er::Log::ILog* const m_log;
    IconCache* m_iconCache;
    Er::Desktop::DesktopEntries* m_desktopEntries;
    std::string const DefaultExeIcon;
    mutable std::shared_mutex m_mutex;
    mutable Er::LruCache<std::string, std::shared_ptr<IconData>> m_cache16; // exec -> icon
    mutable Er::LruCache<std::string, std::shared_ptr<IconData>> m_cache32;
    mutable std::unordered_map<std::string, std::shared_ptr<IconData>> m_default16;
    mutable std::unordered_map<std::string, std::shared_ptr<IconData>> m_default32;
};
    

} // namespace Private {}

} // namespace Er {}