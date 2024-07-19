#pragma once

#include <erebus-processmgr/erebus-processmgr.hxx>

#include "procfs.hxx"


#include <chrono>
#include <unordered_map>
#include <vector>

namespace Erp
{

namespace ProcessMgr
{


struct ProcessData
    : public Er::NonCopyable
{
    uint64_t pid;
    uint64_t ppid;
    bool isNew;
    std::chrono::steady_clock::time_point timestamp;
    Er::PropertyBag properties;

    explicit ProcessData(uint64_t pid, uint64_t ppid, bool isNew, std::chrono::steady_clock::time_point timestamp, Er::PropertyBag&& properties) noexcept
        : pid(pid)
        , ppid(ppid)
        , isNew(isNew)
        , timestamp(timestamp)
        , properties(std::move(properties))
    {}
};

struct ProcessCollection
    : public Er::NonCopyable
{
    using Container = std::unordered_map<uint64_t, std::unique_ptr<ProcessData>>;
    Container processes;
};

struct ProcessDataDiff
{
    uint64_t pid;
    std::vector<Er::Property> properties;

    explicit ProcessDataDiff(uint64_t pid) noexcept
        : pid(pid)
    {}
}; 

struct ProcessCollectionDiff
{
    std::size_t processCount = 0;
    std::vector<uint64_t> removed;
    std::vector<ProcessDataDiff> modified;
    std::vector<const ProcessData*> added;
};

struct ProcessDetailsCached // smth that is faster than a property bag lookup
{
    uint64_t ppid = InvalidPid;
    std::string comm;
    std::string exe;
    double stime = 0.0;
    double utime = 0.0;
};

Er::PropertyBag collectProcessDetails(ProcFs& source, uint64_t pid, Er::ProcessMgr::ProcessProps::PropMask required, Er::PropertyBag&& previous, ProcessDetailsCached& cached);
Er::PropertyBag collectKernelDetails(ProcFs& source, Er::ProcessMgr::ProcessProps::PropMask required);

Er::ProcessMgr::ProcessProps::PropMask filterVolatileProps(ProcFs& source, uint64_t pid, uint64_t ppid, const Er::PropertyBag& existing, Er::ProcessMgr::ProcessProps::PropMask required, Er::PropertyBag& current);

ProcessDataDiff diffAndUpdateProcessProps(uint64_t pid, const Er::PropertyBag& prev, Er::PropertyBag& curr);

struct ProcessStatistics
{
    double sTimeTotal = 0.0;
    double uTimeTotal = 0.0;
};

ProcessCollectionDiff updateProcessCollection(ProcFs& source, Er::ProcessMgr::ProcessProps::PropMask required, ProcessCollection& collection, ProcessStatistics& stats);


} // namespace ProcessMgr {}

} // namespace Erp {}