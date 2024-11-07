#pragma once

#include "common.hxx"

#include <erebus/stopwatch.hxx>

#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>


class StreamRunner final
    : public Er::Client::IClient::IStreamReceiver
{
public:
    ~StreamRunner() = default;

    StreamRunner(
        Er::Log::ILog* log,
        Er::Event* exitEvent,
        Er::Client::ChannelPtr channel,
        const std::string& request,
        const std::string& domain,
        const Er::PropertyBag& args,
        int interval,
        int threads
    )
        : m_log(log)
        , m_exitEvent(exitEvent)
        , m_channel(channel)
        , m_request(request)
        , m_domain(domain)
        , m_args(args)
        , m_interval(interval)
        , m_threadCount(threads)
    {
        m_workers.reserve(threads);
        for (int i = 0; i < threads; ++i)
        {
            m_workers.push_back(std::jthread([this](std::stop_token stop) { run(stop); }));
        }
    }

private:
    struct Request
    {
        Er::Stopwatch<> sw;

        Request()
        {
            sw.start();
        }
    };

    Request* pick(Er::Client::IClient::CallId callId)
    {
        std::lock_guard l(m_mutex);

        auto it = m_pending.find(callId);
        if (it != m_pending.end())
        {
            return &it->second;
        }
    
        return nullptr;
    }

    void remove(Er::Client::IClient::CallId callId)
    {
        std::lock_guard l(m_mutex);

        auto it = m_pending.find(callId);
        if (it != m_pending.end())
        {
            m_pending.erase(it);
        }
    }


    Er::Client::IClient::CallbackResult receive(Er::Client::IClient::CallId callId, Er::PropertyBag&& result) override
    {
        auto r = pick(callId);

        if (!r)
        {
            Er::Log::error(m_log, "Unexpected CallID {}", callId);
        }
        else
        {
            r->sw.stop();
            std::lock_guard l(bulkLogWrite());

            Er::Log::write(m_log, Er::Log::Level::Info, "ID: {}; RTT: {} ms", callId, r->sw.value().count());

            dumpPropertyBag(m_domain, result, m_log);

            Er::Log::writeln(m_log, Er::Log::Level::Info, "------------------------------------------------------");
            r->sw.start();
        }

        return Er::Client::IClient::CallbackResult::Continue;
    }

    Er::Client::IClient::CallbackResult receive(Er::Client::IClient::CallId callId, Er::Exception&& exception) override
    {
        auto r = pick(callId);

        if (!r)
        {
            Er::Log::error(m_log, "Unexpected CallID {}", callId);
        }
        else
        {
            r->sw.stop();
            std::lock_guard l(bulkLogWrite());

            Er::Log::write(m_log, Er::Log::Level::Info, "ID: {}; RTT: {} ms", callId, r->sw.value().count());

            Er::Util::logException(m_log, Er::Log::Level::Error, exception);

            Er::Log::writeln(m_log, Er::Log::Level::Info, "------------------------------------------------------");
            r->sw.start();
        }

        return Er::Client::IClient::CallbackResult::Continue;
    }

    void finish(Er::Client::IClient::CallId callId, Er::Result result, std::string&& message) override
    {
        remove(callId);

        Er::Log::error(m_log, "gRPC error {} ({})", int(result), message);
        m_finished.store(true, std::memory_order_release);
    }

    void finish(Er::Client::IClient::CallId callId) override
    {
        remove(callId);

        Er::Log::info(m_log, "ID: {}L End of stream", callId);
        m_finished.store(true, std::memory_order_release);
    }

    void run(std::stop_token stop)
    {
        auto finished = Er::protectedCall<bool>(
            m_log,
            [this, &stop]()
            {
                return runImpl(stop);
            });

        if (!stop.stop_requested())
        {
            if (!finished)
                Er::Log::error(m_log, "Worker exited unexpectedly");

            if (--m_threadCount == 0)
            {
                Er::Log::warning(m_log, "No active workers left");
                if (m_exitEvent)
                    m_exitEvent->setAndNotifyAll(true);
            }
        }
    }

    bool runImpl(std::stop_token stop)
    {
        auto client = Er::Client::createClient(m_channel, m_log);

        while (!stop.stop_requested())
        {
            auto callId = m_nextCallId++;
            {
                std::lock_guard l(m_mutex);
                m_pending.insert({ callId, {} });
            }

            client->requestStream(callId, m_request, m_args, this);

            if (m_interval <= 0)
                break;

            std::this_thread::sleep_for(std::chrono::seconds(m_interval));

            if (m_finished.load(std::memory_order_acquire) == true)
            {
                return false;
            }
        }

        return true;
    }

    auto reader(std::stop_token stop, Er::Stopwatch<>& sw, Er::PropertyBag&& item) -> Er::Client::IClient::CallbackResult
    {
        if (stop.stop_requested())
            return Er::Client::IClient::CallbackResult::Break;

        sw.stop();

        {
            std::lock_guard l(bulkLogWrite());
            dumpPropertyBag(m_domain, item, m_log);
            
            Er::Log::writeln(m_log, Er::Log::Level::Info, "------------------------------------------------------");
        }
    
        return Er::Client::IClient::CallbackResult::Continue;
    }

    Er::Log::ILog* const m_log;
    Er::Event* m_exitEvent;
    Er::Client::ChannelPtr const m_channel;
    std::string const m_request;
    std::string const m_domain;
    Er::PropertyBag const m_args;
    int const m_interval;
    int m_threadCount;
    std::vector<std::jthread> m_workers;

    std::atomic_bool m_finished = false;
    std::atomic<Er::Client::IClient::CallId> m_nextCallId = 0;
    std::mutex m_mutex;
    std::unordered_map<Er::Client::IClient::CallId, Request> m_pending;
};