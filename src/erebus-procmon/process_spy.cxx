#include <erebus/util/exceptionutil.hxx>

#include "process_spy.hxx"



namespace Er
{

namespace Private
{

ProcessSpy::~ProcessSpy()
{
    if (m_worker)
        m_worker.reset();

    if (m_bpf)
        sched_bpf::detach(m_bpf);

    ::ring_buffer__free(m_ringBuffer);
    sched_bpf::destroy(m_bpf);
}

ProcessSpy::ProcessSpy(Er::Log::ILog* log)
    : m_log(log)
    , m_bpf(sched_bpf::open_and_load())
{
    if (!m_bpf)
        throw Er::Exception(ER_HERE(), "Failed to load the BPF object");

    auto err = sched_bpf::attach(m_bpf);
    if (err)
    {
        sched_bpf::destroy(m_bpf);
        throw Er::Exception(ER_HERE(), "Failed to attach to the BPF object");
    }

    m_ringBuffer = ::ring_buffer__new(::bpf_map__fd(m_bpf->maps.rb), staticHandleEvent, this, nullptr);
    if (!m_ringBuffer)
    {
        sched_bpf::detach(m_bpf);
        sched_bpf::destroy(m_bpf);
        throw Er::Exception(ER_HERE(), "Failed to create a ring buffer");
    }

    m_worker.reset(new std::jthread([this](std::stop_token stop){ worker(stop); }));
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

    auto ev = static_cast<const sched_event*>(data);
    return this_->handleEvent(ev);
}

int ProcessSpy::handleEvent(const sched_event* ev) noexcept
{
    return Er::protectedCall<int>(
        m_log,
        LogInstance("ProcessSpy"),
        [this, ev]()
        {
            if (ev->exit_event) 
            {
                LogInfo(m_log, LogInstance("ProcessSpy"), "%-5s %-16s %-7d %-7d [%u]", "EXIT", ev->comm, ev->pid, ev->ppid, ev->exit_code);
            } 
            else 
            {
                LogInfo(m_log, LogInstance("ProcessSpy"), "%-5s %-16s %-7d %-7d %s\n", "EXEC", ev->comm, ev->pid, ev->ppid, ev->filename);
            }

            return 0;
        }
    );
}

} // namespace Private {}

} // namespace Er {}