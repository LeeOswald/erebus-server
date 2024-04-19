#pragma once

#include <erebus/log.hxx>

#include "procmon.hxx"
#include "sched.bpf.h"
#include "sched.skel.h"

#include <bpf/libbpf.h>

#include <thread>

namespace Er
{

namespace Private
{


class ProcessSpy final
    : public Er::NonCopyable
{
public:
    ~ProcessSpy();
    explicit ProcessSpy(Er::Log::ILog* log);

private:
    static int staticHandleEvent(void* ctx, void* data, size_t size) noexcept;
    int handleEvent(const sched_event* ev) noexcept;
    void worker(std::stop_token stop) noexcept;

    static constexpr int PollTimeoutMs = 1000;
    Er::Log::ILog* m_log;
    sched_bpf* m_bpf = nullptr;
    struct ring_buffer* m_ringBuffer = nullptr;
    std::unique_ptr<std::jthread> m_worker;
};


} // namespace Private {}

} // namespace Er {}