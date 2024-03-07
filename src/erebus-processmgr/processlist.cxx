#include "iconmanager.hxx"
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

ProcessList::ProcessList(Er::Log::ILog* log, IconManager* iconManager)
    : m_log(log)
    , m_iconManager(iconManager)
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

ProcessList::Session* ProcessList::getSession(std::optional<SessionId> id)
{
    if (!id)
        throw Er::Exception(ER_HERE(), "Session not specified");

    std::unique_lock l(m_mutex);

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
        auto required = getPropMask(args);
        return processDetails(args, required);
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
    Stream* stream = nullptr;
    {
        std::unique_lock l(m_mutex);
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
    auto required = getPropMask(args);

    std::unique_lock l(m_mutex);

    auto streamId = m_nextStreamId++;
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
    if (stream->required[Er::ProcessProps::PropIndices::Icon])
    {
        if (m_iconManager)
        {
            addProcessIcon(m_iconManager, bag);
        }
    }

    Er::Log::Debug(m_log, LogInstance("ProcessList")) << "Next PID " << stream->pids[stream->next] << " on stream " << stream->id;

    ++stream->next;

    return bag;
}

ProcessList::StreamId ProcessList::beginProcessDiffStream(const Er::PropertyBag& args, Session* session)
{
    auto required = getPropMask(args);
    auto diff = updateProcessCollection(m_procFs, required, session->processes);
    
    if (required[Er::ProcessProps::PropIndices::Icon] && m_iconManager)
    {
        for (auto& process: session->processes.processes)
        {
            auto& processProps = process.second->properties;
            addProcessIcon(m_iconManager, processProps);
        }
    }

    std::unique_lock l(m_mutex);

    auto streamId = m_nextStreamId++;

    auto stream = std::make_unique<ProcessListDiffStream>(streamId, required, std::move(diff));
    m_streams.insert({ streamId, std::move(stream) });

    LogDebug(m_log, LogInstance("ProcessList"), "Started process diff stream %d", streamId);

    return streamId;
}

Er::PropertyBag ProcessList::nextProcessDiff(ProcessListDiffStream* stream, Session* session)
{
    Er::PropertyBag bag;

    if (stream->stage == ProcessListDiffStream::Stage::Removed)
    {
        // pack next removed process
        if (stream->next >= stream->diff.removed.size())
        {
            stream->stage = ProcessListDiffStream::Stage::Modified;
            stream->next = 0;
            return nextProcessDiff(stream, session);
        }

        bag.insert({ Er::ProcessProps::Pid::Id::value, Er::Property(Er::ProcessProps::Pid::Id::value, stream->diff.removed[stream->next]) });
        bag.insert({ Er::ProcessProps::IsDeleted::Id::value, Er::Property(Er::ProcessProps::IsDeleted::Id::value, true) }); 

        Er::Log::Debug(m_log, LogInstance("ProcessList")) << "Next removed PID " << stream->diff.removed[stream->next] << " on stream " << stream->id;
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

        bag.insert({ Er::ProcessProps::Pid::Id::value, Er::Property(Er::ProcessProps::Pid::Id::value, modified.pid) });
        bag.insert({ Er::ProcessProps::Valid::Id::value, Er::Property(Er::ProcessProps::Valid::Id::value, true) }); 

        Er::Log::Debug(m_log, LogInstance("ProcessList")) << "Next modified PID " << modified.pid << " on stream " << stream->id;
    }
    else
    {
        // pack next added process
        if (stream->next >= stream->diff.added.size())
            return Er::PropertyBag(); // end of stream

        auto added = stream->diff.added[stream->next];
        if (added->isNew)
            bag.insert({ Er::ProcessProps::IsNew::Id::value, Er::Property(Er::ProcessProps::IsNew::Id::value, true) }); 

        for (auto& prop: added->properties)
        {
            bag.insert({ prop.first, prop.second });
        }

        assert(bag.find(Er::ProcessProps::Pid::Id::value) != bag.end());
        assert(bag.find(Er::ProcessProps::Valid::Id::value) != bag.end());

        if (added->isNew)
            Er::Log::Debug(m_log, LogInstance("ProcessList")) << "Next new PID " << added->pid << " on stream " << stream->id;
        else
            Er::Log::Debug(m_log, LogInstance("ProcessList")) << "Next existing PID " << added->pid << " on stream " << stream->id;
    }
    
    ++stream->next;

    return bag;
}

} // namespace Private {}

} // namespace Er {}