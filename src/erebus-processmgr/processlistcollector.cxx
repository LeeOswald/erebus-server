#include "processlistcollector.hxx"

#include <erebus/system/user.hxx>


namespace Erp
{

namespace ProcessMgr
{

namespace
{

template <Er::IsPropertyValue PropT>
void updateProperty(bool replaceProps, Er::PropertyBag& bag, Er::PropertyBag* diff, std::size_t index, typename PropT::ValueType const& value)
{
    bool addToDiff = !!diff;
    if (replaceProps)
    {
        // update the existing prop
        if (!Er::updatePropertyValueAt<PropT>(bag, index, value))
            addToDiff = false;
    }
    else
    {
        // just add prop
        Er::setPropertyValueAt<PropT>(bag, index, value);
    }

    if (addToDiff)
        Er::addProperty<PropT>(*diff, value);
}

template <Er::IsPropertyValue PropT>
void updateProperty(bool replaceProps, Er::PropertyBag& bag, Er::PropertyBag* diff, std::size_t index, typename PropT::ValueType&& value)
{
    bool addToDiff = !!diff;
    if (replaceProps)
    {
        // update the existing prop
        if (!Er::updatePropertyValueAt<PropT>(bag, index, value))
            addToDiff = false;
    }
    else
    {
        // just add prop
        Er::setPropertyValueAt<PropT>(bag, index, value);
    }

    if (addToDiff)
        Er::addProperty<PropT>(*diff, std::move(value));
}

} // namespace {}


ProcessListCollector::ProcessListCollector(Er::Log::ILog* log, ProcFs& procFs)
    : m_log(log)
    , m_procFs(procFs)
{
}

ProcessListCollector::ProcessInfoCollectionDiff ProcessListCollector::update(PropMask required)
{
    ProcessInfoCollectionDiff result;
    result.firstRun = m_firstRun;

    auto now = ProcessInfo::Clock::now();

    auto pids = m_procFs.enumeratePids();
    result.processCount = pids.size();

    for (auto pid: pids)
    {
        std::shared_ptr<ProcessInfo> info;

        auto it = m_collection.find(pid);
        if (it != m_collection.end())
        {
            // update the existing process
            info = it->second;
            info->isNew = false;
            info->timestamp = now;
        }
        else
        {
            // either a new process or just the initial run
            info = std::make_shared<ProcessInfo>(pid, true, now);

            m_collection.insert({ pid, info });
        }

        if (pid == KernelPid)
            updateKernelProcess(result, required, info);
        else
            updateProcess(result, required, pid, info);

        if (m_firstRun)
            info->isNew = false;

        result.uTimeTotal += info->utime;
        result.uTimeTotal += info->utime;
    }

    m_firstRun = false;
    m_required = required;

    // now look for processes that haven't updated their timestamps
    // this means they have gone away
    for (auto it = m_collection.begin(); it != m_collection.end(); )
    {
        if (it->second->timestamp < now)
        {
            result.removed.push_back(it->second);

            auto next = std::next(it);
            m_collection.erase(it);
            it = next;
        }
        else
        {
            ++it;
        }
    }    

    return result;
}

void ProcessListCollector::updateKernelProcess(ProcessInfoCollectionDiff& result, PropMask required, std::shared_ptr<ProcessInfo> info)
{
    ProcessInfoDiff diff(info);
    Er::PropertyBag* pdiff = info->isNew ? nullptr : &diff.properties;

    if ((required != m_required) && !info->isNew)
    {
        auto oldSize = Er::propertyCount(info->properties);
        Er::clearPropertyBag(info->properties);
        Er::resizePropertyBag(info->properties, std::max(oldSize, required.count()));
    }

    bool replaceProps = (required == m_required) && !info->isNew;

    // property[0] -> pid
    // property[1] -> valid
    // property[2] -> error
    updateProperty<Er::ProcessMgr::ProcessProps::Pid>(replaceProps, info->properties, pdiff, 0, KernelPid);
    updateProperty<Er::ProcessMgr::ProcessProps::Valid>(replaceProps, info->properties, pdiff, 1, true);
    updateProperty<Er::ProcessMgr::ProcessProps::Error>(replaceProps, info->properties, pdiff, 2, std::string());
    
    std::size_t nextProp = 3;
        
    if (required[Er::ProcessMgr::ProcessProps::PropIndices::StartTime])
    {
        auto bootTime = m_procFs.bootTime();
        updateProperty<Er::ProcessMgr::ProcessProps::StartTime>(replaceProps, info->properties, pdiff, nextProp++, bootTime);
    }

    if (required[Er::ProcessMgr::ProcessProps::PropIndices::CmdLine])
    {
        auto cmdLine = m_procFs.readCmdLine(KernelPid);
        if (!cmdLine.empty())
            updateProperty<Er::ProcessMgr::ProcessProps::CmdLine>(replaceProps, info->properties, pdiff, nextProp++, std::move(cmdLine));
    }

    if (info->isNew)
    {
        result.added.push_back(info);
    }
    else if (!diff.properties.empty())
    {
        result.modified.push_back(std::move(diff));
    }
}

void ProcessListCollector::updateProcess(ProcessInfoCollectionDiff& result, PropMask required, uint64_t pid, std::shared_ptr<ProcessInfo> info)
{
    ProcessInfoDiff diff(info);
    Er::PropertyBag* pdiff = info->isNew ? nullptr : &diff.properties;

    if ((required != m_required) && !info->isNew)
    {
        auto oldSize = Er::propertyCount(info->properties);
        Er::clearPropertyBag(info->properties);
        Er::resizePropertyBag(info->properties, std::max(oldSize, required.count()));
    }

    auto stat = m_procFs.readStat(pid);
    ErAssert(stat.pid != InvalidPid); // PID is always valid

    bool replaceProps = (required == m_required) && !info->isNew;

    // property[0] -> pid
    // property[1] -> valid
    // property[2] -> error
    updateProperty<Er::ProcessMgr::ProcessProps::Pid>(replaceProps, info->properties, pdiff, 0, stat.pid);
    updateProperty<Er::ProcessMgr::ProcessProps::Valid>(replaceProps, info->properties, pdiff, 1, stat.valid);
    updateProperty<Er::ProcessMgr::ProcessProps::Error>(replaceProps, info->properties, pdiff, 2, std::move(stat.error));
    
    if (stat.valid)
    {
        info->ppid = stat.ppid;

        std::size_t nextProp = 3;
        
        updateProperty<Er::ProcessMgr::ProcessProps::PPid>(replaceProps, info->properties, pdiff, nextProp++, stat.ppid);

        if (required[Er::ProcessMgr::ProcessProps::PropIndices::PGrp])
            updateProperty<Er::ProcessMgr::ProcessProps::PGrp>(replaceProps, info->properties, pdiff, nextProp++, stat.pgrp);

        if (required[Er::ProcessMgr::ProcessProps::PropIndices::Tpgid])
            if (stat.tpgid != std::numeric_limits<uint64_t>::max())
                updateProperty<Er::ProcessMgr::ProcessProps::Tpgid>(replaceProps, info->properties, pdiff, nextProp++, stat.tpgid);
        
        if (required[Er::ProcessMgr::ProcessProps::PropIndices::Session])
            updateProperty<Er::ProcessMgr::ProcessProps::Session>(replaceProps, info->properties, pdiff, nextProp++, stat.session);
        
        if (required[Er::ProcessMgr::ProcessProps::PropIndices::Ruid])
            updateProperty<Er::ProcessMgr::ProcessProps::Ruid>(replaceProps, info->properties, pdiff, nextProp++, stat.ruid);
        
        if (required[Er::ProcessMgr::ProcessProps::PropIndices::StartTime])
            updateProperty<Er::ProcessMgr::ProcessProps::StartTime>(replaceProps, info->properties, pdiff, nextProp++, stat.startTime);

        if (required[Er::ProcessMgr::ProcessProps::PropIndices::Tty])
            updateProperty<Er::ProcessMgr::ProcessProps::Tty>(replaceProps, info->properties, pdiff, nextProp++, stat.tty_nr);
        
        if (required[Er::ProcessMgr::ProcessProps::PropIndices::State])
        {
            updateProperty<Er::ProcessMgr::ProcessProps::State>(replaceProps, info->properties, pdiff, nextProp++, Er::ProcessMgr::ProcessProps::State::ValueType(stat.state));
        }

        if (required[Er::ProcessMgr::ProcessProps::PropIndices::Comm])
        {
            auto comm = m_procFs.readComm(pid);
            if (!comm.empty())
            {
                info->comm = comm;
                updateProperty<Er::ProcessMgr::ProcessProps::Comm>(replaceProps, info->properties, pdiff, nextProp++, std::move(comm));
            }
        }

        if (required[Er::ProcessMgr::ProcessProps::PropIndices::CmdLine])
        {
            auto cmdLine = m_procFs.readCmdLine(pid);
            if (!cmdLine.empty())
                updateProperty<Er::ProcessMgr::ProcessProps::CmdLine>(replaceProps, info->properties, pdiff, nextProp++, std::move(cmdLine));
        }

        if (stat.ppid != KThreadDPid)
        {
            if (required[Er::ProcessMgr::ProcessProps::PropIndices::Exe])
            {
                auto exe = m_procFs.readExePath(pid);
                if (!exe.empty())
                {
                    info->exe = exe;
                    updateProperty<Er::ProcessMgr::ProcessProps::Exe>(replaceProps, info->properties, pdiff, nextProp++, std::move(exe));
                }
            }
        }

        if (required[Er::ProcessMgr::ProcessProps::PropIndices::User])
        {
            auto user = Er::System::User::lookup(stat.ruid);
            if (user)
                updateProperty<Er::ProcessMgr::ProcessProps::User>(replaceProps, info->properties, pdiff, nextProp++, std::move(user->name));
        }

        if (required[Er::ProcessMgr::ProcessProps::PropIndices::ThreadCount])
        {
            updateProperty<Er::ProcessMgr::ProcessProps::ThreadCount>(replaceProps, info->properties, pdiff, nextProp++, stat.num_threads);
        }

        if (required[Er::ProcessMgr::ProcessProps::PropIndices::UTime])
        {
            updateProperty<Er::ProcessMgr::ProcessProps::UTime>(replaceProps, info->properties, pdiff, nextProp++, stat.uTime);
        }

        info->utime = stat.uTime;

        if (required[Er::ProcessMgr::ProcessProps::PropIndices::STime])
        {
            updateProperty<Er::ProcessMgr::ProcessProps::STime>(replaceProps, info->properties, pdiff, nextProp++, stat.sTime);
        }

        info->stime = stat.sTime;
    }

    if (info->isNew)
    {
        result.added.push_back(info);
    }
    else if (!diff.properties.empty())
    {
        result.modified.push_back(std::move(diff));
    }
}

} // namespace ProcessMgr {}

} // namespace Erp {}
