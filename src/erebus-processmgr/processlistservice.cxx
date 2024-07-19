#include "processlistservice.hxx"

#include <erebus/exception.hxx>
#include <erebus/util/format.hxx>

#include <time.h>

namespace Erp
{

namespace ProcessMgr
{

namespace
{

double rtime() noexcept // current time, sec
{
    struct timespec time = {};
    ::clock_gettime(CLOCK_MONOTONIC, &time);
    double v = time.tv_sec;
    v += double(time.tv_nsec) / 1000000000LL;
    return v;
} 

} // namespace {}


ProcessListService::~ProcessListService()
{
}

ProcessListService::ProcessListService(Er::Log::ILog* log)
    : m_log(log)
    , m_procFs(log)
{
}

void ProcessListService::registerService(Er::Server::IServiceContainer* container)
{
    container->registerService(Er::ProcessMgr::ProcessRequests::ListProcesses, this);
    container->registerService(Er::ProcessMgr::ProcessRequests::ListProcessesDiff, this);
    container->registerService(Er::ProcessMgr::ProcessRequests::ProcessDetails, this);
    container->registerService(Er::ProcessMgr::ProcessRequests::ProcessesGlobal, this);
}

void ProcessListService::unregisterService(Er::Server::IServiceContainer* container)
{
    container->unregisterService(this);
}

ProcessListService::SessionId ProcessListService::allocateSession()
{
    std::unique_lock l(m_mutexSession);
    auto id = m_nextSessionId++;

    m_sessions.insert({ id, std::make_unique<Session>(id) });

    return id;
}

void ProcessListService::deleteSession(SessionId id)
{
    std::unique_lock l(m_mutexSession);

    auto it = m_sessions.find(id);
    if (it == m_sessions.end())
        throw Er::Exception(ER_HERE(), Er::Util::format("Non-existent session %d", id));

    m_sessions.erase(it);

    dropStaleSessions();
}

ProcessListService::Session* ProcessListService::getSession(std::optional<SessionId> id)
{
    if (!id)
        throw Er::Exception(ER_HERE(), "Session not specified");

    std::unique_lock l(m_mutexSession);

    auto it = m_sessions.find(*id);
    if (it == m_sessions.end())
        throw Er::Exception(ER_HERE(), "Session not found");

    it->second->touched = std::chrono::steady_clock::now();

    return it->second.get();
}

Er::PropertyBag ProcessListService::request(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId)
{
    if (request == Er::ProcessMgr::ProcessRequests::ProcessDetails)
    {
        auto required = getProcessPropMask(args);
        return processDetails(args, required);
    }
    else if (request == Er::ProcessMgr::ProcessRequests::ProcessesGlobal)
    {
        auto required = getProcessesGlobalPropMask(args);
        return processesGlobal(required, std::nullopt);
    }

    throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported request %s", std::string(request).c_str()));
}

ProcessListService::StreamId ProcessListService::beginStream(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId)
{
    if (request == Er::ProcessMgr::ProcessRequests::ListProcesses)
        return beginProcessStream(args);
    else if (request == Er::ProcessMgr::ProcessRequests::ListProcessesDiff)
        return beginProcessDiffStream(args, getSession(sessionId));

    throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported request %s", std::string(request).c_str()));
}

void ProcessListService::endStream(StreamId id, std::optional<SessionId> sessionId)
{
    std::unique_lock l(m_mutexSession);

    auto it = m_streams.find(id);
    if (it == m_streams.end())
        throw Er::Exception(ER_HERE(), Er::Util::format("Non-existent stream %d", id));

    m_streams.erase(it);

    dropStaleStreams();
}

Er::PropertyBag ProcessListService::next(StreamId id, std::optional<SessionId> sessionId)
{
    Stream* stream = nullptr;
    {
        std::unique_lock l(m_mutexSession);
        auto it = m_streams.find(id);
        if (it == m_streams.end())
            throw Er::Exception(ER_HERE(), Er::Util::format("Non-existent stream %d", id));

        stream = it->second.get();

        stream->touched = std::chrono::steady_clock::now();
    }

    if (stream->type == StreamType::ProcessList)
        return nextProcess(static_cast<ProcessListStream*>(stream));
    else if (stream->type == StreamType::ProcessListDiff)
        return nextProcessDiff(static_cast<ProcessListDiffStream*>(stream), getSession(sessionId));

    ErAssert(!"Unknown stream type");
    return Er::PropertyBag();
}

Er::ProcessMgr::ProcessProps::PropMask ProcessListService::getProcessPropMask(const Er::PropertyBag& args)
{
    // default mask is 'everything included'
    auto mask = Er::getPropertyValueOr<Er::ProcessMgr::ProcessProps::RequiredFields>(args, 0xffffffffffffffff);
    return Er::ProcessMgr::ProcessProps::PropMask(mask, Er::ProcessMgr::ProcessProps::PropMask::FromBits);
}

Er::ProcessMgr::ProcessesGlobal::PropMask ProcessListService::getProcessesGlobalPropMask(const Er::PropertyBag& args)
{
    // default mask is 'everything included'
    auto mask = Er::getPropertyValueOr<Er::ProcessMgr::ProcessesGlobal::RequiredFields>(args, 0xffffffffffffffff);
    return Er::ProcessMgr::ProcessesGlobal::PropMask(mask, Er::ProcessMgr::ProcessesGlobal::PropMask::FromBits);
}

Er::PropertyBag ProcessListService::processDetails(const Er::PropertyBag& args, Er::ProcessMgr::ProcessProps::PropMask required)
{
    auto pid = Er::getPropertyValue<Er::ProcessMgr::ProcessProps::Pid>(args);
    if (!pid)
        throw Er::Exception(ER_HERE(), "No process.pid field in ProcessDetails request");

    if (*pid == KernelPid)
        return collectKernelDetails(m_procFs, required);
        
    ProcessDetailsCached cached;
    return collectProcessDetails(m_procFs, *pid, required, Er::PropertyBag(), cached);
}

Er::PropertyBag ProcessListService::processesGlobal(Er::ProcessMgr::ProcessesGlobal::PropMask required, std::optional<uint64_t> processCount)
{
    Er::PropertyBag bag;
    
    if (!processCount)
    {
        if (required[Er::ProcessMgr::ProcessesGlobal::PropIndices::ProcessCount])
        {
            auto pids = m_procFs.enumeratePids();
            processCount = pids.size();
        }
        else
        {
            processCount = 0;
        }
    }

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

    if (required[Er::ProcessMgr::ProcessesGlobal::PropIndices::ProcessCount])
    {
        Er::addProperty<Er::ProcessMgr::ProcessesGlobal::ProcessCount>(bag, *processCount);
    }

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

void ProcessListService::dropStaleStreams() noexcept
{
    auto now = std::chrono::steady_clock::now();

    for (auto it = m_streams.begin(); it != m_streams.end();)
    {
        auto d = std::chrono::duration_cast<std::chrono::seconds>(now - it->second->touched);
        if (d.count() > kStreamTimeoutSeconds)
        {
            auto next = std::next(it);
            ErLogWarning(m_log, "Dropping stale stream %d", it->first);
            m_streams.erase(it);
            it = next;
        }
        else
        {
            ++it;
        }
    }
}

void ProcessListService::dropStaleSessions() noexcept
{
    auto now = std::chrono::steady_clock::now();

    for (auto it = m_sessions.begin(); it != m_sessions.end();)
    {
        auto d = std::chrono::duration_cast<std::chrono::seconds>(now - it->second->touched);
        if (d.count() > kSessionTimeoutSeconds)
        {
            auto next = std::next(it);
            ErLogWarning(m_log, "Dropping stale session %d", it->first);
            m_sessions.erase(it);
            it = next;
        }
        else
        {
            ++it;
        }
    }
}

ProcessListService::StreamId ProcessListService::beginProcessStream(const Er::PropertyBag& args)
{
    auto pids = m_procFs.enumeratePids();
    auto required = getProcessPropMask(args);

    std::unique_lock l(m_mutexSession);
    
    auto streamId = m_nextStreamId++;
    auto stream = std::make_unique<ProcessListStream>(streamId, required, std::move(pids));
    m_streams.insert({ streamId, std::move(stream) });

    return streamId;
}

Er::PropertyBag ProcessListService::nextProcess(ProcessListStream* stream)
{
    if (stream->next >= stream->pids.size())
    {
        return Er::PropertyBag(); // end of stream
    }

    ProcessDetailsCached cached;
    auto bag = collectProcessDetails(m_procFs, stream->pids[stream->next], stream->required, Er::PropertyBag(), cached);

    ++stream->next;

    return bag;
}

ProcessListService::StreamId ProcessListService::beginProcessDiffStream(const Er::PropertyBag& args, Session* session)
{
    auto required = getProcessPropMask(args);

    ProcessStatistics stats;
    auto diff = updateProcessCollection(m_procFs, required, session->processes, stats);
    
    std::unique_lock l(m_mutexSession);

    auto streamId = m_nextStreamId++;

    auto stream = std::make_unique<ProcessListDiffStream>(streamId, required, std::move(diff));
    m_streams.insert({ streamId, std::move(stream) });

    return streamId;
}

Er::PropertyBag ProcessListService::nextProcessDiff(ProcessListDiffStream* stream, Session* session)
{
    Er::PropertyBag bag;

    if (stream->stage == ProcessListDiffStream::Stage::Globals)
    {
        bag = processesGlobal(Er::ProcessMgr::ProcessesGlobal::PropMask(0xffffffffffffffff, Er::ProcessMgr::ProcessesGlobal::PropMask::FromBits), stream->diff.processCount);
        stream->stage = ProcessListDiffStream::Stage::Removed;
    }
    else if (stream->stage == ProcessListDiffStream::Stage::Removed)
    {
        // pack next removed process
        if (stream->next >= stream->diff.removed.size())
        {
            stream->stage = ProcessListDiffStream::Stage::Modified;
            stream->next = 0;
            return nextProcessDiff(stream, session);
        }

        Er::addProperty<Er::ProcessMgr::ProcessProps::Pid>(bag, stream->diff.removed[stream->next]);
        Er::addProperty<Er::ProcessMgr::ProcessProps::IsDeleted>(bag, true);

        ++stream->next;
    }
    else if (stream->stage == ProcessListDiffStream::Stage::Modified)
    {
        // pack next modified process
        if (stream->next >= stream->diff.modified.size())
        {
            stream->stage = ProcessListDiffStream::Stage::Added;
            stream->next = 0;
            return nextProcessDiff(stream, session);
        }

        auto& modified = stream->diff.modified[stream->next];
        Er::enumerateProperties(modified.properties, [&bag](Er::Property& prop)
        {
            Er::addProperty(bag, std::move(prop));
        });

        Er::addProperty<Er::ProcessMgr::ProcessProps::Pid>(bag, modified.pid);
        Er::addProperty<Er::ProcessMgr::ProcessProps::Valid>(bag, true);

        ++stream->next;
    }
    else
    {
        // pack next added process
        if (stream->next >= stream->diff.added.size())
            return Er::PropertyBag(); // end of stream

        auto& added = stream->diff.added[stream->next];
        if (added->isNew)
            Er::addProperty<Er::ProcessMgr::ProcessProps::IsNew>(bag, true);

        Er::enumerateProperties(added->properties, [&bag](const Er::Property& prop)
        {
            Er::addProperty(bag, prop);
        });

        ErAssert(Er::propertyPresent<Er::ProcessMgr::ProcessProps::Pid>(bag));
        ErAssert(Er::propertyPresent<Er::ProcessMgr::ProcessProps::Valid>(bag));
        
        ++stream->next;
    }
    

    return bag;
}

} // namespace ProcessMgr {}

} // namespace Erp {}