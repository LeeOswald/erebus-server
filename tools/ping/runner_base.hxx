#pragma once


#include <erebus/ipc/grpc/client/grpc_client.hxx>
#include <erebus/rtl/log.hxx>
#include <erebus/rtl/util/exception_util.hxx>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>


class RunnerBase
{
public:
    using IdleCallback = std::function<void()>;

    ~RunnerBase() = default;

    RunnerBase(IdleCallback&& idle, Er::Ipc::Grpc::ChannelPtr channel, unsigned parallel, unsigned iterations, bool wait)
        : m_idle(std::move(idle))
        , m_channel(channel)
        , m_iterations(iterations)
        , m_wait(wait)
    {
        ErAssert(parallel > 0);
        m_threads.reserve(parallel);

        for (unsigned i = 0; i < parallel; ++i)
            m_threads.push_back(std::make_unique<std::jthread>([this, i](std::stop_token stop) { _run(i, stop); }));
    }

protected:
    template <class Interface>
    struct Completion
        : public Interface
    {
        ~Completion()
        {
            m_owner->idle();
        }

        Completion(RunnerBase* owner)
            : m_owner(owner)
        {
            owner->busy();
        }

        void onError(Er::ResultCode result, std::string&& message) noexcept override
        {
            ErLogError("Request completed with an error {}: {}", int(result), message);

            done();
        }

        bool wait(std::chrono::seconds timeout = std::chrono::seconds(5))
        {
            std::unique_lock l(m_mutex);
            return m_cv.wait_for(l, timeout, [this]() { return m_done; });
        }

    protected:
        void done()
        {
            {
                std::lock_guard l(m_mutex);
                m_done = true;
            }

            m_cv.notify_one();
        }

        RunnerBase* m_owner;
        std::mutex m_mutex;
        std::condition_variable m_cv;
        bool m_done = false;
    };

    virtual void run(std::stop_token stop) = 0;

    void _run(unsigned index, std::stop_token stop) noexcept
    {
        busy();

        ErLogDebug("Worker {} started", index);

        Er::Util::ExceptionLogger xcptHandler(Er::Log::get());
        try
        {
            run(stop);
        }
        catch (...)
        {
            Er::dispatchException(std::current_exception(), xcptHandler);
        }

        ErLogDebug("Worker {} stopped", index);
        idle();
    }

    void busy() noexcept
    {
        ++m_busy;
    }

    void idle() noexcept
    {
        auto prev = m_busy.fetch_sub(1, std::memory_order_acq_rel);
        if (prev == 1) // this was the last reference
            m_idle();
    }

    IdleCallback m_idle;
    std::atomic<long> m_busy = 0;
    Er::Ipc::Grpc::ChannelPtr m_channel;
    bool m_wait;
    unsigned m_iterations;
    std::vector<std::unique_ptr<std::jthread>> m_threads;
};