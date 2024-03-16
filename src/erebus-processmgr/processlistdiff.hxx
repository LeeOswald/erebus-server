#pragma once

#include <erebus-processmgr/processmgr.hxx>
#include <erebus-processmgr/processprops.hxx>
#include <erebus-processmgr/procfs.hxx>


#include <chrono>
#include <unordered_map>
#include <vector>

namespace Er
{

namespace Private
{


struct ProcessData
    : public Er::NonCopyable
{
    uint64_t pid;
    bool isNew;
    std::chrono::steady_clock::time_point timestamp;
    Er::PropertyBag properties;

    explicit ProcessData(uint64_t pid, bool isNew, std::chrono::steady_clock::time_point timestamp, Er::PropertyBag&& properties) noexcept
        : pid(pid)
        , isNew(isNew)
        , timestamp(timestamp)
        , properties(std::move(properties))
    {}
};

struct ProcessCollection
    : public Er::NonCopyable
{
    std::unordered_map<uint64_t, std::unique_ptr<ProcessData>> processes;
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
    double stime = 0.0;
    double utime = 0.0;
};

Er::PropertyBag collectProcessDetails(Er::ProcFs::ProcFs& source, uint64_t pid, Er::ProcessProps::PropMask required, Er::PropertyBag&& previous, ProcessDetailsCached& cached);
Er::PropertyBag collectKernelDetails(Er::ProcFs::ProcFs& source, Er::ProcessProps::PropMask required);

Er::ProcessProps::PropMask filterVolatileProps(Er::ProcFs::ProcFs& source, uint64_t pid, const Er::PropertyBag& existing, Er::ProcessProps::PropMask required, Er::PropertyBag& current);

class IconManager;
void addProcessIcon(IconManager* iconCache, Er::PropertyBag& bag);

ProcessDataDiff diffProcessData(uint64_t pid, const Er::PropertyBag& prev, const Er::PropertyBag& curr);

struct ProcessStatistics
{
    double sTimeTotal = 0.0;
    double uTimeTotal = 0.0;
};

ProcessCollectionDiff updateProcessCollection(Er::ProcFs::ProcFs& source, IconManager* iconCache, Er::ProcessProps::PropMask required, ProcessCollection& collection, ProcessStatistics& stats);


} // namespace Private {}

} // namespace Er {}