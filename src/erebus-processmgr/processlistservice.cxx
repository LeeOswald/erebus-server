#include "processlistservice.hxx"

#include <erebus/exception.hxx>
#include <erebus/util/format.hxx>


namespace Erp
{

namespace ProcessMgr
{


ProcessListService::~ProcessListService()
{
}

ProcessListService::ProcessListService(Er::Log::ILog* log)
    : m_log(log)
    , m_procFs(log)
    , m_globalsCollector(m_log, m_procFs)
{
}

void ProcessListService::registerService(Er::Server::IServiceContainer* container)
{
    container->registerService(Er::ProcessMgr::ProcessRequests::ListProcessesDiff, this);
    container->registerService(Er::ProcessMgr::ProcessRequests::ProcessesGlobal, this);
}

void ProcessListService::unregisterService(Er::Server::IServiceContainer* container)
{
    container->unregisterService(this);
}

ProcessListService::SessionId ProcessListService::allocateSession()
{
    std::unique_lock l(m_mutex);
    auto id = m_nextSessionId++;

    m_sessions.insert({ id, std::make_shared<Session>(id) });

    return id;
}

void ProcessListService::deleteSession(SessionId id)
{
    std::unique_lock l(m_mutex);

    auto it = m_sessions.find(id);
    if (it == m_sessions.end())
        throw Er::Exception(ER_HERE(), Er::Util::format("Non-existent session %d", id));

    m_sessions.erase(it);

    dropStaleSessions();
}

std::shared_ptr<ProcessListService::Session> ProcessListService::getSession(SessionId id)
{
    std::unique_lock l(m_mutex);

    auto it = m_sessions.find(id);
    if (it == m_sessions.end())
        throw Er::Exception(ER_HERE(), "Session not found");

    {
        std::unique_lock l(it->second->mutex);
        it->second->touched = std::chrono::steady_clock::now();
    }

    return it->second;
}

Er::PropertyBag ProcessListService::request(std::string_view request, const Er::PropertyBag& args, SessionId sessionId)
{
    auto session = getSession(sessionId);

    if (request == Er::ProcessMgr::ProcessRequests::ProcessesGlobal)
    {
        auto required = getProcessesGlobalPropMask(args);
        return processesGlobal(session.get(), required, std::nullopt);
    }

    throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported request %s", std::string(request).c_str()));
}

ProcessListService::StreamId ProcessListService::beginStream(std::string_view request, const Er::PropertyBag& args, SessionId sessionId)
{
    auto session = getSession(sessionId);

    if (request == Er::ProcessMgr::ProcessRequests::ListProcessesDiff)
        return beginProcessDiffStream(args, session.get());

    throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported request %s", std::string(request).c_str()));
}

void ProcessListService::endStream(StreamId id, SessionId sessionId)
{
    auto session = getSession(sessionId);

    std::unique_lock l(session->mutex);

    auto it = session->streams.find(id);
    if (it == session->streams.end())
        throw Er::Exception(ER_HERE(), Er::Util::format("Non-existent stream %d:%d", sessionId, id));

    session->streams.erase(it);

    dropStaleStreams(session.get());
}

Er::PropertyBag ProcessListService::next(StreamId id, SessionId sessionId)
{
    auto session = getSession(sessionId);

    std::shared_ptr<Stream> stream;
    {
        std::unique_lock l(session->mutex);
        auto it = session->streams.find(id);
        if (it == session->streams.end())
            throw Er::Exception(ER_HERE(), Er::Util::format("Non-existent stream %d:%d", sessionId, id));

        stream = it->second;

        stream->touched = std::chrono::steady_clock::now();
    }

    if (stream->type == Stream::Type::ProcessListDiff)
        return nextProcessDiff(static_cast<ProcessListDiffStream*>(stream.get()), session.get());

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

Er::PropertyBag ProcessListService::processesGlobal(Session* session, Er::ProcessMgr::ProcessesGlobal::PropMask required, std::optional<uint64_t> processCount)
{
    auto bag = m_globalsCollector.collect(required);
    
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

    if (required[Er::ProcessMgr::ProcessesGlobal::PropIndices::ProcessCount])
    {
        Er::addProperty<Er::ProcessMgr::ProcessesGlobal::ProcessCount>(bag, *processCount);
    }

    return bag;
}

void ProcessListService::dropStaleStreams(Session* session) noexcept
{
    auto now = std::chrono::steady_clock::now();

    for (auto it = session->streams.begin(); it != session->streams.end();)
    {
        auto d = std::chrono::duration_cast<std::chrono::seconds>(now - it->second->touched);
        if (d.count() > kStreamTimeoutSeconds)
        {
            auto next = std::next(it);
            ErLogWarning(m_log, "Dropping stale stream %d", it->first);
            session->streams.erase(it);
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

ProcessListService::StreamId ProcessListService::beginProcessDiffStream(const Er::PropertyBag& args, Session* session)
{
    auto requiredProcess = getProcessPropMask(args);
    if (!session->collector)
        session->collector.reset(new ProcessListCollector(m_log, m_procFs));
        
    auto processesDiff = session->collector->update(requiredProcess);

    auto requiredGlobals = getProcessesGlobalPropMask(args);
    auto globals = processesGlobal(session, requiredGlobals, processesDiff.processCount);

    std::unique_lock l(session->mutex);

    auto streamId = session->nextStreamId++;

    auto stream = std::make_unique<ProcessListDiffStream>(streamId, std::move(globals), std::move(processesDiff));
    session->streams.insert({ streamId, std::move(stream) });

    return streamId;
}

Er::PropertyBag ProcessListService::nextProcessDiff(ProcessListDiffStream* stream, Session* session)
{
    Er::PropertyBag bag;

    if (stream->stage == ProcessListDiffStream::Stage::Globals)
    {
        bag = std::move(stream->globals);
        stream->stage = ProcessListDiffStream::Stage::Removed;
    }
    else if (stream->stage == ProcessListDiffStream::Stage::Removed)
    {
        // pack next removed process
        if (stream->processes.firstRun || (stream->next >= stream->processes.removed.size()))
        {
            stream->stage = ProcessListDiffStream::Stage::Modified;
            stream->next = 0;
            return nextProcessDiff(stream, session);
        }

        Er::addProperty<Er::ProcessMgr::ProcessProps::Pid>(bag, stream->processes.removed[stream->next]->pid);
        Er::addProperty<Er::ProcessMgr::ProcessProps::IsDeleted>(bag, true);

        ++stream->next;
    }
    else if (stream->stage == ProcessListDiffStream::Stage::Modified)
    {
        // pack next modified process
        if (stream->processes.firstRun || (stream->next >= stream->processes.modified.size()))
        {
            stream->stage = ProcessListDiffStream::Stage::Added;
            stream->next = 0;
            return nextProcessDiff(stream, session);
        }

        auto& modified = stream->processes.modified[stream->next];
        Er::enumerateProperties(modified.properties, [&bag](Er::Property& prop)
        {
            Er::addProperty(bag, std::move(prop)); // we can move from 'modified'
        });

        Er::addProperty<Er::ProcessMgr::ProcessProps::Pid>(bag, modified.process->pid);
        Er::addProperty<Er::ProcessMgr::ProcessProps::Valid>(bag, true);

        ++stream->next;
    }
    else
    {
        // pack next added process
        if (stream->next >= stream->processes.added.size())
            return Er::PropertyBag(); // end of stream

        auto added = stream->processes.added[stream->next];
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