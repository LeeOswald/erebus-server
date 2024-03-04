#pragma once

#include <erebus/log.hxx>
#include <erebus-processmgr/processmgr.hxx>


#include <shared_mutex>
#include <vector>


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
    void addXdgDataDirs();
    void addUserDirs();

    Er::Log::ILog* const m_log;
    std::shared_mutex m_mutex;
    std::vector<std::string> m_dirs;
};


} // DesktopEnv {}

} // namespace Er {}