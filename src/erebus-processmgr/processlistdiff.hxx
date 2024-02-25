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
    bool isNew;
    std::chrono::steady_clock::time_point timestamp;
    Er::PropertyBag properties;

    explicit ProcessData(bool isNew, std::chrono::steady_clock::time_point timestamp, Er::PropertyBag&& properties) noexcept
        : isNew(isNew)
        , timestamp(timestamp)
        , properties(std::move(properties))
    {}
};


struct ProcessCollection
    : public Er::NonCopyable
{
    std::unordered_map<uint64_t, std::unique_ptr<ProcessData>> processes;
};


using PropertyRefs = std::vector<const Er::Property*>;

struct ProcessCollectionDiff
{
    std::vector<uint64_t> removed;
    std::vector<PropertyRefs> modified;
    std::vector<const ProcessData*> added;
};


Er::PropertyBag collectProcessDetails(Er::ProcFs::ProcFs& source, uint64_t pid, Er::ProcessProps::PropMask required);
Er::PropertyBag collectKernelDetails(Er::ProcFs::ProcFs& source, Er::ProcessProps::PropMask required);

PropertyRefs diffProcessData(const Er::PropertyBag& prev, const Er::PropertyBag& curr);

ProcessCollectionDiff updateProcessCollection(Er::ProcFs::ProcFs& source, Er::ProcessProps::PropMask required, ProcessCollection& collection);


} // namespace Private {}

} // namespace Er {}