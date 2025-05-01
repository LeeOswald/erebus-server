#pragma once

#include "runner_base.hxx"

#include <erebus/ipc/grpc/client/isystem_info_client.hxx>
#include <erebus/rtl/time.hxx>

#include <atomic>
#include <random>

class PingRunner
    : public RunnerBase
{
public:
    ~PingRunner() = default;

    PingRunner(IdleCallback&& idle, Er::Ipc::Grpc::ChannelPtr channel, unsigned parallel, unsigned iterations, bool wait, unsigned payloadSize)
        : RunnerBase(std::move(idle), channel, parallel, iterations, wait)
        , m_payloadSize(payloadSize)
    {
    }

private:
    class PingCompletion
        : public Completion<Er::Ipc::Grpc::ISystemInfoClient::IPingCompletion>
    {
        using Base = Completion<Er::Ipc::Grpc::ISystemInfoClient::IPingCompletion>;

        struct PrivateTag {};

    public:
        ~PingCompletion()
        {
            ErLogDebug("{}.PingCompletion::~PingCompletion()", Er::Format::ptr(this));
        }

        PingCompletion(PrivateTag, RunnerBase* owner)
            : Base(owner)
        {
            ErLogDebug("{}.PingCompletion::PingCompletion()", Er::Format::ptr(this));
        }

        static auto make(RunnerBase* owner)
        {
            return Er::SharedPtr<IPingCompletion>{ new PingCompletion(PrivateTag{}, owner) };
        }

    private:
        void onReply(Er::Ipc::Grpc::ISystemInfoClient::PingMessage&& ping, Er::Ipc::Grpc::ISystemInfoClient::PingMessage&& reply) override
        {
            auto now = Er::Time::now();
            if (ping.sequence != reply.sequence)
            {
                ErLogError("Ping seq # was {} while reply seq is #{}", ping.sequence, reply.sequence);
            }
            else if (ping.payload.size() != reply.payload.size())
            {
                ErLogError("Ping payload size was {} while reply payload size is {}", ping.sequence, reply.sequence);
            }
            else if (ping.payload != reply.payload)
            {
                ErLogError("Ping payload data is corrupted");
            }
            else
            {
                auto then = reply.timestamp.value();

                auto delta = now - then;
                if (delta > 1999)
                    ErLogInfo("Pinged target with {} bytes of data with RTT of {} ms", ping.payload.size(), (now - then) / 1000);
                else
                    ErLogInfo("Pinged target with {} bytes of data with RTT of {} \u03bcs", ping.payload.size(), (now - then));
            }

            done();
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

            Er::Ipc::Grpc::ISystemInfoClient::PingMessage pm;
            pm.payload = makePayload(m_payloadSize);
            pm.timestamp = Er::Time::now();
            pm.sequence = m_sequence.fetch_add(1, std::memory_order_relaxed);

            auto completion = PingCompletion::make(this);

            client->ping(std::move(pm), completion);
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

    static Er::Binary makePayload(unsigned size)
    {
        static const char ValidChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";
        static const size_t ValidCharsCount = sizeof(ValidChars) - 1;

        std::uniform_int_distribution<> charDistrib(0, ValidCharsCount - 1);

        static std::random_device rd;
        std::mt19937 random(rd());

        std::string data(size, ' ');
        for (size_t i = 0; i < size; ++i)
            data[i] = ValidChars[charDistrib(random)];

        return Er::Binary{ std::move(data) };
    }
        
    unsigned m_payloadSize;
    std::atomic<std::uint64_t> m_sequence = 0;
};