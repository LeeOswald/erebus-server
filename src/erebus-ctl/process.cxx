#include "process.hxx"

#include <erebus/protocol.hxx>

#include <erebus/util/file.hxx>
#include <erebus/util/format.hxx>
#include <erebus-clt/erebus-clt.hxx>
#include <erebus-processmgr/processprops.hxx>



void dumpProcess(const Er::PropertyBag& info, Er::Log::ILog* log)
{
    auto it = Er::findProperty(info, Er::ProcessesGlobal::Global::Id::value);
    if (!it)
    {
        it = Er::findProperty(info, Er::ProcessProps::Pid::Id::value);
        if (!it)
        {
            log->write(Er::Log::Level::Error, ErLogNowhere(), "<invalid process>");
            return;
        }

        auto pid = std::get<uint64_t>(it->value);

        it = Er::findProperty(info, Er::ProcessProps::IsDeleted::Id::value);
        if (it)
        {
            log->write(Er::Log::Level::Error, ErLogNowhere(), "%zu { exited }", pid);
            return;
        }

        it = Er::findProperty(info, Er::ProcessProps::Valid::Id::value);
        if (!it)
        {
            log->write(Er::Log::Level::Error, ErLogNowhere(), "No data for PID %zu", pid);
            return;
        }

        auto valid = std::get<bool>(it->value);
        if (!valid)
        {
            it = Er::findProperty(info, Er::ProcessProps::Error::Id::value);
            if (it)
                log->write(Er::Log::Level::Error, ErLogNowhere(), "Invalid stat for PID %zu", pid);
            else
                log->write(Er::Log::Level::Error, ErLogNowhere(), "Invalid stat for PID %zu: %s", pid, std::get<std::string>(it->value).c_str());

            return;
        }
        
        log->write(Er::Log::Level::Info, ErLogNowhere(), "%zu {", pid);
    }
    else
    {
        log->write(Er::Log::Level::Info, ErLogNowhere(), "Global {");
    }

    Er::enumerateProperties(info, [log](const Er::Property& it)
    {
        if (
            (it.id != Er::ProcessProps::Valid::Id::value) &&
            (it.id != Er::ProcessProps::Error::Id::value) &&
            (it.id != Er::ProcessesGlobal::Global::Id::value)
            )
        {
            auto propInfo = Er::lookupProperty(it.id).get();
            if (!propInfo)
            {
                log->write(Er::Log::Level::Warning, ErLogNowhere(), "   0x%08x: ???", it.id);
            }
            else
            {
                std::ostringstream ss;
                propInfo->format(it, ss);

                log->write(Er::Log::Level::Info, ErLogNowhere(), "   %s: %s", propInfo->name(), ss.str().c_str());
            }
        }
    });

    log->write(Er::Log::Level::Info, ErLogNowhere(), "}");
}

void dumpProcess(Er::Client::IClient* client, Er::Log::ILog* log, int pid)
{
    protectedCall(
        log,
        [client, log, pid]()
        {
            Er::PropertyBag req;
            Er::insertProperty(req, Er::Property(Er::ProcessProps::Pid::Id::value, uint64_t(pid)));
            
            auto info = client->request(Er::ProcessRequests::ProcessDetails, req);
            dumpProcess(info, log);
        }
    );
}

void dumpProcess(Er::Log::ILog* log, const Er::Client::ChannelParams& params, int pid, int interval)
{
    protectedCall(
        log,
        [log, &params, pid, interval]()
        {
            auto channel = Er::Client::createChannel(params);
            auto client = Er::Client::createClient(channel, log);

            while (!g_signalReceived)
            {
                dumpProcess(client.get(), log, pid);

                if (interval <= 0)
                    break;

                log->write(Er::Log::Level::Info, ErLogNowhere(), "------------------------------------------------------");

                std::this_thread::sleep_for(std::chrono::seconds(interval));
            }
        }
    );
}

void dumpProcesses(Er::Client::IClient* client, Er::Log::ILog* log)
{
    protectedCall(
        log,
        [client, log]()
        {
            Er::PropertyBag req;
            auto list = client->requestStream(Er::ProcessRequests::ListProcesses, req);
            for (auto& process : list)
            {
                dumpProcess(process, log);
            }
        }
    );
}

void dumpProcesses(Er::Log::ILog* log, const Er::Client::ChannelParams& params, int interval)
{
    protectedCall(
        log,
        [log, &params, interval]()
        {
            auto channel = Er::Client::createChannel(params);
            auto client = Er::Client::createClient(channel, log);

            while (!g_signalReceived)
            {
                dumpProcesses(client.get(), log);

                if (interval <= 0)
                    break;

                log->write(Er::Log::Level::Info, ErLogNowhere(), "------------------------------------------------------");

                // globals
                {
                    Er::PropertyBag req;
                    auto globals = client->request(Er::ProcessRequests::ProcessesGlobal, req);
                    dumpPropertyBag(globals, log);
                }

                std::this_thread::sleep_for(std::chrono::seconds(interval));
            }
        }
    );
}

void dumpProcessesDiff(Er::Client::IClient* client, Er::Log::ILog* log, Er::Client::IClient::SessionId sessionId)
{
    protectedCall(
        log,
        [client, log, sessionId]()
        {
            Er::PropertyBag req;
            auto list = client->requestStream(Er::ProcessRequests::ListProcessesDiff, req, sessionId);
            for (auto& process : list)
            {
                dumpProcess(process, log);
            }

        }
    );
}

void dumpProcessesDiff(Er::Log::ILog* log, const Er::Client::ChannelParams& params, int interval)
{
    protectedCall(
        log,
        [log, &params, interval]()
        {
            auto channel = Er::Client::createChannel(params);
            auto client = Er::Client::createClient(channel, log);
            auto sessionId = client->beginSession(Er::ProcessRequests::ListProcessesDiff);

            while (!g_signalReceived)
            {
                dumpProcessesDiff(client.get(), log, sessionId);

                if (interval <= 0)
                    break;

                log->write(Er::Log::Level::Info, ErLogNowhere(), "------------------------------------------------------");

                std::this_thread::sleep_for(std::chrono::seconds(interval));
            }

            client->endSession(Er::ProcessRequests::ListProcessesDiff, sessionId);
        }
    );
}

void killProcess(Er::Log::ILog* log, const Er::Client::ChannelParams& params, uint64_t pid, const std::string& signame)
{
    protectedCall(
        log,
        [log, &params, pid, &signame]()
        {
            auto channel = Er::Client::createChannel(params);
            auto client = Er::Client::createClient(channel, log);
            
            Er::PropertyBag req;
            Er::insertProperty(req, Er::Property(Er::ProcessesGlobal::Pid::Id::value, pid));
            Er::insertProperty(req, Er::Property(Er::ProcessesGlobal::Signal::Id::value, signame));

            auto result = client->request(Er::ProcessRequests::KillProcess, req);

            dumpPropertyBag(result, log);
        }
    );
}


