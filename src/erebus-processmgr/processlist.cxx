#include "processlist.hxx"

#include <erebus/exception.hxx>
#include <erebus/util/format.hxx>
#include <erebus-processmgr/processprops.hxx>


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

Er::PropertyBag ProcessList::request(const std::string& request, const Er::PropertyBag& args)
{
    if (request == Er::ProcessRequests::ProcessDetails)
        return processDetails(args);

    throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported request %s", request.c_str()));
}

ProcessList::StreamId ProcessList::beginStream(const std::string& request, const Er::PropertyBag& args)
{
    if (request == Er::ProcessRequests::ListProcesses)
        return beginProcessStream(args);

    throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported request %s", request.c_str()));
}

void ProcessList::endStream(StreamId id)
{
    {
        std::lock_guard l(m_mutex);
        auto it = m_streams.find(id);
        if (it == m_streams.end())
            throw Er::Exception(ER_HERE(), Er::Util::format("Non-existent stream %d", id));

        m_streams.erase(it);
        LogDebug(m_log, LogInstance("ProcessList"), "Ended stream %d", id);    
    }

    dropStaleStreams();
}

Er::PropertyBag ProcessList::next(StreamId id)
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

Er::PropertyBag ProcessList::processDetails(const Er::PropertyBag& args)
{
    auto it = args.find(Er::ProcessProps::Pid::Id::value);
    if (it == args.end())
        throw Er::Exception(ER_HERE(), "No process.pid field in ProcessDetails request");

    auto pid = std::any_cast<Er::ProcessProps::Pid::ValueType>(it->second.value);

    if (pid == ProcFs::KernelPid)
        return kernelDetails();
        
    return processDetails(pid);
}

Er::PropertyBag ProcessList::processDetails(uint64_t pid)
{
    Er::PropertyBag bag;

    auto stat = m_procFs.readStat(pid);
    if (!stat.valid)
    {
        bag.insert({ Er::ProcessProps::Valid::Id::value, Er::Property(Er::ProcessProps::Valid::Id::value, false) });
        bag.insert({ Er::ProcessProps::Error::Id::value, Er::Property(Er::ProcessProps::Error::Id::value, std::move(stat.error)) });
    }
    else
    {
        bag.insert({ Er::ProcessProps::Valid::Id::value, Er::Property(Er::ProcessProps::Valid::Id::value, true) });
        bag.insert({ Er::ProcessProps::Pid::Id::value, Er::Property(Er::ProcessProps::Pid::Id::value, stat.pid) });
        bag.insert({ Er::ProcessProps::PPid::Id::value, Er::Property(Er::ProcessProps::PPid::Id::value, stat.ppid) });
        bag.insert({ Er::ProcessProps::PGrp::Id::value, Er::Property(Er::ProcessProps::PGrp::Id::value, stat.pgrp) });

        if (stat.tpgid != std::numeric_limits<uint64_t>::max())
            bag.insert({ Er::ProcessProps::Tpgid::Id::value, Er::Property(Er::ProcessProps::Tpgid::Id::value, stat.tpgid) });
        
        bag.insert({ Er::ProcessProps::Session::Id::value, Er::Property(Er::ProcessProps::Session::Id::value, stat.session) });
        bag.insert({ Er::ProcessProps::Ruid::Id::value, Er::Property(Er::ProcessProps::Ruid::Id::value, stat.ruid) });
        bag.insert({ Er::ProcessProps::StatComm::Id::value, Er::Property(Er::ProcessProps::StatComm::Id::value, std::move(stat.comm)) });
        bag.insert({ Er::ProcessProps::StartTime::Id::value, Er::Property(Er::ProcessProps::StartTime::Id::value, stat.startTime) });
        
        std::string state({ stat.state });
        bag.insert({ Er::ProcessProps::State::Id::value, Er::Property(Er::ProcessProps::State::Id::value, std::move(state)) });

        auto comm = m_procFs.readComm(pid);
        if (!comm.empty())
            bag.insert({ Er::ProcessProps::Comm::Id::value, Er::Property(Er::ProcessProps::Comm::Id::value, std::move(comm)) });

        auto cmdLine = m_procFs.readCmdLine(pid);
        if (!cmdLine.empty())
            bag.insert({ Er::ProcessProps::CmdLine::Id::value, Er::Property(Er::ProcessProps::CmdLine::Id::value, std::move(cmdLine)) });

        auto exe = m_procFs.readExePath(pid);
        if (!exe.empty())
            bag.insert({ Er::ProcessProps::Exe::Id::value, Er::Property(Er::ProcessProps::Exe::Id::value, std::move(exe)) });
    }

    return bag;
}

Er::PropertyBag ProcessList::kernelDetails()
{
    Er::PropertyBag bag;

    bag.insert({ Er::ProcessProps::Valid::Id::value, Er::Property(Er::ProcessProps::Valid::Id::value, true) });
    bag.insert({ Er::ProcessProps::Pid::Id::value, Er::Property(Er::ProcessProps::Pid::Id::value, ProcFs::KernelPid) });
    
    auto bootTime = m_procFs.getBootTime();
    bag.insert({ Er::ProcessProps::StartTime::Id::value, Er::Property(Er::ProcessProps::StartTime::Id::value, bootTime) });

    auto cmdLine = m_procFs.readCmdLine(ProcFs::KernelPid);
    if (!cmdLine.empty())
        bag.insert({ Er::ProcessProps::CmdLine::Id::value, Er::Property(Er::ProcessProps::CmdLine::Id::value, std::move(cmdLine)) });


    return bag;
}

void ProcessList::dropStaleStreams() noexcept
{
    auto now = std::chrono::steady_clock::now();

    std::lock_guard l(m_mutex);
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

ProcessList::StreamId ProcessList::beginProcessStream(const Er::PropertyBag& args)
{
    auto pids = m_procFs.enumeratePids();

    std::lock_guard l(m_mutex);

    auto streamId = m_nextStreamId++;

    auto stream = std::make_unique<ProcessListStream>(streamId, std::move(pids));
    m_streams.insert({ streamId, std::move(stream) });

    LogDebug(m_log, LogInstance("ProcessList"), "Started process stream %d", streamId);

    return streamId;
}

Er::PropertyBag ProcessList::nextProcess(ProcessListStream* stream)
{
    if (stream->next >= stream->pids.size())
        return Er::PropertyBag(); // end of stream

    auto bag = processDetails(stream->pids[stream->next]);

    Er::Log::Debug(m_log, LogInstance("ProcessList")) << "Next PID " << stream->pids[stream->next] << " on stream " << stream->id;

    ++stream->next;

    return bag;
}

} // namespace Private {}

} // namespace Er {}