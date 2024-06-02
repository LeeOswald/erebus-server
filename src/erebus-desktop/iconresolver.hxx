#pragma once

#include <erebus-desktop/erebus-desktop.hxx>


namespace Er
{

namespace Desktop
{

namespace Private
{

//
// lookup process icon name
// 

class IconResolver final
    : public Er::Desktop::IAppEntryCallback
    , public Er::NonCopyable
{
public:
    ~IconResolver();
    explicit IconResolver(Er::Log::ILog* log, std::shared_ptr<Er::Desktop::IAppEntryMonitor> appEntryMonitor);

    std::string lookup(std::optional<std::string> exe, std::optional<std::string> comm, std::optional<uint64_t> pid);

private:
    void appEntryAdded(std::shared_ptr<Er::Desktop::AppEntry> app) override;

    Er::Log::ILog* const m_log;
    std::shared_ptr<Er::Desktop::IAppEntryMonitor> m_appEntryMonitor;
};



} // namespace Private {}

} // namespace Desktop {}

} // namespace Er {}