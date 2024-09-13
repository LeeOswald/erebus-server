#include "globalscollector.hxx"

#include <time.h>

namespace Erp
{

namespace ProcessMgr
{

namespace
{

//
// current time, sec
//

double rtime() noexcept 
{
    struct timespec time = {};
    ::clock_gettime(CLOCK_MONOTONIC, &time);
    double v = time.tv_sec;
    v += double(time.tv_nsec) / 1000000000LL;
    return v;
} 

} // namespace {}


GlobalsCollector::GlobalsCollector(Er::Log::ILog* log, ProcFs& procFs)
    : m_log(log)
    , m_procFs(procFs)
{
}

Er::PropertyBag GlobalsCollector::collect(PropMask required)
{
    Er::PropertyBag bag;

    auto cpuTimes = m_procFs.readCpuTimes();

    // guest time is already accounted in user time
    cpuTimes.all.user -= cpuTimes.all.guest;
    cpuTimes.all.user_nice -= cpuTimes.all.guest_nice;

    auto idleAll = cpuTimes.all.idle + cpuTimes.all.iowait;
    auto userAll = cpuTimes.all.user + cpuTimes.all.user_nice;
    auto systemAll = cpuTimes.all.system + cpuTimes.all.irq + cpuTimes.all.softirq;
    auto virtAll = cpuTimes.all.guest + cpuTimes.all.guest_nice;
    auto totalAll = userAll + systemAll + cpuTimes.all.steal + virtAll;

    auto cpuCount = cpuTimes.cores.size();
    if (!cpuCount) [[unlikely]]
        cpuCount = 1;

    if (required[Er::ProcessMgr::GlobalProps::PropIndices::RealTime])
    {
        auto r = rtime();
        Er::addProperty<Er::ProcessMgr::GlobalProps::RealTime>(bag, r);
    }

    if (required[Er::ProcessMgr::GlobalProps::PropIndices::IdleTime])
    {
        Er::addProperty<Er::ProcessMgr::GlobalProps::IdleTime>(bag, idleAll / cpuCount);
    }

    if (required[Er::ProcessMgr::GlobalProps::PropIndices::UserTime])
    {
        Er::addProperty<Er::ProcessMgr::GlobalProps::UserTime>(bag, userAll / cpuCount);
    }

    if (required[Er::ProcessMgr::GlobalProps::PropIndices::SystemTime])
    {
        Er::addProperty<Er::ProcessMgr::GlobalProps::SystemTime>(bag, systemAll / cpuCount);
    }

    if (required[Er::ProcessMgr::GlobalProps::PropIndices::VirtualTime])
    {
        Er::addProperty<Er::ProcessMgr::GlobalProps::VirtualTime>(bag, virtAll / cpuCount);
    }

    if (required[Er::ProcessMgr::GlobalProps::PropIndices::TotalTime])
    {
        Er::addProperty<Er::ProcessMgr::GlobalProps::TotalTime>(bag, totalAll / cpuCount);
    }

    auto mem = m_procFs.readMemStats();

    if (required[Er::ProcessMgr::GlobalProps::PropIndices::TotalMem])
        Er::addProperty<Er::ProcessMgr::GlobalProps::TotalMem>(bag, mem.totalMem);

    if (required[Er::ProcessMgr::GlobalProps::PropIndices::UsedMem])
        Er::addProperty<Er::ProcessMgr::GlobalProps::UsedMem>(bag, mem.usedMem);

    if (required[Er::ProcessMgr::GlobalProps::PropIndices::BuffersMem])
        Er::addProperty<Er::ProcessMgr::GlobalProps::BuffersMem>(bag, mem.buffersMem);

    if (required[Er::ProcessMgr::GlobalProps::PropIndices::CachedMem])
        Er::addProperty<Er::ProcessMgr::GlobalProps::CachedMem>(bag, mem.cachedMem);

    if (required[Er::ProcessMgr::GlobalProps::PropIndices::SharedMem])
        Er::addProperty<Er::ProcessMgr::GlobalProps::SharedMem>(bag, mem.sharedMem);

    if (required[Er::ProcessMgr::GlobalProps::PropIndices::AvailableMem])
        Er::addProperty<Er::ProcessMgr::GlobalProps::AvailableMem>(bag, mem.availableMem);

    if (required[Er::ProcessMgr::GlobalProps::PropIndices::TotalSwap])
        Er::addProperty<Er::ProcessMgr::GlobalProps::TotalSwap>(bag, mem.totalSwap);

    if (required[Er::ProcessMgr::GlobalProps::PropIndices::UsedSwap])
        Er::addProperty<Er::ProcessMgr::GlobalProps::UsedSwap>(bag, mem.usedSwap);

    if (required[Er::ProcessMgr::GlobalProps::PropIndices::CachedSwap])
        Er::addProperty<Er::ProcessMgr::GlobalProps::CachedSwap>(bag, mem.cachedSwap);

    if (required[Er::ProcessMgr::GlobalProps::PropIndices::ZSwapComp])
        Er::addProperty<Er::ProcessMgr::GlobalProps::ZSwapComp>(bag, mem.zswapComp);

    if (required[Er::ProcessMgr::GlobalProps::PropIndices::ZSwapOrig])
        Er::addProperty<Er::ProcessMgr::GlobalProps::ZSwapOrig>(bag, mem.zswapOrig);

    Er::addProperty<Er::ProcessMgr::GlobalProps::Global>(bag, true);

    return bag;
}

} // namespace ProcessMgr {}

} // namespace Erp {}
