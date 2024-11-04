#pragma once

#include "common.hxx"

#include <erebus/condition.hxx>
#include <erebus/stopwatch.hxx>

#include <thread>
#include <vector>


class RequestRunner final
{
public:
    ~RequestRunner() = default;

    RequestRunner(
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

        Er::PropertyBag result;

        Er::Stopwatch<> sw;
        {
            Er::Stopwatch<>::Scope sws(sw);
            result = client->request(m_request, m_args);
        }

        while (!stop.stop_requested())
        {
            {
                std::lock_guard l(bulkLogWrite());

                dumpPropertyBag(m_domain, result, m_log);
                Er::Log::write(m_log, Er::Log::Level::Info, "RTT: {} ms", sw.value().count());

                if (m_interval <= 0)
                    break;

                Er::Log::writeln(m_log, Er::Log::Level::Info, "------------------------------------------------------");
            }

            sw.reset();

            std::this_thread::sleep_for(std::chrono::seconds(m_interval));

            {
                Er::Stopwatch<>::Scope sws(sw);
                result = client->request(m_request, m_args);
            }
        }

        return true;
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
};