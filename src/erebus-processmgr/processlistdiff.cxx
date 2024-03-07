#include "iconmanager.hxx"
#include "processlistdiff.hxx"

#include <erebus/exception.hxx>
#include <erebus/system/user.hxx>
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

        if (required[Er::ProcessProps::PropIndices::Exe] || required[Er::ProcessProps::PropIndices::Icon])
        {
            auto exe = source.readExePath(pid);
            if (!exe.empty())
                bag.insert({ Er::ProcessProps::Exe::Id::value, Er::Property(Er::ProcessProps::Exe::Id::value, std::move(exe)) });
        }

        if (required[Er::ProcessProps::PropIndices::User])
        {
            auto user = Er::System::User::lookup(stat.ruid);
            if (user)
                bag.insert({ Er::ProcessProps::User::Id::value, Er::Property(Er::ProcessProps::User::Id::value, std::move(user->name)) });
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

void addProcessIcon(IconManager* cache, Er::PropertyBag& bag)
{
    auto it = bag.find(Er::ProcessProps::Exe::Id::value);
    if (it == bag.end())
        return;

    auto exe = std::any_cast<std::string>(it->second.value);
    auto ico = cache->lookup(exe, Er::Private::IconSize::Small);
    if (ico && ico->valid)
        bag.insert({ Er::ProcessProps::Icon::Id::value, Er::Property(Er::ProcessProps::Icon::Id::value, ico->data) });
}

ProcessDataDiff diffProcessData(uint64_t pid, const Er::PropertyBag& prev, const Er::PropertyBag& curr)
{
    ProcessDataDiff diff(pid);
    diff.properties.reserve(curr.size()); // do not track properties that have wanished

    for (auto& prop: curr)
    {
        auto id = prop.first;
        auto it = prev.find(id);
        if (it == prev.end())
        {
            // new property appeared
            diff.properties.push_back(prop.second);
        }
        else
        {
            // compare property values
            auto pi = Er::getPropertyInfo(prop.second);
            if (!pi)
                throw Er::Exception(ER_HERE(), Er::Util::format("Unknown property #%08x", prop.first));

            if (!pi->equal(it->second, prop.second))
            {
                diff.properties.push_back(prop.second);
            }
        }
    }

    return diff;
}

static void updateDiffAndCollectionForProcess(bool firstRun, ProcessCollectionDiff& diff, ProcessCollection& collection, std::chrono::steady_clock::time_point now, uint64_t pid, Er::PropertyBag&& process)
{
    Er::cachePropertyInfo(process);

    // is this a previously existed process?
    auto it = collection.processes.find(pid);
    if (it == collection.processes.end())
    {
        // new one
        auto data = std::make_unique<ProcessData>(pid, !firstRun, now, std::move(process));
        auto item = collection.processes.insert({ pid, std::move(data) });
        assert(item.second); // really a new item
        diff.added.push_back(item.first->second.get());
    }
    else
    {
        // modified one
        auto singleDiff = diffProcessData(pid, process, it->second->properties);
        if (singleDiff.properties.empty())
        {
            // unmodified, just update the timestamp
            it->second->isNew = false;
            it->second->timestamp = now;
        }
        else
        {
            // replace with updated data
            auto data = std::make_unique<ProcessData>(pid, !firstRun, now, std::move(process));
            it->second.swap(data);

            diff.modified.push_back(std::move(singleDiff));
        }
    }
}

ProcessCollectionDiff updateProcessCollection(Er::ProcFs::ProcFs& source, Er::ProcessProps::PropMask required, ProcessCollection& collection)
{
    auto firstRun = collection.processes.empty();
    auto now = std::chrono::steady_clock::now();

    ProcessCollectionDiff diff;
    
    auto pids = source.enumeratePids();
    for (auto pid: pids)
    {
        auto process = collectProcessDetails(source, pid, required);
        updateDiffAndCollectionForProcess(firstRun, diff, collection, now, pid, std::move(process));
    }

    // now look for processes that haven't updated their timestamps
    // this means they have gone away
    for (auto process = collection.processes.begin(); process != collection.processes.end(); )
    {
        if (process->second->timestamp < now)
        {
            diff.removed.push_back(process->first);

            auto next = std::next(process);
            collection.processes.erase(process);
            process = next;
        }
        else
        {
            ++process;
        }
    }    

    return diff;
}



} // namespace Private {}

} // namespace Er {}