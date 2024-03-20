#include "iconmanager.hxx"
#include "processlistdiff.hxx"

#include <erebus/exception.hxx>
#include <erebus/system/user.hxx>
#include <erebus/util/format.hxx>


namespace Er
{

namespace Private
{

Er::PropertyBag collectProcessDetails(Er::ProcFs::ProcFs& source, uint64_t pid, Er::ProcessProps::PropMask required, Er::PropertyBag&& previous, ProcessDetailsCached& cached)
{
    Er::PropertyBag bag(std::move(previous));

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

        if (required[Er::ProcessProps::PropIndices::Tty])
            bag.insert({ Er::ProcessProps::Tty::Id::value, Er::Property(Er::ProcessProps::Tty::Id::value, stat.tty_nr) });
        
        if (required[Er::ProcessProps::PropIndices::State])
        {
            std::string state({ stat.state });
            bag.insert({ Er::ProcessProps::State::Id::value, Er::Property(Er::ProcessProps::State::Id::value, std::move(state)) });
        }

        if (required[Er::ProcessProps::PropIndices::Comm])
        {
            auto comm = source.readComm(pid);
            if (!comm.empty())
            {
                cached.comm = comm;
                bag.insert({ Er::ProcessProps::Comm::Id::value, Er::Property(Er::ProcessProps::Comm::Id::value, std::move(comm)) });
            }
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
            {
                cached.exe = exe;
                bag.insert({ Er::ProcessProps::Exe::Id::value, Er::Property(Er::ProcessProps::Exe::Id::value, std::move(exe)) });
            }
        }

        if (required[Er::ProcessProps::PropIndices::User])
        {
            auto user = Er::System::User::lookup(stat.ruid);
            if (user)
                bag.insert({ Er::ProcessProps::User::Id::value, Er::Property(Er::ProcessProps::User::Id::value, std::move(user->name)) });
        }

        if (required[Er::ProcessProps::PropIndices::ThreadCount])
        {
            bag.insert({ Er::ProcessProps::ThreadCount::Id::value, Er::Property(Er::ProcessProps::ThreadCount::Id::value, stat.num_threads)});
        }

        if (required[Er::ProcessProps::PropIndices::UTime])
        {
            bag.insert({ Er::ProcessProps::UTime::Id::value, Er::Property(Er::ProcessProps::UTime::Id::value, stat.uTime)});
        }

        cached.utime = stat.uTime;

        if (required[Er::ProcessProps::PropIndices::STime])
        {
            bag.insert({ Er::ProcessProps::STime::Id::value, Er::Property(Er::ProcessProps::STime::Id::value, stat.sTime)});
        }

        cached.stime = stat.sTime;
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
        auto bootTime = source.bootTime();
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

Er::ProcessProps::PropMask filterVolatileProps(Er::ProcFs::ProcFs& source, uint64_t pid, const Er::PropertyBag& existing, Er::ProcessProps::PropMask required, Er::PropertyBag& current)
{
    auto filtered = required;
    if (existing.empty())
        return filtered; // looks like this is a new process, no data collected yet

    // almost all props get changed when exec() is called
    // compare /proc/[pid]/exe contents to detect that exec() has occurred 
    
    auto exeOld = Er::getProperty<Er::ProcessProps::Exe::ValueType>(existing, Er::ProcessProps::Exe::Id::value, std::string());
    auto exeCurrent = source.readExePath(pid);
    auto exeChanged = (exeCurrent != exeOld);
    if (exeChanged)
    {
        // since we already have an exe path, no need to look for it again
        current.insert({ Er::ProcessProps::Exe::Id::value, Er::Property(Er::ProcessProps::Exe::Id::value, std::move(exeCurrent)) });
        filtered.reset(Er::ProcessProps::PropIndices::Exe);
    }

    if (!exeChanged)
    {
        if (required[Er::ProcessProps::PropIndices::Icon])
        {
            // if we have an icon already
            if (propertyPresent(existing, Er::ProcessProps::Icon::Id::value))
            {
                if (!exeChanged)
                    filtered.reset(Er::ProcessProps::PropIndices::Icon);
            }
        }

        filtered.reset(Er::ProcessProps::PropIndices::User);
        filtered.reset(Er::ProcessProps::PropIndices::Ruid);
    }

    return filtered;
}

void addProcessIcon(const std::string& comm, const std::string& exe, IconManager* cache, Er::PropertyBag& bag)
{
    auto ico = cache->lookup(comm, exe, Er::Private::IconSize::Small);
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

ProcessCollectionDiff updateProcessCollection(Er::ProcFs::ProcFs& source, IconManager* iconCache, Er::ProcessProps::PropMask required, ProcessCollection& collection, ProcessStatistics& stats)
{
    auto firstRun = collection.processes.empty();
    auto now = std::chrono::steady_clock::now();

    ProcessCollectionDiff diff;
    
    auto pids = source.enumeratePids();
    diff.processCount = pids.size();

    for (auto pid: pids)
    {
        ProcessDetailsCached cached;

        auto existing = collection.processes.find(pid);
        if (existing != collection.processes.end())
        {
            // this is an existing process, only need to update volatile props
            auto& oldProps = existing->second.get()->properties;
            Er::PropertyBag newProps;
            auto filtered = filterVolatileProps(source, pid, oldProps, required, newProps);

            auto process = collectProcessDetails(source, pid, filtered, std::move(newProps), cached);
            
            if (filtered[Er::ProcessProps::PropIndices::Icon])
            {
                if (iconCache)
                    addProcessIcon(cached.comm, cached.exe, iconCache, process);
            }

            updateDiffAndCollectionForProcess(firstRun, diff, collection, now, pid, std::move(process));
        }
        else
        {
            // this is a new process
            auto process = collectProcessDetails(source, pid, required, Er::PropertyBag(), cached);

            if (iconCache)
                addProcessIcon(cached.comm, cached.exe, iconCache, process);

            updateDiffAndCollectionForProcess(firstRun, diff, collection, now, pid, std::move(process));
        }

        stats.uTimeTotal += cached.utime;
        stats.sTimeTotal += cached.stime;
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