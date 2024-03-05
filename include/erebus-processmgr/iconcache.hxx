#pragma once

#include <erebus/log.hxx>
#include <erebus-processmgr/processmgr.hxx>


#include <vector>

namespace Er
{

class ER_PROCESSMGR_EXPORT IconCache final
    : public Er::NonCopyable
{
public:
    explicit IconCache(Er::Log::ILog* log, const std::string& iconCache, const std::string& iconCacheDir);
    
    std::optional<std::string> lookup(const std::string& name, unsigned size);

private:
    std::string makeCachePath(const std::string& name, unsigned size) const;
    bool callCacheAgent(const std::string& iconName, unsigned size) const;

    Er::Log::ILog* const m_log;
    std::string m_iconCache;
    std::string m_iconCacheDir;
};

} // namespace Er {}