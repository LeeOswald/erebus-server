#pragma once

#include <erebus-processmgr/erebus-processmgr.hxx>

#include "procfs.hxx"

#include <chrono>
#include <unordered_map>
#include <vector>


namespace Erp::ProcessMgr
{


class ProcessListCollector final
    : public Er::NonCopyable
{
public:
    using PropMask = Er::ProcessMgr::ProcessProps::PropMask;

    struct ProcessInfo
        : public Er::NonCopyable
    {
        using Clock = std::chrono::steady_clock;

        uint64_t pid;
        uint64_t ppid = uint64_t(-1);
        bool isNew;
        Clock::time_point timestamp;
        Er::PropertyBag properties;
        std::string comm;
        std::string exe;
        double stime = 0.0;
        double utime = 0.0;
        
        explicit ProcessInfo(uint64_t pid, bool isNew, Clock::time_point timestamp) noexcept
            : pid(pid)
            , isNew(isNew)
            , timestamp(timestamp)
        {}
    };

    using ProcessInfoCollection = std::unordered_map<uint64_t, std::shared_ptr<ProcessInfo>>;

    struct ProcessInfoDiff
    {
        std::shared_ptr<ProcessInfo> process;
        Er::PropertyBag properties;

        explicit ProcessInfoDiff(std::shared_ptr<ProcessInfo> process) noexcept
            : process(process)
        {}
    };

    struct ProcessInfoCollectionDiff
    {
        bool firstRun = false;
        std::size_t processCount = 0;
        std::vector<std::shared_ptr<ProcessInfo>> removed;
        std::vector<ProcessInfoDiff> modified;
        std::vector<std::shared_ptr<ProcessInfo>> added;
        double sTimeTotal = 0.0;
        double uTimeTotal = 0.0;
    };

    explicit ProcessListCollector(Er::Log::ILog* log, ProcFs& procFs);

    ProcessInfoCollectionDiff update(PropMask required);

private:
    void updateProcess(ProcessInfoCollectionDiff& diff, PropMask required, uint64_t pid, std::shared_ptr<ProcessInfo> info);
    void updateKernelProcess(ProcessInfoCollectionDiff& diff, PropMask required, std::shared_ptr<ProcessInfo> info);

    Er::Log::ILog* const m_log;
    ProcFs& m_procFs;
    bool m_firstRun = true;
    PropMask m_required;
    ProcessInfoCollection m_collection;
};


} // namespace Erp::ProcessMgr {}
