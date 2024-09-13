#pragma once

#include <erebus-processmgr/erebus-processmgr.hxx>

#include "procfs.hxx"


namespace Erp
{

namespace ProcessMgr
{


class GlobalsCollector final
    : public Er::NonCopyable
{
public:
    using PropMask = Er::ProcessMgr::GlobalProps::PropMask;

    explicit GlobalsCollector(Er::Log::ILog* log, ProcFs& procFs);

    Er::PropertyBag collect(PropMask required);

private:
    Er::Log::ILog* const m_log;
    ProcFs& m_procFs;
};


} // namespace ProcessMgr {}

} // namespace Erp {}