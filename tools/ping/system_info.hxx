#pragma once

#include "runner_base.hxx"

#include <erebus/ipc/grpc/client/isystem_info_client.hxx>


class SystemInfoRunner
    : public RunnerBase
{
public:
    ~SystemInfoRunner() = default;

    SystemInfoRunner(IdleCallback&& idle, Er::Ipc::Grpc::ChannelPtr channel, unsigned parallel, unsigned iterations, bool wait, const std::string& pattern)
        : RunnerBase(std::move(idle), channel, parallel, iterations, wait)
        , m_pattern(pattern)
    {
    }

private:
    class SystemInfoCompletion
        : public Completion<Er::Ipc::Grpc::ISystemInfoClient::ISystemInfoCompletion>
    {
        using Base = Completion<Er::Ipc::Grpc::ISystemInfoClient::ISystemInfoCompletion>;
        
        struct PrivateTag {};

    public:
        SystemInfoCompletion(PrivateTag, RunnerBase* owner)
            : Base(owner)
        {
        }

        static auto make(RunnerBase* owner)
        {
            return Er::SharedPtr<ISystemInfoCompletion>{ new SystemInfoCompletion(PrivateTag{}, owner) };
        }

    private:
        Er::CallbackResult onProperty(Er::Property&& prop) override
        {
            ErLogInfo("{} = {}", prop.name(), prop.str());

            done();
            return Er::CallbackResult::Continue;
        }
    };

    void run(std::stop_token stop) noexcept
    {
        auto client = Er::DisposablePtr<Er::Ipc::Grpc::ISystemInfoClient>(Er::Ipc::Grpc::createSystemInfoClient(m_channel, Er::Log::global(), nullptr));
        auto n = m_iterations;

        do
        {
            if (n != unsigned(-1))
            {
                if (!n)
                    break;

                --n;
            }

            auto completion = SystemInfoCompletion::make(this);
            client->getSystemInfo(m_pattern, completion);

            if (m_wait)
            {
                auto w = completion->queryInterface<Er::IWaitable>();
                if (!w->wait(std::uint32_t(5000)))
                {
                    ErLogError("Completion timed out");
                }
            }

        } while (!stop.stop_requested());
    }

    std::string m_pattern;
};