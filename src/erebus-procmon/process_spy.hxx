#pragma once

#include <erebus/log.hxx>
#include <erebus/util/autoptr.hxx>

#include "procmon.hxx"
#include "process.bpf.h"
#include "process.skel.h"

#include <bpf/libbpf.h>

#include <thread>
#include <unordered_map>

namespace Erp
{

namespace Procmon
{


class ProcessSpy final
    : public Er::NonCopyable
{
public:
    ~ProcessSpy();
    explicit ProcessSpy(Er::Log::ILog* log);

    void attach();
    void detach();

private:
    struct RingBufferDeleter final
    {
        void operator()(ring_buffer* p) { ::ring_buffer__free(p); }
    };

    using RingBufferHolder = Er::Util::AutoPtr<ring_buffer, RingBufferDeleter>;

    struct ProcessBpfDeleter final
    {
        void operator()(process_bpf* p) { process_bpf::destroy(p); }
    };

    using ProcessBpfHolder = Er::Util::AutoPtr<process_bpf, ProcessBpfDeleter>;

    struct ProcessInfo final
    {
        uint64_t pid = uint64_t(-1);
        uint64_t ppid = uint64_t(-1);
        uint64_t uid = uint64_t(-1);
        uint64_t sid = uint64_t(-1);
        uint64_t startTime = 0;
        std::string comm;
        std::string fileName;
        std::string argv;
        
        ProcessInfo() noexcept = default;

        ProcessInfo(const process_event_execve_enter_t* ev)
            : pid(ev->header.pid)
            , ppid(ev->ppid)
            , uid(ev->uid)
            , sid(ev->sid)
            , startTime(ev->start_time)
            , comm(ev->comm)
        {}
    };

    static int staticHandleEvent(void* ctx, void* data, size_t size) noexcept;
    std::shared_ptr<ProcessInfo> lookupCurrentExecve(uint64_t pid);
    int handleExecveEnter(const process_event_execve_enter_t* ev);
    int handleExecveRetval(const process_event_retval_t* ev);
    int handleExecveFilename(const process_event_data_t* ev);
    int handleExecveArg(const process_event_data_t* ev);
    int handleExit(const process_event_exit_t* ev);
    int handleFork(const process_event_fork_t* ev);
    
    void worker(std::stop_token stop) noexcept;
    
    void issueExecve(std::shared_ptr<ProcessInfo> info, uint64_t retVal);
    void issueTaskExit(std::shared_ptr<ProcessInfo> info, int32_t exitCode, uint64_t pid, uint64_t tid);

    static constexpr int PollTimeoutMs = 1000;
    Er::Log::ILog* m_log;
    ProcessBpfHolder m_bpf;
    RingBufferHolder m_ringBuffer;
    bool m_attached = false;
    std::unique_ptr<std::jthread> m_worker;
    std::unordered_map<uint64_t, std::shared_ptr<ProcessInfo>> m_runningProcesses;
    std::shared_ptr<ProcessInfo> m_currentExecve;
};


} // namespace Procmon {}

} // namespace Erp {}