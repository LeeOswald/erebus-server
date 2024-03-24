#include "iconmanager.hxx"
#include "processlist.hxx"

#include <erebus/exception.hxx>
#include <erebus/util/format.hxx>

#include <time.h>

namespace Er
{

namespace Private
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


ProcessList::~ProcessList()
{
    LogDebug(m_log, LogInstance("ProcessList"), "~ProcessList()");
}

ProcessList::ProcessList(Er::Log::ILog* log, IconManager* iconManager)
    : m_log(log)
    , m_iconManager(iconManager)
    , m_procFs(log)
{
    LogDebug(m_log, LogInstance("ProcessList"), "ProcessList()");
}

void ProcessList::registerService(Er::Server::IServiceContainer* container)
{
    container->registerService(Er::ProcessRequests::ListProcesses, this);
    container->registerService(Er::ProcessRequests::ListProcessesDiff, this);
    container->registerService(Er::ProcessRequests::ProcessDetails, this);
    container->registerService(Er::ProcessRequests::ProcessesGlobal, this);
}

void ProcessList::unregisterService(Er::Server::IServiceContainer* container)
{
    container->unregisterService(this);
}

ProcessList::SessionId ProcessList::allocateSession()
{
    std::unique_lock l(m_mutexSession);
    auto id = m_nextSessionId++;

    m_sessions.insert({ id, std::make_unique<Session>(id) });

    LogDebug(m_log, LogInstance("ProcessList"), "Started session %d", id);

    return id;
}

void ProcessList::deleteSession(SessionId id)
{
    std::unique_lock l(m_mutexSession);

    auto it = m_sessions.find(id);
    if (it == m_sessions.end())
        throw Er::Exception(ER_HERE(), Er::Util::format("Non-existent session %d", id));

    m_sessions.erase(it);

    dropStaleSessions();

    LogDebug(m_log, LogInstance("ProcessList"), "Ended session %d", id);    
}

ProcessList::Session* ProcessList::getSession(std::optional<SessionId> id)
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

Er::PropertyBag ProcessList::request(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId)
{
    if (request == Er::ProcessRequests::ProcessDetails)
    {
        auto required = getProcessPropMask(args);
        return processDetails(args, required);
    }
    else if (request == Er::ProcessRequests::ProcessesGlobal)
    {
        auto required = getProcessesGlobalPropMask(args);
        return processesGlobal(required, std::nullopt);
    }

    throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported request %s", std::string(request).c_str()));
}

ProcessList::StreamId ProcessList::beginStream(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId)
{
    if (request == Er::ProcessRequests::ListProcesses)
        return beginProcessStream(args);
    else if (request == Er::ProcessRequests::ListProcessesDiff)
        return beginProcessDiffStream(args, getSession(sessionId));

    throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported request %s", std::string(request).c_str()));
}

void ProcessList::endStream(StreamId id, std::optional<SessionId> sessionId)
{
    std::unique_lock l(m_mutexSession);

    auto it = m_streams.find(id);
    if (it == m_streams.end())
        throw Er::Exception(ER_HERE(), Er::Util::format("Non-existent stream %d", id));

    m_streams.erase(it);

    dropStaleStreams();
#if 0
    LogDebug(m_log, LogInstance("ProcessList"), "Ended stream %d", id);    
#endif
}

Er::PropertyBag ProcessList::next(StreamId id, std::optional<SessionId> sessionId)
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

    assert(!"Unknown stream type");
    return Er::PropertyBag();
}

Er::ProcessProps::PropMask ProcessList::getProcessPropMask(const Er::PropertyBag& args)
{
    // default mask is 'everything included'
    auto mask = Er::getProperty<Er::ProcessProps::RequiredFields>(args, 0xffffffffffffffff);
    return Er::ProcessProps::PropMask(mask, Er::ProcessProps::PropMask::FromBits);
}

Er::ProcessesGlobal::PropMask ProcessList::getProcessesGlobalPropMask(const Er::PropertyBag& args)
{
    // default mask is 'everything included'
    auto mask = Er::getProperty<Er::ProcessesGlobal::RequiredFields>(args, 0xffffffffffffffff);
    return Er::ProcessesGlobal::PropMask(mask, Er::ProcessesGlobal::PropMask::FromBits);
}

Er::PropertyBag ProcessList::processDetails(const Er::PropertyBag& args, Er::ProcessProps::PropMask required)
{
    auto pid = Er::getProperty<Er::ProcessProps::Pid>(args);
    if (!pid)
        throw Er::Exception(ER_HERE(), "No process.pid field in ProcessDetails request");

    if (*pid == ProcFs::KernelPid)
        return collectKernelDetails(m_procFs, required);
        
    ProcessDetailsCached cached;
    return collectProcessDetails(m_procFs, *pid, required, Er::PropertyBag(), cached);
}

Er::PropertyBag ProcessList::processesGlobal(Er::ProcessesGlobal::PropMask required, std::optional<uint64_t> processCount)
{
    Er::PropertyBag bag;
    
    if (!processCount)
    {
        if (required[Er::ProcessesGlobal::PropIndices::ProcessCount])
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

    if (required[Er::ProcessesGlobal::PropIndices::ProcessCount])
    {
        Er::addProperty<Er::ProcessesGlobal::ProcessCount>(bag, *processCount);
    }

    if (required[Er::ProcessesGlobal::PropIndices::RealTime])
    {
        auto r = rtime();
        Er::addProperty<Er::ProcessesGlobal::RealTime>(bag, r);
    }

    if (required[Er::ProcessesGlobal::PropIndices::IdleTime])
    {
        Er::addProperty<Er::ProcessesGlobal::IdleTime>(bag, idleAll / cpuCount);
    }

    if (required[Er::ProcessesGlobal::PropIndices::UserTime])
    {
        Er::addProperty<Er::ProcessesGlobal::UserTime>(bag, userAll / cpuCount);
    }

    if (required[Er::ProcessesGlobal::PropIndices::SystemTime])
    {
        Er::addProperty<Er::ProcessesGlobal::SystemTime>(bag, systemAll / cpuCount);
    }

    if (required[Er::ProcessesGlobal::PropIndices::VirtualTime])
    {
        Er::addProperty<Er::ProcessesGlobal::VirtualTime>(bag, virtAll / cpuCount);
    }

    if (required[Er::ProcessesGlobal::PropIndices::TotalTime])
    {
        Er::addProperty<Er::ProcessesGlobal::TotalTime>(bag, totalAll / cpuCount);
    }

    auto mem = m_procFs.readMemStats();

    if (required[Er::ProcessesGlobal::PropIndices::TotalMem])
        Er::addProperty<Er::ProcessesGlobal::TotalMem>(bag, mem.totalMem);

    if (required[Er::ProcessesGlobal::PropIndices::UsedMem])
        Er::addProperty<Er::ProcessesGlobal::UsedMem>(bag, mem.usedMem);

    if (required[Er::ProcessesGlobal::PropIndices::BuffersMem])
        Er::addProperty<Er::ProcessesGlobal::BuffersMem>(bag, mem.buffersMem);

    if (required[Er::ProcessesGlobal::PropIndices::CachedMem])
        Er::addProperty<Er::ProcessesGlobal::CachedMem>(bag, mem.cachedMem);

    if (required[Er::ProcessesGlobal::PropIndices::SharedMem])
        Er::addProperty<Er::ProcessesGlobal::SharedMem>(bag, mem.sharedMem);

    if (required[Er::ProcessesGlobal::PropIndices::AvailableMem])
        Er::addProperty<Er::ProcessesGlobal::AvailableMem>(bag, mem.availableMem);

    if (required[Er::ProcessesGlobal::PropIndices::TotalSwap])
        Er::addProperty<Er::ProcessesGlobal::TotalSwap>(bag, mem.totalSwap);

    if (required[Er::ProcessesGlobal::PropIndices::UsedSwap])
        Er::addProperty<Er::ProcessesGlobal::UsedSwap>(bag, mem.usedSwap);

    if (required[Er::ProcessesGlobal::PropIndices::CachedSwap])
        Er::addProperty<Er::ProcessesGlobal::CachedSwap>(bag, mem.cachedSwap);

    if (required[Er::ProcessesGlobal::PropIndices::ZSwapComp])
        Er::addProperty<Er::ProcessesGlobal::ZSwapComp>(bag, mem.zswapComp);

    if (required[Er::ProcessesGlobal::PropIndices::ZSwapOrig])
        Er::addProperty<Er::ProcessesGlobal::ZSwapOrig>(bag, mem.zswapOrig);

    Er::addProperty<Er::ProcessesGlobal::Global>(bag, true);

    return bag;
}

void ProcessList::dropStaleStreams() noexcept
{
    auto now = std::chrono::steady_clock::now();

    for (auto it = m_streams.begin(); it != m_streams.end();)
    {
        auto d = std::chrono::duration_cast<std::chrono::seconds>(now - it->second->touched);
        if (d.count() > kStreamTimeoutSeconds)
        {
            auto next = std::next(it);
            LogWarning(m_log, LogInstance("ProcessList"), "Dropping stale stream %d", it->first);
            m_streams.erase(it);
            it = next;
        }
        else
        {
            ++it;
        }
    }
}

void ProcessList::dropStaleSessions() noexcept
{
    auto now = std::chrono::steady_clock::now();

    for (auto it = m_sessions.begin(); it != m_sessions.end();)
    {
        auto d = std::chrono::duration_cast<std::chrono::seconds>(now - it->second->touched);
        if (d.count() > kSessionTimeoutSeconds)
        {
            auto next = std::next(it);
            LogWarning(m_log, LogInstance("ProcessList"), "Dropping stale session %d", it->first);
            m_sessions.erase(it);
            it = next;
        }
        else
        {
            ++it;
        }
    }
}

ProcessList::StreamId ProcessList::beginProcessStream(const Er::PropertyBag& args)
{
    auto pids = m_procFs.enumeratePids();
    auto required = getProcessPropMask(args);

    std::unique_lock l(m_mutexSession);
    
    auto streamId = m_nextStreamId++;
    auto stream = std::make_unique<ProcessListStream>(streamId, required, std::move(pids));
    m_streams.insert({ streamId, std::move(stream) });
#if 0
    LogDebug(m_log, LogInstance("ProcessList"), "Started process stream %d", streamId);
#endif
    return streamId;
}

Er::PropertyBag ProcessList::nextProcess(ProcessListStream* stream)
{
    if (stream->next >= stream->pids.size())
    {
        return Er::PropertyBag(); // end of stream
    }

    ProcessDetailsCached cached;
    auto bag = collectProcessDetails(m_procFs, stream->pids[stream->next], stream->required, Er::PropertyBag(), cached);

    if (stream->required[Er::ProcessProps::PropIndices::Icon])
    {
        if (m_iconManager)
        {
            addProcessIcon(cached.comm, cached.exe, m_iconManager, bag);
        }
    }
#if 0
    Er::Log::Debug(m_log, LogInstance("ProcessList")) << "Next PID " << stream->pids[stream->next] << " on stream " << stream->id;
#endif
    ++stream->next;

    return bag;
}

ProcessList::StreamId ProcessList::beginProcessDiffStream(const Er::PropertyBag& args, Session* session)
{
    auto required = getProcessPropMask(args);

    ProcessStatistics stats;
    auto diff = updateProcessCollection(m_procFs, m_iconManager, required, session->processes, stats);
    
    std::unique_lock l(m_mutexSession);

    auto streamId = m_nextStreamId++;

    auto stream = std::make_unique<ProcessListDiffStream>(streamId, required, std::move(diff));
    m_streams.insert({ streamId, std::move(stream) });

#if 0
    LogDebug(m_log, LogInstance("ProcessList"), "Started process diff stream %d", streamId);
#endif

    return streamId;
}

Er::PropertyBag ProcessList::nextProcessDiff(ProcessListDiffStream* stream, Session* session)
{
    Er::PropertyBag bag;

    if (stream->stage == ProcessListDiffStream::Stage::Globals)
    {
        bag = processesGlobal(Er::ProcessesGlobal::PropMask(0xffffffffffffffff, Er::ProcessesGlobal::PropMask::FromBits), stream->diff.processCount);
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

        Er::addProperty<Er::ProcessProps::Pid>(bag, stream->diff.removed[stream->next]);
        Er::addProperty<Er::ProcessProps::IsDeleted>(bag, true);
#if 0
        Er::Log::Debug(m_log, LogInstance("ProcessList")) << "Next removed PID " << stream->diff.removed[stream->next] << " on stream " << stream->id;
#endif
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
        for (auto& prop : modified.properties)
        {
            bag.insert({ prop.id, std::move(prop) });
        }

        Er::addProperty<Er::ProcessProps::Pid>(bag, modified.pid);
        Er::addProperty<Er::ProcessProps::Valid>(bag, true);

#if 0
        Er::Log::Debug(m_log, LogInstance("ProcessList")) << "Next modified PID " << modified.pid << " on stream " << stream->id;
#endif
        
        ++stream->next;
    }
    else
    {
        // pack next added process
        if (stream->next >= stream->diff.added.size())
            return Er::PropertyBag(); // end of stream

        auto added = stream->diff.added[stream->next];
        if (added->isNew)
            Er::addProperty<Er::ProcessProps::IsNew>(bag, true);

        for (auto& prop: added->properties)
        {
            bag.insert({ prop.first, prop.second });
        }

        assert(bag.find(Er::ProcessProps::Pid::Id::value) != bag.end());
        assert(bag.find(Er::ProcessProps::Valid::Id::value) != bag.end());

#if 0
        if (added->isNew)
            Er::Log::Debug(m_log, LogInstance("ProcessList")) << "Next new PID " << added->pid << " on stream " << stream->id;
        else
            Er::Log::Debug(m_log, LogInstance("ProcessList")) << "Next existing PID " << added->pid << " on stream " << stream->id;
#endif
        
        ++stream->next;
    }
    

    return bag;
}

} // namespace Private {}

} // namespace Er {}