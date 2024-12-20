#include <erebus/system/thread.hxx>
#include <erebus/util/exceptionutil.hxx>

#include "process_spy.hxx"



namespace Erp::Procmon
{

ProcessSpy::~ProcessSpy()
{
    detach();

    if (m_worker)
        m_worker.reset();
}

ProcessSpy::ProcessSpy(Er::Log::ILog* log)
    : m_log(log)
    , m_bpf(process_bpf::open_and_load())
{
    if (!m_bpf)
        ErThrow("Failed to load the BPF object");

    m_ringBuffer.reset(::ring_buffer__new(::bpf_map__fd(m_bpf->maps.g_ringbuf), staticHandleEvent, this, nullptr));
    if (!m_ringBuffer)
        ErThrow("Failed to create a ring buffer");
    
    m_worker.reset(new std::jthread([this](std::stop_token stop){ worker(stop); }));

    attach();
}

void ProcessSpy::attach()
{
    ErAssert(!m_attached);
    
    auto err = process_bpf::attach(m_bpf);
    if (err)
        ErThrow("Failed to attach to the BPF object");

    m_attached = true;
}

void ProcessSpy::detach()
{
    if (m_attached)
    {
        process_bpf::detach(m_bpf);
        m_attached = false;
    }
}

void ProcessSpy::worker(std::stop_token stop) noexcept
{
    Er::System::CurrentThread::setName("ProcmonWorker");
    
    Er::Log::debug(m_log, "Worker started");

    while (!stop.stop_requested())
    {
        auto err = ::ring_buffer__poll(m_ringBuffer, PollTimeoutMs);
        if (err == -EINTR)
        {
            Er::Log::error(m_log, "Resuming after EINTR");
            continue;
        }

        if (err < 0) 
        {
            Er::Log::error(m_log, "Error polling the ring buffer: {}", err);
            break;
        }
    }

    Er::Log::debug(m_log, "Worker exited");
}

int ProcessSpy::staticHandleEvent(void* ctx, void* data, size_t size) noexcept
{
    auto this_ = static_cast<ProcessSpy*>(ctx);
    ErAssert(this_);

    return Er::protectedCall<int>(
        this_->m_log,
        [this_, ctx, data, size]()
        {
            auto header = static_cast<const process_event_header_t*>(data);
            switch (header->type)
            {
            case PROCESS_EVENT_EXECVE_ENTER: return this_->handleExecveEnter(static_cast<const process_event_execve_enter_t*>(data));
            case PROCESS_EVENT_EXECVE_RETVAL: return this_->handleExecveRetval(static_cast<const process_event_retval_t*>(data));
            case PROCESS_EVENT_EXECVE_FILENAME: return this_->handleExecveFilename(static_cast<const process_event_data_t*>(data));
            case PROCESS_EVENT_EXECVE_ARG: return this_->handleExecveArg(static_cast<const process_event_data_t*>(data));
            case PROCESS_EVENT_EXIT: return this_->handleExit(static_cast<const process_event_exit_t*>(data));
            case PROCESS_EVENT_FORK: return this_->handleFork(static_cast<const process_event_fork_t*>(data));
            }

            Er::Log::error(this_->m_log, "Unknown process event type {}", int(header->type));
            return 0;
        }
    );
}

int ProcessSpy::handleExecveEnter(const process_event_execve_enter_t* ev)
{
    auto process = std::make_shared<ProcessInfo>(ev);
    auto r = m_runningProcesses.insert({ uint64_t(ev->header.pid), process });
    if (!r.second)
        Er::Log::warning(m_log, "Reusing existing PID {}", ev->header.pid);

    m_currentExecve = r.first->second;
    return 0;
}

int ProcessSpy::handleExecveRetval(const process_event_retval_t* ev)
{
    auto current = lookupCurrentExecve(ev->header.pid);
    if (current) [[likely]]
    {
        auto p = current;
        current.reset();

        if (ev->retval < 0)
        {
            // execve() failed, don't expect exit()
            auto it = m_runningProcesses.find(ev->header.pid);
            if (it != m_runningProcesses.end())
                m_runningProcesses.erase(it);
        
        }

        issueExecve(p, ev->retval);
    }

    return 0;
}

std::shared_ptr<ProcessSpy::ProcessInfo> ProcessSpy::lookupCurrentExecve(uint64_t pid)
{
    auto current = m_currentExecve;
    if (current) [[likely]]
    {
        if (current->pid != pid) [[unlikely]]
        {
            current.reset();
        }
    }

    if (!current) [[unlikely]]
    {
        auto it = m_runningProcesses.find(pid);
        if (it == m_runningProcesses.end())
        {
            Er::Log::warning(m_log, "Non-existing PID {}", pid);
            return std::shared_ptr<ProcessInfo>();
        }

        current = it->second;
    }

    return current;
}

int ProcessSpy::handleExecveFilename(const process_event_data_t* ev)
{
    auto current = lookupCurrentExecve(ev->header.pid);
    if (current) [[likely]]
    {
        current->fileName.assign(ev->data);
    }

    return 0;
}

int ProcessSpy::handleExecveArg(const process_event_data_t* ev)
{
    auto current = lookupCurrentExecve(ev->header.pid);
    if (current) [[likely]]
    {
        current->argv.append(" ");
        current->argv.append(ev->data);
    }

    return 0;
}

int ProcessSpy::handleExit(const process_event_exit_t* ev)
{
    std::shared_ptr<ProcessInfo> process;
    auto it = m_runningProcesses.find(ev->header.pid);
    if (it != m_runningProcesses.end())
    {
        process = it->second;

        m_runningProcesses.erase(it);
    }

    issueTaskExit(process, ev->exit_code, ev->header.pid, ev->tid);

    return 0;
}

int ProcessSpy::handleFork(const process_event_fork_t* ev)
{

    Er::Log::info(m_log, "FORK parent_pid={}; parent_comm=[{}]; child_pid={}}; child_comm=[{}]", ev->parent_pid, ev->parent_comm, ev->child_pid, ev->child_comm);
    
    return 0;
}
    
void ProcessSpy::issueExecve(std::shared_ptr<ProcessInfo> info, uint64_t retVal)
{
    Er::Log::info(m_log, "{} EXECVE pid={}; ppid={}; uid={}; sid={}; [{}] [{}] {}", int(retVal), info->pid, info->ppid, info->uid, info->sid, info->comm, info->fileName, info->argv);
}

void ProcessSpy::issueTaskExit(std::shared_ptr<ProcessInfo> info, int32_t exitCode, uint64_t pid, uint64_t tid)
{
    if (info)
    {
        if (pid == tid)
            Er::Log::info(m_log, "EXIT pid={}; [{}] -> {}", pid, info->comm, exitCode);
        else
            Er::Log::info(m_log, "THREAD EXIT pid={}; tid={}; [{}] -> {}", pid, tid, info->comm, exitCode);
    }
    else
    {
        if (pid == tid)
            Er::Log::info(m_log, "EXIT pid={} -> {}", pid, exitCode);
        else
            Er::Log::info(m_log, "THREAD EXIT pid={}; tid={} -> {}", pid, tid, exitCode);
    }
}

} // namespace Erp::Procmon {}