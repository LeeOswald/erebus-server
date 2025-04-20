#pragma once


#include <erebus/ipc/grpc/grpc_client.hxx>
#include <erebus/rtl/log.hxx>
#include <erebus/rtl/util/exception_util.hxx>

#include <thread>
#include <vector>


class RunnerBase
{
public:
    ~RunnerBase() = default;

    RunnerBase(Er::Ipc::Grpc::ChannelPtr channel, unsigned parallel)
        : m_channel(channel)
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
        }
    };

    virtual void run(std::stop_token stop) = 0;

    void _run(unsigned index, std::stop_token stop) noexcept
    {
        ErLogInfo("Worker {} started", index);

        Er::Util::ExceptionLogger xcptHandler(Er::Log::get());
        try
        {
            run(stop);
        }
        catch (...)
        {
            Er::dispatchException(std::current_exception(), xcptHandler);
        }

        ErLogInfo("Worker {} stopped", index);
    }

    Er::Ipc::Grpc::ChannelPtr m_channel;
    std::vector<std::unique_ptr<std::jthread>> m_threads;
};