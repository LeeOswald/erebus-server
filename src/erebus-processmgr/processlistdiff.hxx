#pragma once

#include <erebus-processmgr/processmgr.hxx>
#include <erebus-processmgr/processprops.hxx>
#include <erebus-processmgr/procfs.hxx>


#include <chrono>
#include <unordered_map>

namespace Er
{

namespace Private
{


struct ProcessData
    : public Er::NonCopyable
{
    std::chrono::steady_clock::time_point timestamp;
    Er::PropertyBag properties;

    explicit ProcessData(std::chrono::steady_clock::time_point timestamp, Er::PropertyBag&& properties) noexcept
        : timestamp(timestamp)
        , properties(std::move(properties))
    {}
};


struct ProcessCollection
    : public Er::NonCopyable
{
    std::unordered_map<uint64_t, std::unique_ptr<ProcessData>> processes;
};


Er::PropertyBag collectProcessDetails(Er::ProcFs::ProcFs& source, uint64_t pid, Er::ProcessProps::PropMask required);
Er::PropertyBag collectKernelDetails(Er::ProcFs::ProcFs& source, Er::ProcessProps::PropMask required);

std::unique_ptr<ProcessCollection> gatherProcessCollection(Er::ProcFs::ProcFs& source, Er::ProcessProps::PropMask required);


} // namespace Private {}

} // namespace Er {}