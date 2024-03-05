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
    explicit IconCache(Er::Log::ILog* log);
    
private:
    Er::Log::ILog* const m_log;
};

} // namespace Er {}