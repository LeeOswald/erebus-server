#include "processlist.hxx"

#include <erebus/exception.hxx>
#include <erebus/util/format.hxx>



namespace Er
{

namespace Private
{

ProcessList::~ProcessList()
{
    LogDebug(m_log, LogInstance("ProcessList"), "~ProcessList()");
}

ProcessList::ProcessList(Er::Log::ILog* log)
    : m_log(log)
    , m_procFs(log)
{
    LogDebug(m_log, LogInstance("ProcessList"), "ProcessList()");
}

ProcessList::SessionId ProcessList::allocateSession()
{
    std::unique_lock l(m_mutex);
    auto id = m_nextSessionId++;

    m_sessions.insert({ id, std::make_unique<Session>(id) });

    LogDebug(m_log, LogInstance("ProcessList"), "Started session %d", id);

    return id;
}

void ProcessList::deleteSession(SessionId id)
{
    std::unique_lock l(m_mutex);

    auto it = m_sessions.find(id);
    if (it == m_sessions.end())
        throw Er::Exception(ER_HERE(), Er::Util::format("Non-existent session %d", id));

    m_sessions.erase(it);

    dropStaleSessions();

    LogDebug(m_log, LogInstance("ProcessList"), "Ended session %d", id);    
}

ProcessList::Session* ProcessList::getSesstion(std::optional<SessionId> id) noexcept
{
    if (!id)
        return nullptr;

    std::shared_lock l(m_mutex);

    auto it = m_sessions.find(*id);
    if (it == m_sessions.end())
        return nullptr;

    it->second->touched = std::chrono::steady_clock::now();

    return it->second.get();
}

Er::PropertyBag ProcessList::request(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId)
{
    if (request == Er::ProcessRequests::ProcessDetails)
    {
        auto required = getPropMask(args);
        return processDetails(args, required);
    }

    throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported request %s", std::string(request).c_str()));
}

ProcessList::StreamId ProcessList::beginStream(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId)
{
    if (request == Er::ProcessRequests::ListProcesses)
        return beginProcessStream(args);

    throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported request %s", std::string(request).c_str()));
}

void ProcessList::endStream(StreamId id, std::optional<SessionId> sessionId)
{
    std::unique_lock l(m_mutex);

    auto it = m_streams.find(id);
    if (it == m_streams.end())
        throw Er::Exception(ER_HERE(), Er::Util::format("Non-existent stream %d", id));

    m_streams.erase(it);

    dropStaleStreams();

    LogDebug(m_log, LogInstance("ProcessList"), "Ended stream %d", id);    
}

Er::PropertyBag ProcessList::next(StreamId id, std::optional<SessionId> sessionId)
{
    std::shared_lock l(m_mutex);
    auto it = m_streams.find(id);
    if (it == m_streams.end())
        throw Er::Exception(ER_HERE(), Er::Util::format("Non-existent stream %d", id));

    it->second->touched = std::chrono::steady_clock::now();

    if (it->second->type == StreamType::ProcessList)
        return nextProcess(static_cast<ProcessListStream*>(it->second.get()));

    assert(!"Unknown stream type");
    return Er::PropertyBag();
}

Er::ProcessProps::PropMask ProcessList::getPropMask(const Er::PropertyBag& args)
{
    auto it = args.find(Er::ProcessProps::RequiredFields::Id::value);
    if (it == args.end())
    {
        // default mask - everything included
        return Er::ProcessProps::PropMask(0xffffffffffffffff, Er::ProcessProps::PropMask::FromBits);
    }

    auto mask = std::any_cast<uint64_t>(it->second.value);
    return Er::ProcessProps::PropMask(mask, Er::ProcessProps::PropMask::FromBits);
}

Er::PropertyBag ProcessList::processDetails(const Er::PropertyBag& args, Er::ProcessProps::PropMask required)
{
    auto it = args.find(Er::ProcessProps::Pid::Id::value);
    if (it == args.end())
        throw Er::Exception(ER_HERE(), "No process.pid field in ProcessDetails request");

    auto pid = std::any_cast<Er::ProcessProps::Pid::ValueType>(it->second.value);
    if (pid == ProcFs::KernelPid)
        return collectKernelDetails(m_procFs, required);
        
    return collectProcessDetails(m_procFs, pid, required);
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

    std::unique_lock l(m_mutex);

    auto streamId = m_nextStreamId++;

    auto required = getPropMask(args);

    auto stream = std::make_unique<ProcessListStream>(streamId, required, std::move(pids));
    m_streams.insert({ streamId, std::move(stream) });

    LogDebug(m_log, LogInstance("ProcessList"), "Started process stream %d", streamId);

    return streamId;
}

Er::PropertyBag ProcessList::nextProcess(ProcessListStream* stream)
{
    if (stream->next >= stream->pids.size())
        return Er::PropertyBag(); // end of stream

    auto bag = collectProcessDetails(m_procFs, stream->pids[stream->next], stream->required);

    Er::Log::Debug(m_log, LogInstance("ProcessList")) << "Next PID " << stream->pids[stream->next] << " on stream " << stream->id;

    ++stream->next;

    return bag;
}

} // namespace Private {}

} // namespace Er {}