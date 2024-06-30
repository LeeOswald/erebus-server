#pragma once

#include <erebus-desktop/erebus-desktop.hxx>
#include <erebus/log.hxx>

#include "procfs.hxx"

namespace Er
{

namespace Desktop
{

namespace Private
{

class DesktopFileCache;


class IconResolver final
    : public Er::NonCopyable
{
public:
    explicit IconResolver(Er::Log::ILog* log, std::shared_ptr<DesktopFileCache> m_desktopFileCache);

    std::string lookupIcon(uint64_t pid) const;

private:
    std::string desktopFileFor(uint64_t pid, const std::string& comm, const std::string& exec) const;

    Er::Log::ILog* const m_log;
    std::shared_ptr<DesktopFileCache> m_desktopFileCache;
    ProcFs m_procFs;
};



} // namespace Private {}

} // namespace Desktop {}

} // namespace Er {}