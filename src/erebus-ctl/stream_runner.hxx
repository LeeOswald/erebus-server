#pragma once

#include "common.hxx"

#include <erebus/stopwatch.hxx>

#include <thread>
#include <vector>


class StreamRunner final
{
public:
    ~StreamRunner() = default;

    StreamRunner(
        Er::Log::ILog* log,
        Er::Client::ChannelPtr channel,
        const std::string& request,
        const std::string& domain,
        const Er::PropertyBag& args,
        int interval,
        int threads
    )
        : m_log(log)
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
                Er::Log::warning(m_log, "No active workers left");
        }
    }

    bool runImpl(std::stop_token stop)
    {
        auto client = Er::Client::createClient(m_channel, m_log);

        Er::Stopwatch<> sw;
        sw.start();

        client->requestStream(
            m_request,
            m_args,
            [this, &sw, &stop](Er::PropertyBag&& item) -> bool
            {
                if (stop.stop_requested())
                    return false;

                sw.stop();

                {
                    std::lock_guard l(bulkLogWrite());
                    dumpPropertyBag(m_domain, item, m_log);
                    Er::Log::write(m_log, Er::Log::Level::Info, "Time delta: {} ms", sw.value().count());
                    Er::Log::writeln(m_log, Er::Log::Level::Info, "------------------------------------------------------");
                }

                sw.reset();
                sw.start();
                return true;
            });

        return true;
    }

    Er::Log::ILog* const m_log;
    Er::Client::ChannelPtr const m_channel;
    std::string const m_request;
    std::string const m_domain;
    Er::PropertyBag const m_args;
    int const m_interval;
    int m_threadCount;
    std::vector<std::jthread> m_workers;
};