#include <erebus/util/exceptionutil.hxx>

#include "process_spy.hxx"



namespace Er
{

namespace Private
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
        throw Er::Exception(ER_HERE(), "Failed to load the BPF object");

    m_ringBuffer.reset(::ring_buffer__new(::bpf_map__fd(m_bpf->maps.g_ringbuf), staticHandleEvent, this, nullptr));
    if (!m_ringBuffer)
        throw Er::Exception(ER_HERE(), "Failed to create a ring buffer");
    
    m_worker.reset(new std::jthread([this](std::stop_token stop){ worker(stop); }));

    attach();
}

void ProcessSpy::attach()
{
    assert(!m_attached);
    
    auto err = process_bpf::attach(m_bpf);
    if (err)
        throw Er::Exception(ER_HERE(), "Failed to attach to the BPF object");

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
    LogDebug(m_log, LogInstance("ProcessSpy"), "Worker started");

    while (!stop.stop_requested())
    {
        auto err = ::ring_buffer__poll(m_ringBuffer, PollTimeoutMs);
        if (err == -EINTR)
            break;

        if (err < 0) 
        {
            LogError(m_log, LogInstance("ProcessSpy"), "Error polling the ring buffer: %d", err);
            break;
        }
    }

    LogDebug(m_log, LogInstance("ProcessSpy"), "Worker exited");
}

int ProcessSpy::staticHandleEvent(void* ctx, void* data, size_t size) noexcept
{
    auto this_ = static_cast<ProcessSpy*>(ctx);
    assert(this_);

    return Er::protectedCall<int>(
        this_->m_log,
        LogComponent("ProcessSpy"),
        [this_, ctx, data, size]()
        {
            auto header = static_cast<const process_event_header_t*>(data);
            switch (header->type)
            {
            case PROCESS_EVENT_START: return this_->handleStart(static_cast<const process_event_start_t*>(data));
            case PROCESS_EVENT_RETVAL: return this_->handleRetval(static_cast<const process_event_retval_t*>(data));
            case PROCESS_EVENT_FILENAME: return this_->handleFilename(static_cast<const process_event_data_t*>(data));
            case PROCESS_EVENT_ARG: return this_->handleArg(static_cast<const process_event_data_t*>(data));
            }

            LogError(this_->m_log, LogComponent("ProcessSpy"), "Unknown process event type %d", header->type);
            return 0;
        }
    );
}

int ProcessSpy::handleStart(const process_event_start_t* ev)
{
    auto process = std::make_shared<ProcessInfo>(ev);
    auto r = m_runningProcesses.insert({ uint64_t(ev->header.pid), process });
    if (!r.second)
        LogWarning(m_log, LogComponent("ProcessSpy"), "Reusing existing PID %zu", ev->header.pid);

    m_starting = r.first->second;
    return 0;
}

int ProcessSpy::handleRetval(const process_event_retval_t* ev)
{
    auto current = lookupCurrent(ev->header.pid);
    if (current) [[likely]]
    {
        current->retVal = ev->retval;
        auto p = current;
        current.reset();

        issueProcessStart(p);
    }

    return 0;
}

std::shared_ptr<ProcessSpy::ProcessInfo> ProcessSpy::lookupCurrent(uint64_t pid)
{
    auto current = m_starting;
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
            LogWarning(m_log, LogComponent("ProcessSpy"), "Non-existing PID %zu", pid);
            return std::shared_ptr<ProcessInfo>();
        }

        current = it->second;
    }

    return current;
}

int ProcessSpy::handleFilename(const process_event_data_t* ev)
{
    auto current = lookupCurrent(ev->header.pid);
    if (current) [[likely]]
    {
        current->fileName.assign(ev->data);
    }

    return 0;
}

int ProcessSpy::handleArg(const process_event_data_t* ev)
{
    auto current = lookupCurrent(ev->header.pid);
    if (current) [[likely]]
    {
        current->argv.append(" ");
        current->argv.append(ev->data);
    }

    return 0;
}
    
void ProcessSpy::issueProcessStart(std::shared_ptr<ProcessInfo> info)
{
    LogInfo(m_log, LogNowhere(), "%d EXECVE pid=%zu; ppid=%zu; uid=%zu; sid=%zu; [%s] [%s] %s", int(info->retVal.value_or(-1)), info->pid, info->ppid, info->uid, info->sid, info->comm.c_str(), info->fileName.c_str(), info->argv.c_str());
}

} // namespace Private {}

} // namespace Er {}