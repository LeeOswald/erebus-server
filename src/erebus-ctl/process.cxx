#include "process.hxx"

#include <erebus/protocol.hxx>

#include <erebus/util/file.hxx>
#include <erebus/util/format.hxx>
#include <erebus-clt/erebus-clt.hxx>
#include <erebus-processmgr/processprops.hxx>



void dumpProcess(const Er::PropertyBag& info, Er::Log::ILog* log)
{
    auto it = info.find(Er::ProcessesGlobal::Global::Id::value);
    if (it == info.end())
    {
        it = info.find(Er::ProcessProps::Pid::Id::value);
        if (it == info.end())
        {
            log->write(Er::Log::Level::Error, ErLogNowhere(), "<invalid process>");
            return;
        }

        auto pid = std::get<uint64_t>(it->second.value);

        it = info.find(Er::ProcessProps::IsDeleted::Id::value);
        if (it != info.end())
        {
            log->write(Er::Log::Level::Error, ErLogNowhere(), "%zu { exited }", pid);
            return;
        }

        it = info.find(Er::ProcessProps::Valid::Id::value);
        if (it == info.end())
        {
            log->write(Er::Log::Level::Error, ErLogNowhere(), "No data for PID %zu", pid);
            return;
        }

        auto valid = std::get<bool>(it->second.value);
        if (!valid)
        {
            it = info.find(Er::ProcessProps::Error::Id::value);
            if (it != info.end())
                log->write(Er::Log::Level::Error, ErLogNowhere(), "Invalid stat for PID %zu", pid);
            else
                log->write(Er::Log::Level::Error, ErLogNowhere(), "Invalid stat for PID %zu: %s", pid, std::get<std::string>(it->second.value).c_str());

            return;
        }
        
        log->write(Er::Log::Level::Info, ErLogNowhere(), "%zu {", pid);
    }
    else
    {
        log->write(Er::Log::Level::Info, ErLogNowhere(), "Global {");
    }

    for (auto it = info.begin(); it != info.end(); ++it)
    {
        if (
            (it->second.id != Er::ProcessProps::Valid::Id::value) &&
            (it->second.id != Er::ProcessProps::Error::Id::value) &&
            (it->second.id != Er::ProcessesGlobal::Global::Id::value)
            )
        {
            auto propInfo = Er::lookupProperty(it->second.id).get();
            if (!propInfo)
            {
                log->write(Er::Log::Level::Warning, ErLogNowhere(), "   0x%08x: ???", it->second.id);
            }
            else
            {
                std::ostringstream ss;
                propInfo->format(it->second, ss);

                log->write(Er::Log::Level::Info, ErLogNowhere(), "   %s: %s", propInfo->name(), ss.str().c_str());
            }
        }
    }

    log->write(Er::Log::Level::Info, ErLogNowhere(), "}");
}

void dumpProcess(Er::Client::IClient* client, Er::Log::ILog* log, int pid)
{
    protectedCall(
        log,
        [client, log, pid]()
        {
            Er::PropertyBag req;
            req.insert({ Er::ProcessProps::Pid::Id::value, Er::Property(Er::ProcessProps::Pid::Id::value, uint64_t(pid)) });
            auto info = client->request(Er::ProcessRequests::ProcessDetails, req);
            dumpProcess(info, log);
        }
    );
}

void dumpProcess(Er::Log::ILog* log, const Er::Client::Params& params, int pid, int interval)
{
    protectedCall(
        log,
        [log, &params, pid, interval]()
        {
            auto client = Er::Client::create(params);

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

void dumpProcesses(Er::Log::ILog* log, const Er::Client::Params& params, int interval)
{
    protectedCall(
        log,
        [log, &params, interval]()
        {
            auto client = Er::Client::create(params);

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

void dumpProcessesDiff(Er::Log::ILog* log, const Er::Client::Params& params, int interval)
{
    protectedCall(
        log,
        [log, &params, interval]()
        {
            auto client = Er::Client::create(params);
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

void killProcess(Er::Log::ILog* log, const Er::Client::Params& params, uint64_t pid, const std::string& signame)
{
    protectedCall(
        log,
        [log, &params, pid, &signame]()
        {
            auto client = Er::Client::create(params);
            
            Er::PropertyBag req;
            req.insert({ Er::ProcessesGlobal::Pid::Id::value, Er::Property(Er::ProcessesGlobal::Pid::Id::value, pid) });
            req.insert({ Er::ProcessesGlobal::Signal::Id::value, Er::Property(Er::ProcessesGlobal::Signal::Id::value, signame) });
            auto result = client->request(Er::ProcessRequests::KillProcess, req);

            dumpPropertyBag(result, log);
        }
    );
}


