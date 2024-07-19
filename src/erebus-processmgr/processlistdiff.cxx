#include "processlistdiff.hxx"

#include <erebus/exception.hxx>
#include <erebus/system/user.hxx>
#include <erebus/util/format.hxx>


namespace Erp
{

namespace ProcessMgr
{

Er::PropertyBag collectProcessDetails(ProcFs& source, uint64_t pid, Er::ProcessMgr::ProcessProps::PropMask required, Er::PropertyBag&& previous, ProcessDetailsCached& cached)
{
    Er::PropertyBag bag(std::move(previous));

    auto stat = source.readStat(pid);
    ErAssert(stat.pid != InvalidPid); // PID is always valid

    if (!stat.valid)
    {
        Er::addProperty<Er::ProcessMgr::ProcessProps::Valid>(bag, false);
        Er::addProperty<Er::ProcessMgr::ProcessProps::Pid>(bag, stat.pid);
        Er::addProperty<Er::ProcessMgr::ProcessProps::Error>(bag, std::move(stat.error));
    }
    else
    {
        Er::addProperty<Er::ProcessMgr::ProcessProps::Valid>(bag, true);
        Er::addProperty<Er::ProcessMgr::ProcessProps::Pid>(bag, stat.pid);
        Er::addProperty<Er::ProcessMgr::ProcessProps::PPid>(bag, stat.ppid);
        cached.ppid = stat.ppid;
                
        if (required[Er::ProcessMgr::ProcessProps::PropIndices::PGrp])
            Er::addProperty<Er::ProcessMgr::ProcessProps::PGrp>(bag, stat.pgrp);

        if (required[Er::ProcessMgr::ProcessProps::PropIndices::Tpgid])
            if (stat.tpgid != std::numeric_limits<uint64_t>::max())
                Er::addProperty<Er::ProcessMgr::ProcessProps::Tpgid>(bag, stat.tpgid);
        
        if (required[Er::ProcessMgr::ProcessProps::PropIndices::Session])
            Er::addProperty<Er::ProcessMgr::ProcessProps::Session>(bag, stat.session);
        
        if (required[Er::ProcessMgr::ProcessProps::PropIndices::Ruid])
            Er::addProperty<Er::ProcessMgr::ProcessProps::Ruid>(bag, stat.ruid);
        
        if (required[Er::ProcessMgr::ProcessProps::PropIndices::StartTime])
            Er::addProperty<Er::ProcessMgr::ProcessProps::StartTime>(bag, stat.startTime);

        if (required[Er::ProcessMgr::ProcessProps::PropIndices::Tty])
            Er::addProperty<Er::ProcessMgr::ProcessProps::Tty>(bag, stat.tty_nr);
        
        if (required[Er::ProcessMgr::ProcessProps::PropIndices::State])
        {
            std::string state({ stat.state });
            Er::addProperty<Er::ProcessMgr::ProcessProps::State>(bag, std::move(state));
        }

        if (required[Er::ProcessMgr::ProcessProps::PropIndices::Comm])
        {
            auto comm = source.readComm(pid);
            if (!comm.empty())
            {
                cached.comm = comm;
                Er::addProperty<Er::ProcessMgr::ProcessProps::Comm>(bag, std::move(comm));
            }
        }

        if (required[Er::ProcessMgr::ProcessProps::PropIndices::CmdLine])
        {
            auto cmdLine = source.readCmdLine(pid);
            if (!cmdLine.empty())
                Er::addProperty<Er::ProcessMgr::ProcessProps::CmdLine>(bag, std::move(cmdLine));
        }

        if (stat.ppid != KThreadDPid)
        {
            if (required[Er::ProcessMgr::ProcessProps::PropIndices::Exe])
            {
                auto exe = source.readExePath(pid);
                if (!exe.empty())
                {
                    cached.exe = exe;
                    Er::addProperty<Er::ProcessMgr::ProcessProps::Exe>(bag, std::move(exe));
                }
            }
        }

        if (required[Er::ProcessMgr::ProcessProps::PropIndices::User])
        {
            auto user = Er::System::User::lookup(stat.ruid);
            if (user)
                Er::addProperty<Er::ProcessMgr::ProcessProps::User>(bag, std::move(user->name));
        }

        if (required[Er::ProcessMgr::ProcessProps::PropIndices::ThreadCount])
        {
            Er::addProperty<Er::ProcessMgr::ProcessProps::ThreadCount>(bag, stat.num_threads);
        }

        if (required[Er::ProcessMgr::ProcessProps::PropIndices::UTime])
        {
            Er::addProperty<Er::ProcessMgr::ProcessProps::UTime>(bag, stat.uTime);
        }

        cached.utime = stat.uTime;

        if (required[Er::ProcessMgr::ProcessProps::PropIndices::STime])
        {
            Er::addProperty<Er::ProcessMgr::ProcessProps::STime>(bag, stat.sTime);
        }

        cached.stime = stat.sTime;
    }

    return bag;
}

Er::PropertyBag collectKernelDetails(ProcFs& source, Er::ProcessMgr::ProcessProps::PropMask required)
{
    Er::PropertyBag bag;

    Er::addProperty<Er::ProcessMgr::ProcessProps::Valid>(bag, true);
    Er::addProperty<Er::ProcessMgr::ProcessProps::Pid>(bag, KernelPid);
    
    if (required[Er::ProcessMgr::ProcessProps::PropIndices::StartTime])
    {
        auto bootTime = source.bootTime();
        Er::addProperty<Er::ProcessMgr::ProcessProps::StartTime>(bag, bootTime);
    }

    if (required[Er::ProcessMgr::ProcessProps::PropIndices::CmdLine])
    {
        auto cmdLine = source.readCmdLine(KernelPid);
        if (!cmdLine.empty())
            Er::addProperty<Er::ProcessMgr::ProcessProps::CmdLine>(bag, std::move(cmdLine));
    }

    return bag;
}

Er::ProcessMgr::ProcessProps::PropMask filterVolatileProps(ProcFs& source, uint64_t pid, uint64_t ppid, const Er::PropertyBag& existing, Er::ProcessMgr::ProcessProps::PropMask required, Er::PropertyBag& current)
{
    auto filtered = required;
    if (existing.empty())
        return filtered; // looks like this is a new process, no data collected yet

    if (ppid != KThreadDPid)
    {
        // almost all props get changed when exec() is called
        // compare /proc/[pid]/exe contents to detect that exec() has occurred 
        
        auto exeOld = Er::getPropertyValueOr<Er::ProcessMgr::ProcessProps::Exe>(existing, std::string());
        auto exeCurrent = source.readExePath(pid);
        auto exeChanged = (exeCurrent != exeOld);
        if (exeChanged)
        {
            // since we already have an exe path, no need to look for it again
            Er::addProperty<Er::ProcessMgr::ProcessProps::Exe>(current, std::move(exeCurrent));
            filtered.reset(Er::ProcessMgr::ProcessProps::PropIndices::Exe);
        }

        if (!exeChanged)
        {
            filtered.reset(Er::ProcessMgr::ProcessProps::PropIndices::User);
            filtered.reset(Er::ProcessMgr::ProcessProps::PropIndices::Ruid);
        }
    }

    return filtered;
}

ProcessDataDiff diffAndUpdateProcessProps(uint64_t pid, Er::PropertyBag&& newProps, Er::PropertyBag& existing)
{
    ProcessDataDiff diff(pid);
    diff.properties.reserve(existing.size()); // do not track properties that have wanished

    Er::enumerateProperties(newProps, [&existing, &diff](Er::Property& prop)
    {
        auto current = Er::getProperty(existing, prop.id);
        if (!current)
        {
            // new property appeared
            diff.properties.push_back(prop);
            Er::addProperty(existing, std::move(prop));
        }
        else
        {
            // compare property values
            if (*current != prop)
            {
                // update changed props
                diff.properties.push_back(prop);
                *current = std::move(prop);
            }
        }
    });

    return diff;
}

static void updateDiffAndCollectionForProcess(ProcessCollection::Container::iterator which, bool firstRun, ProcessCollectionDiff& diff, ProcessCollection& collection, std::chrono::steady_clock::time_point now, uint64_t pid, uint64_t ppid, Er::PropertyBag&& newProps)
{
    // is this a previously existed process?
    if (which == collection.processes.end())
    {
        // new one
        auto data = std::make_unique<ProcessData>(pid, ppid, !firstRun, now, std::move(newProps));
        auto item = collection.processes.insert({ pid, std::move(data) });
        ErAssert(item.second); // really a new item
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

ProcessCollectionDiff updateProcessCollection(ProcFs& source, Er::ProcessMgr::ProcessProps::PropMask required, ProcessCollection& collection, ProcessStatistics& stats)
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
            auto ppid = existing->second.get()->ppid;
            Er::PropertyBag newProps;
            auto filtered = filterVolatileProps(source, pid, ppid, oldProps, required, newProps);

            auto process = collectProcessDetails(source, pid, filtered, std::move(newProps), cached);
            updateDiffAndCollectionForProcess(existing, firstRun, diff, collection, now, pid, ppid, std::move(process));
        }
        else
        {
            // this is a new process
            auto process = collectProcessDetails(source, pid, required, Er::PropertyBag(), cached);
            updateDiffAndCollectionForProcess(collection.processes.end(), firstRun, diff, collection, now, pid, cached.ppid, std::move(process));
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



} // namespace ProcessMgr {}

} // namespace Erp {}