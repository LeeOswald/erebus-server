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
        Er::addProperty<Er::ProcessProps::Pid>(bag, stat.pid);
        Er::addProperty<Er::ProcessProps::Valid>(bag, false);
        Er::addProperty<Er::ProcessProps::Error>(bag, std::move(stat.error));
    }
    else
    {
        Er::addProperty<Er::ProcessProps::Valid>(bag, true);
        Er::addProperty<Er::ProcessProps::Pid>(bag, stat.pid);
        Er::addProperty<Er::ProcessProps::PPid>(bag, stat.ppid);
                
        if (required[Er::ProcessProps::PropIndices::PGrp])
            Er::addProperty<Er::ProcessProps::PGrp>(bag, stat.pgrp);

        if (required[Er::ProcessProps::PropIndices::Tpgid])
            if (stat.tpgid != std::numeric_limits<uint64_t>::max())
                Er::addProperty<Er::ProcessProps::Tpgid>(bag, stat.tpgid);
        
        if (required[Er::ProcessProps::PropIndices::Session])
            Er::addProperty<Er::ProcessProps::Session>(bag, stat.session);
        
        if (required[Er::ProcessProps::PropIndices::Ruid])
            Er::addProperty<Er::ProcessProps::Ruid>(bag, stat.ruid);
        
        if (required[Er::ProcessProps::PropIndices::StartTime])
            Er::addProperty<Er::ProcessProps::StartTime>(bag, stat.startTime);

        if (required[Er::ProcessProps::PropIndices::Tty])
            Er::addProperty<Er::ProcessProps::Tty>(bag, stat.tty_nr);
        
        if (required[Er::ProcessProps::PropIndices::State])
        {
            std::string state({ stat.state });
            Er::addProperty<Er::ProcessProps::State>(bag, std::move(state));
        }

        if (required[Er::ProcessProps::PropIndices::Comm])
        {
            auto comm = source.readComm(pid);
            if (!comm.empty())
            {
                cached.comm = comm;
                Er::addProperty<Er::ProcessProps::Comm>(bag, std::move(comm));
            }
        }

        if (required[Er::ProcessProps::PropIndices::CmdLine])
        {
            auto cmdLine = source.readCmdLine(pid);
            if (!cmdLine.empty())
                Er::addProperty<Er::ProcessProps::CmdLine>(bag, std::move(cmdLine));
        }

        if (required[Er::ProcessProps::PropIndices::Exe])
        {
            auto exe = source.readExePath(pid);
            if (!exe.empty())
            {
                cached.exe = exe;
                Er::addProperty<Er::ProcessProps::Exe>(bag, std::move(exe));
            }
        }

        if (required[Er::ProcessProps::PropIndices::User])
        {
            auto user = Er::System::User::lookup(stat.ruid);
            if (user)
                Er::addProperty<Er::ProcessProps::User>(bag, std::move(user->name));
        }

        if (required[Er::ProcessProps::PropIndices::ThreadCount])
        {
            Er::addProperty<Er::ProcessProps::ThreadCount>(bag, stat.num_threads);
        }

        if (required[Er::ProcessProps::PropIndices::UTime])
        {
            Er::addProperty<Er::ProcessProps::UTime>(bag, stat.uTime);
        }

        cached.utime = stat.uTime;

        if (required[Er::ProcessProps::PropIndices::STime])
        {
            Er::addProperty<Er::ProcessProps::STime>(bag, stat.sTime);
        }

        cached.stime = stat.sTime;
    }

    return bag;
}

Er::PropertyBag collectKernelDetails(Er::ProcFs::ProcFs& source, Er::ProcessProps::PropMask required)
{
    Er::PropertyBag bag;

    Er::addProperty<Er::ProcessProps::Valid>(bag, true);
    Er::addProperty<Er::ProcessProps::Pid>(bag, ProcFs::KernelPid);
    
    if (required[Er::ProcessProps::PropIndices::StartTime])
    {
        auto bootTime = source.bootTime();
        Er::addProperty<Er::ProcessProps::StartTime>(bag, bootTime);
    }

    if (required[Er::ProcessProps::PropIndices::CmdLine])
    {
        auto cmdLine = source.readCmdLine(ProcFs::KernelPid);
        if (!cmdLine.empty())
            Er::addProperty<Er::ProcessProps::CmdLine>(bag, std::move(cmdLine));
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
    
    auto exeOld = Er::getPropertyOr<Er::ProcessProps::Exe>(existing, std::string());
    auto exeCurrent = source.readExePath(pid);
    auto exeChanged = (exeCurrent != exeOld);
    if (exeChanged)
    {
        // since we already have an exe path, no need to look for it again
        Er::addProperty<Er::ProcessProps::Exe>(current, std::move(exeCurrent));
        filtered.reset(Er::ProcessProps::PropIndices::Exe);
    }

    if (!exeChanged)
    {
        if (required[Er::ProcessProps::PropIndices::Icon])
        {
            // if we have an icon already
            if (propertyPresent<Er::ProcessProps::Icon>(existing))
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
        Er::addProperty<Er::ProcessProps::Icon>(bag, ico->data);
}

ProcessDataDiff diffAndUpdateProcessProps(uint64_t pid, Er::PropertyBag&& newProps, Er::PropertyBag& existing)
{
    ProcessDataDiff diff(pid);
    diff.properties.reserve(existing.size()); // do not track properties that have wanished

    for (auto& prop: newProps)
    {
        auto id = prop.first;
        auto it = existing.find(id);
        if (it == existing.end())
        {
            // new property appeared
            diff.properties.push_back(prop.second);

            existing.insert({prop.first, std::move(prop.second)});
        }
        else
        {
            // compare property values
            auto pi = Er::getPropertyInfo(prop.second);
            if (!pi)
                throw Er::Exception(ER_HERE(), Er::Util::format("Unknown property #%08x", prop.first));

            if (!pi->equal(it->second, prop.second))
            {
                // update changed props
                diff.properties.push_back(prop.second);

                auto& ref = existing.at(prop.first);
                ref = std::move(prop.second);
            }
        }
    }

    return diff;
}

static void updateDiffAndCollectionForProcess(ProcessCollection::Container::iterator which, bool firstRun, ProcessCollectionDiff& diff, ProcessCollection& collection, std::chrono::steady_clock::time_point now, uint64_t pid, Er::PropertyBag&& newProps)
{
    Er::cachePropertyInfo(newProps);

    // is this a previously existed process?
    if (which == collection.processes.end())
    {
        // new one
        auto data = std::make_unique<ProcessData>(pid, !firstRun, now, std::move(newProps));
        auto item = collection.processes.insert({ pid, std::move(data) });
        assert(item.second); // really a new item
        diff.added.push_back(item.first->second.get());
    }
    else
    {
        // modified one
        auto singleDiff = diffAndUpdateProcessProps(pid, std::move(newProps), which->second->properties);
        
        which->second->isNew = false;
        which->second->timestamp = now;
        
        if (!singleDiff.properties.empty())
        {
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

            updateDiffAndCollectionForProcess(existing, firstRun, diff, collection, now, pid, std::move(process));
        }
        else
        {
            // this is a new process
            auto process = collectProcessDetails(source, pid, required, Er::PropertyBag(), cached);

            if (iconCache)
                addProcessIcon(cached.comm, cached.exe, iconCache, process);

            updateDiffAndCollectionForProcess(collection.processes.end(), firstRun, diff, collection, now, pid, std::move(process));
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