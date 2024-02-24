#include "processlistdiff.hxx"

#include <erebus/exception.hxx>
#include <erebus/util/format.hxx>

namespace Er
{

namespace Private
{

Er::PropertyBag collectProcessDetails(Er::ProcFs::ProcFs& source, uint64_t pid, Er::ProcessProps::PropMask required)
{
    Er::PropertyBag bag;

    auto stat = source.readStat(pid);
    assert(stat.pid != ProcFs::InvalidPid); // PID is always valid

    if (!stat.valid)
    {
        bag.insert({ Er::ProcessProps::Pid::Id::value, Er::Property(Er::ProcessProps::Pid::Id::value, stat.pid) }); 
        bag.insert({ Er::ProcessProps::Valid::Id::value, Er::Property(Er::ProcessProps::Valid::Id::value, false) });
        bag.insert({ Er::ProcessProps::Error::Id::value, Er::Property(Er::ProcessProps::Error::Id::value, std::move(stat.error)) });
    }
    else
    {
        bag.insert({ Er::ProcessProps::Valid::Id::value, Er::Property(Er::ProcessProps::Valid::Id::value, true) });
        bag.insert({ Er::ProcessProps::Pid::Id::value, Er::Property(Er::ProcessProps::Pid::Id::value, stat.pid) });
        bag.insert({ Er::ProcessProps::PPid::Id::value, Er::Property(Er::ProcessProps::PPid::Id::value, stat.ppid) });
        
        if (required[Er::ProcessProps::PropIndices::PGrp])
            bag.insert({ Er::ProcessProps::PGrp::Id::value, Er::Property(Er::ProcessProps::PGrp::Id::value, stat.pgrp) });

        if (required[Er::ProcessProps::PropIndices::Tpgid])
            if (stat.tpgid != std::numeric_limits<uint64_t>::max())
                bag.insert({ Er::ProcessProps::Tpgid::Id::value, Er::Property(Er::ProcessProps::Tpgid::Id::value, stat.tpgid) });
        
        if (required[Er::ProcessProps::PropIndices::Session])
            bag.insert({ Er::ProcessProps::Session::Id::value, Er::Property(Er::ProcessProps::Session::Id::value, stat.session) });
        
        if (required[Er::ProcessProps::PropIndices::Ruid])
            bag.insert({ Er::ProcessProps::Ruid::Id::value, Er::Property(Er::ProcessProps::Ruid::Id::value, stat.ruid) });
        
        if (required[Er::ProcessProps::PropIndices::StartTime])
            bag.insert({ Er::ProcessProps::StartTime::Id::value, Er::Property(Er::ProcessProps::StartTime::Id::value, stat.startTime) });
        
        if (required[Er::ProcessProps::PropIndices::State])
        {
            std::string state({ stat.state });
            bag.insert({ Er::ProcessProps::State::Id::value, Er::Property(Er::ProcessProps::State::Id::value, std::move(state)) });
        }

        if (required[Er::ProcessProps::PropIndices::Comm])
        {
            auto comm = source.readComm(pid);
            if (!comm.empty())
                bag.insert({ Er::ProcessProps::Comm::Id::value, Er::Property(Er::ProcessProps::Comm::Id::value, std::move(comm)) });
        }

        if (required[Er::ProcessProps::PropIndices::CmdLine])
        {
            auto cmdLine = source.readCmdLine(pid);
            if (!cmdLine.empty())
                bag.insert({ Er::ProcessProps::CmdLine::Id::value, Er::Property(Er::ProcessProps::CmdLine::Id::value, std::move(cmdLine)) });
        }

        if (required[Er::ProcessProps::PropIndices::Exe])
        {
            auto exe = source.readExePath(pid);
            if (!exe.empty())
                bag.insert({ Er::ProcessProps::Exe::Id::value, Er::Property(Er::ProcessProps::Exe::Id::value, std::move(exe)) });
        }
    }

    return bag;
}

Er::PropertyBag collectKernelDetails(Er::ProcFs::ProcFs& source, Er::ProcessProps::PropMask required)
{
    Er::PropertyBag bag;

    bag.insert({ Er::ProcessProps::Valid::Id::value, Er::Property(Er::ProcessProps::Valid::Id::value, true) });
    bag.insert({ Er::ProcessProps::Pid::Id::value, Er::Property(Er::ProcessProps::Pid::Id::value, ProcFs::KernelPid) });
    
    if (required[Er::ProcessProps::PropIndices::StartTime])
    {
        auto bootTime = source.getBootTime();
        bag.insert({ Er::ProcessProps::StartTime::Id::value, Er::Property(Er::ProcessProps::StartTime::Id::value, bootTime) });
    }

    if (required[Er::ProcessProps::PropIndices::CmdLine])
    {
        auto cmdLine = source.readCmdLine(ProcFs::KernelPid);
        if (!cmdLine.empty())
            bag.insert({ Er::ProcessProps::CmdLine::Id::value, Er::Property(Er::ProcessProps::CmdLine::Id::value, std::move(cmdLine)) });
    }

    return bag;
}

std::unique_ptr<ProcessCollection> gatherProcessCollection(Er::ProcFs::ProcFs& source, Er::ProcessProps::PropMask required)
{
    auto now = std::chrono::steady_clock::now();

    auto list = std::make_unique<ProcessCollection>();
    
    auto kernel = collectKernelDetails(source, required);
    Er::cachePropertyInfo(kernel);

    list->processes.insert({ Er::ProcFs::KernelPid, std::make_unique<ProcessData>(now, std::move(kernel)) });

    auto pids = source.enumeratePids();
    for (auto pid: pids)
    {
        auto process = collectProcessDetails(source, pid, required);
        Er::cachePropertyInfo(process);

        list->processes.insert({ pid, std::make_unique<ProcessData>(now, std::move(process)) });
    }    

    return list;
}

PropertyRefs diffProcessData(const Er::PropertyBag& prev, const Er::PropertyBag& curr)
{
    PropertyRefs diff;
    diff.reserve(curr.size()); // do not track properties that have wanished

    for (auto& prop: curr)
    {
        auto id = prop.first;
        auto it = prev.find(id);
        if (it == prev.end())
        {
            // the property has wanished O_o
            diff.push_back(&prop.second);
        }
        else
        {
            // compare property values
            auto pi = Er::getPropertyInfo(prop.second);
            if (!pi)
                throw Er::Exception(ER_HERE(), Er::Util::format("Unknown property #%08x", prop.first));

            if (!pi->equal(it->second, prop.second))
            {
                diff.push_back(&prop.second);
            }
        }
    }

    return diff;
}




} // namespace Private {}

} // namespace Er {}