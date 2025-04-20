#pragma once


#include <erebus/ipc/grpc/grpc_client.hxx>
#include <erebus/rtl/log.hxx>
#include <erebus/rtl/util/exception_util.hxx>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>


class RunnerBase
{
public:
    ~RunnerBase() = default;

    RunnerBase(Er::Ipc::Grpc::ChannelPtr channel, unsigned parallel, unsigned iterations, bool wait)
        : m_channel(channel)
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

        std::mutex m_mutex;
        std::condition_variable m_cv;
        bool m_done = false;
    };

    virtual void run(std::stop_token stop) = 0;

    void _run(unsigned index, std::stop_token stop) noexcept
    {
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
    }

    Er::Ipc::Grpc::ChannelPtr m_channel;
    bool m_wait;
    unsigned m_iterations;
    std::vector<std::unique_ptr<std::jthread>> m_threads;
};