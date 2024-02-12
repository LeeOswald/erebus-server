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

}

ProcessList::ProcessList(Er::Log::ILog* log)
    : m_log(log)
    , m_procFs(log)
{

}

Er::PropertyBag ProcessList::request(const std::string& request, const Er::PropertyBag& args)
{
    if (request == Er::ProcessRequests::ProcessDetails)
        return processDetails(args);

    throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported request %s", request.c_str()));
}

ProcessList::StreamId ProcessList::beginStream(const std::string& request, const Er::PropertyBag& args)
{
    return 0;
}

void ProcessList::endStream(StreamId id)
{

}

Er::PropertyBag ProcessList::next(StreamId id)
{
    Er::PropertyBag bag;

    return bag;
}

Er::PropertyBag ProcessList::processDetails(const Er::PropertyBag& args)
{
    auto it = args.find(Er::ProcessProps::Pid::Id::value);
    if (it == args.end())
        throw Er::Exception(ER_HERE(), "No process.pid field in ProcessDetails request");

    auto pid = std::any_cast<Er::ProcessProps::Pid::ValueType>(it->second.value);

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
        bag.insert({ Er::ProcessProps::Tpgid::Id::value, Er::Property(Er::ProcessProps::Tpgid::Id::value, stat.tpgid) });
        bag.insert({ Er::ProcessProps::Session::Id::value, Er::Property(Er::ProcessProps::Session::Id::value, stat.session) });
        bag.insert({ Er::ProcessProps::Ruid::Id::value, Er::Property(Er::ProcessProps::Ruid::Id::value, stat.ruid) });
        bag.insert({ Er::ProcessProps::StatComm::Id::value, Er::Property(Er::ProcessProps::StatComm::Id::value, stat.comm) });

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

} // namespace Private {}

} // namespace Er {}