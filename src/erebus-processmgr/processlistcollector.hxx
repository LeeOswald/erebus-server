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


class ProcessListCollector final
    : public Er::NonCopyable
{
public:
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

    using ProcessCollection = std::unordered_map<uint64_t, std::unique_ptr<ProcessData>>;
};


} // namespace ProcessMgr {}

} // namespace Erp {}