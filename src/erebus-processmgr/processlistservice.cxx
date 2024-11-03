#include "processlistservice.hxx"

#include <erebus/exception.hxx>


namespace Erp::ProcessMgr
{


ProcessListService::~ProcessListService()
{
}

ProcessListService::ProcessListService(Er::Log::ILog* log)
    : m_log(log)
    , m_procFs(log)
    , m_globalsCollector(m_log, m_procFs)
    , m_sessions(std::chrono::seconds(60))
{
}

void ProcessListService::registerService(Er::Server::IServer* container)
{
    container->registerService(Er::ProcessMgr::Requests::ListProcessesDiff, shared_from_this());
    container->registerService(Er::ProcessMgr::Requests::GlobalProps, shared_from_this());
}

void ProcessListService::unregisterService(Er::Server::IServer* container)
{
    container->unregisterService(this);
}

Er::PropertyBag ProcessListService::request(std::string_view request, std::string_view cookie, const Er::PropertyBag& args)
{
    if (request == Er::ProcessMgr::Requests::GlobalProps)
    {
        auto required = getProcessesGlobalPropMask(args);
        return processesGlobal(required, std::nullopt);
    }

    ErThrow(Er::format("Unsupported request {}", request));
}

ProcessListService::StreamId ProcessListService::beginStream(std::string_view request, std::string_view cookie, const Er::PropertyBag& args)
{
    auto session = m_sessions.allocate(std::string(cookie));
    if (!session)
        ErThrow(Er::format("Cookie {} is in use", cookie));

    if (request == Er::ProcessMgr::Requests::ListProcessesDiff)
        return beginProcessDiffStream(std::move(session), args);

    ErThrow(Er::format("Unsupported request {}", request));
}

void ProcessListService::endStream(StreamId id)
{
    auto stream = reinterpret_cast<Stream*>(id);
    delete stream;
}

Er::PropertyBag ProcessListService::next(StreamId id)
{
    auto stream = reinterpret_cast<Stream*>(id);

    if (stream->type == Stream::Type::ProcessListDiff)
        return nextProcessDiff(static_cast<ProcessListDiffStream*>(stream));

    ErAssert(!"Unknown stream type");
    return Er::PropertyBag();
}

Er::ProcessMgr::ProcessProps::PropMask ProcessListService::getProcessPropMask(const Er::PropertyBag& args)
{
    // default mask is 'everything included'
    auto mask = Er::getPropertyValueOr<Er::ProcessMgr::ProcessProps::RequiredFields>(args, 0xffffffffffffffff);
    return Er::ProcessMgr::ProcessProps::PropMask(mask, Er::ProcessMgr::ProcessProps::PropMask::FromBits);
}

Er::ProcessMgr::GlobalProps::PropMask ProcessListService::getProcessesGlobalPropMask(const Er::PropertyBag& args)
{
    // default mask is 'everything included'
    auto mask = Er::getPropertyValueOr<Er::ProcessMgr::GlobalProps::RequiredFields>(args, 0xffffffffffffffff);
    return Er::ProcessMgr::GlobalProps::PropMask(mask, Er::ProcessMgr::GlobalProps::PropMask::FromBits);
}

Er::PropertyBag ProcessListService::processesGlobal(Er::ProcessMgr::GlobalProps::PropMask required, std::optional<uint64_t> processCount)
{
    auto bag = m_globalsCollector.collect(required);
    
    if (!processCount)
    {
        if (required[Er::ProcessMgr::GlobalProps::PropIndices::ProcessCount])
        {
            auto pids = m_procFs.enumeratePids();
            processCount = pids.size();
        }
        else
        {
            processCount = 0;
        }
    }

    if (required[Er::ProcessMgr::GlobalProps::PropIndices::ProcessCount])
    {
        Er::addProperty<Er::ProcessMgr::GlobalProps::ProcessCount>(bag, *processCount);
    }

    return bag;
}

ProcessListService::StreamId ProcessListService::beginProcessDiffStream(SessionRef&& session, const Er::PropertyBag& args)
{
    if (!session.get().collector)
        session.get().collector.reset(new ProcessListCollector(m_log, m_procFs));

    auto requiredProcess = getProcessPropMask(args);
    auto processesDiff = session.get().collector->update(requiredProcess);

    auto requiredGlobals = getProcessesGlobalPropMask(args);
    auto globals = processesGlobal(requiredGlobals, processesDiff.processCount);

    return reinterpret_cast<StreamId>(new ProcessListDiffStream(std::move(session), std::move(globals), std::move(processesDiff)));
}

Er::PropertyBag ProcessListService::nextProcessDiff(ProcessListDiffStream* stream)
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
            return nextProcessDiff(stream);
        }

        Er::addProperty<Er::ProcessMgr::Props::Pid>(bag, stream->processes.removed[stream->next]->pid);
        Er::addProperty<Er::ProcessMgr::Props::IsDeleted>(bag, Er::True);

        ++stream->next;
    }
    else if (stream->stage == ProcessListDiffStream::Stage::Modified)
    {
        // pack next modified process
        if (stream->processes.firstRun || (stream->next >= stream->processes.modified.size()))
        {
            stream->stage = ProcessListDiffStream::Stage::Added;
            stream->next = 0;
            return nextProcessDiff(stream);
        }

        auto& modified = stream->processes.modified[stream->next];
        Er::enumerateProperties(modified.properties, [&bag](Er::Property& prop)
        {
            Er::addProperty(bag, std::move(prop)); // we can move from 'modified'
        });

        Er::addProperty<Er::ProcessMgr::Props::Pid>(bag, modified.process->pid);
        Er::addProperty<Er::ProcessMgr::Props::Valid>(bag, Er::True);

        ++stream->next;
    }
    else
    {
        // pack next added process
        if (stream->next >= stream->processes.added.size())
            return Er::PropertyBag(); // end of stream

        auto added = stream->processes.added[stream->next];
        if (added->isNew)
            Er::addProperty<Er::ProcessMgr::Props::IsNew>(bag, Er::True);

        Er::enumerateProperties(added->properties, [&bag](const Er::Property& prop)
        {
            Er::addProperty(bag, prop);
        });

        ErAssert(Er::propertyPresent<Er::ProcessMgr::Props::Pid>(bag));
        ErAssert(Er::propertyPresent<Er::ProcessMgr::Props::Valid>(bag));
        
        ++stream->next;
    }
    

    return bag;
}


} // namespace Erp::ProcessMgr {}