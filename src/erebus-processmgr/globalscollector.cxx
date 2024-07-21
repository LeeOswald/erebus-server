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

    if (required[Er::ProcessMgr::ProcessesGlobal::PropIndices::RealTime])
    {
        auto r = rtime();
        Er::addProperty<Er::ProcessMgr::ProcessesGlobal::RealTime>(bag, r);
    }

    if (required[Er::ProcessMgr::ProcessesGlobal::PropIndices::IdleTime])
    {
        Er::addProperty<Er::ProcessMgr::ProcessesGlobal::IdleTime>(bag, idleAll / cpuCount);
    }

    if (required[Er::ProcessMgr::ProcessesGlobal::PropIndices::UserTime])
    {
        Er::addProperty<Er::ProcessMgr::ProcessesGlobal::UserTime>(bag, userAll / cpuCount);
    }

    if (required[Er::ProcessMgr::ProcessesGlobal::PropIndices::SystemTime])
    {
        Er::addProperty<Er::ProcessMgr::ProcessesGlobal::SystemTime>(bag, systemAll / cpuCount);
    }

    if (required[Er::ProcessMgr::ProcessesGlobal::PropIndices::VirtualTime])
    {
        Er::addProperty<Er::ProcessMgr::ProcessesGlobal::VirtualTime>(bag, virtAll / cpuCount);
    }

    if (required[Er::ProcessMgr::ProcessesGlobal::PropIndices::TotalTime])
    {
        Er::addProperty<Er::ProcessMgr::ProcessesGlobal::TotalTime>(bag, totalAll / cpuCount);
    }

    auto mem = m_procFs.readMemStats();

    if (required[Er::ProcessMgr::ProcessesGlobal::PropIndices::TotalMem])
        Er::addProperty<Er::ProcessMgr::ProcessesGlobal::TotalMem>(bag, mem.totalMem);

    if (required[Er::ProcessMgr::ProcessesGlobal::PropIndices::UsedMem])
        Er::addProperty<Er::ProcessMgr::ProcessesGlobal::UsedMem>(bag, mem.usedMem);

    if (required[Er::ProcessMgr::ProcessesGlobal::PropIndices::BuffersMem])
        Er::addProperty<Er::ProcessMgr::ProcessesGlobal::BuffersMem>(bag, mem.buffersMem);

    if (required[Er::ProcessMgr::ProcessesGlobal::PropIndices::CachedMem])
        Er::addProperty<Er::ProcessMgr::ProcessesGlobal::CachedMem>(bag, mem.cachedMem);

    if (required[Er::ProcessMgr::ProcessesGlobal::PropIndices::SharedMem])
        Er::addProperty<Er::ProcessMgr::ProcessesGlobal::SharedMem>(bag, mem.sharedMem);

    if (required[Er::ProcessMgr::ProcessesGlobal::PropIndices::AvailableMem])
        Er::addProperty<Er::ProcessMgr::ProcessesGlobal::AvailableMem>(bag, mem.availableMem);

    if (required[Er::ProcessMgr::ProcessesGlobal::PropIndices::TotalSwap])
        Er::addProperty<Er::ProcessMgr::ProcessesGlobal::TotalSwap>(bag, mem.totalSwap);

    if (required[Er::ProcessMgr::ProcessesGlobal::PropIndices::UsedSwap])
        Er::addProperty<Er::ProcessMgr::ProcessesGlobal::UsedSwap>(bag, mem.usedSwap);

    if (required[Er::ProcessMgr::ProcessesGlobal::PropIndices::CachedSwap])
        Er::addProperty<Er::ProcessMgr::ProcessesGlobal::CachedSwap>(bag, mem.cachedSwap);

    if (required[Er::ProcessMgr::ProcessesGlobal::PropIndices::ZSwapComp])
        Er::addProperty<Er::ProcessMgr::ProcessesGlobal::ZSwapComp>(bag, mem.zswapComp);

    if (required[Er::ProcessMgr::ProcessesGlobal::PropIndices::ZSwapOrig])
        Er::addProperty<Er::ProcessMgr::ProcessesGlobal::ZSwapOrig>(bag, mem.zswapOrig);

    Er::addProperty<Er::ProcessMgr::ProcessesGlobal::Global>(bag, true);

    return bag;
}

} // namespace ProcessMgr {}

} // namespace Erp {}
