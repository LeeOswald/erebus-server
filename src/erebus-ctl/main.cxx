#include "log.hxx"

#include <erebus/protocol.hxx>
#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/file.hxx>
#include <erebus/util/format.hxx>
#include <erebus-clt/erebus-clt.hxx>
#include <erebus-processmgr/processprops.hxx>

#include <iostream>
#include <thread>

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>


namespace
{

std::optional<int> g_signalReceived;

void signalHandler(int signo)
{
    g_signalReceived = signo;
}

template <typename Work>
void protectedCall(Er::Log::ILog* log, Work w) noexcept
{
    try
    {
        w();
    }
    catch (Er::Exception& e)
    {
        Er::Util::logException(log, Er::Log::Level::Error, e);
    }
    catch (std::exception& e)
    {
        Er::Util::logException(log, Er::Log::Level::Error, e);
    }
}

void version(Er::Client::IClient* client, Er::Log::ILog* log)
{
    protectedCall(
        log,
        [client, log]()
        {
            Er::PropertyBag req;
            auto reply = client->request(Er::Protocol::GenericRequests::GetVersion, req, std::nullopt);

            auto systemName = Er::getPropertyOr<Er::Protocol::Props::RemoteSystemDesc>(reply, std::string());
            auto serverVer = Er::getPropertyOr<Er::Protocol::Props::ServerVersionString>(reply, std::string());

            log->write(Er::Log::Level::Info, ErLogNowhere(), "Remote system: %s", systemName.c_str());
            log->write(Er::Log::Level::Info, ErLogNowhere(), "Server version: %s", serverVer.c_str());
        }
    );
}

void version(Er::Log::ILog* log, const Er::Client::Params& params, int interval)
{
    protectedCall(
        log,
        [log, &params, interval]()
        {
            auto client = Er::Client::create(params);

            while (!g_signalReceived)
            {
                version(client.get(), log);

                if (interval <= 0)
                    break;

                std::this_thread::sleep_for(std::chrono::seconds(interval));
            }
        }
    );
}

void dumpPropertyBag(const Er::PropertyBag& info, Er::Log::ILog* log)
{
    for (auto it = info.begin(); it != info.end(); ++it)
    {
        auto propInfo = Er::lookupProperty(it->second.id).get();
        if (!propInfo)
        {
            log->write(Er::Log::Level::Warning, ErLogNowhere(), "0x%08x: ???", it->second.id);
        }
        else
        {
            std::ostringstream ss;
            propInfo->format(it->second, ss);

            log->write(Er::Log::Level::Info, ErLogNowhere(), "%s: %s", propInfo->name(), ss.str().c_str());
        }
    }
}

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


} // namespace {}


int main(int argc, char* argv[])
{
#if ER_WINDOWS
    ::SetConsoleOutputCP(CP_UTF8);
#endif

    ::signal(SIGINT, signalHandler);
    ::signal(SIGTERM, signalHandler);
#if ER_POSIX
    ::signal(SIGPIPE, signalHandler);
    ::signal(SIGHUP, signalHandler);
#endif

    std::string rootFile;
    std::string certFile;
    std::string keyFile;
    int interval = 0;

    try
    {
        namespace po = boost::program_options;
        po::options_description options("Command line options");
        options.add_options()
            ("help,?", "display this message")
            ("verbose,v", "display debug output")
            ("endpoint", po::value<std::string>(), "server endpoint")
            ("ssl", "enable SSL")
            ("root", po::value<std::string>(&rootFile), "root certificate file path")
            ("cert", po::value<std::string>(&certFile), "client certificate file path")
            ("key", po::value<std::string>(&keyFile), "client certificate key file path")
            ("version", "display server version")
            ("loop", po::value<int>(&interval)->default_value(0), "repeat the request with an interval")
            ("process", po::value<int>(), "view process info for PID")
            ("processes", "view process list")
            ("procdiff", "view process list (incremental)")
            ("kill", po::value<std::string>(), "kill <pid>:<signal>")
        ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, options), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cout << options << "\n";
            return EXIT_SUCCESS;
        }

        if (!vm.count("endpoint"))
        {
            std::cerr << "Server endpoint address expected\n";
            return EXIT_FAILURE;
        }
        auto ep = vm["endpoint"].as<std::string>();

        bool verbose = (vm.count("verbose") > 0);
        ErCtl::Log console(verbose ? Er::Log::Level::Debug : Er::Log::Level::Info);

        Er::LibScope er(&console);
        Er::Client::LibParams cltParams(&console, console.level());
        Er::Client::LibScope cs(cltParams);

        Er::ProcessesGlobal::Private::registerAll(&console);
        Er::ProcessProps::Private::registerAll(&console);
                
        bool ssl = (vm.count("ssl") > 0);
        std::string root;
        std::string cert;
        std::string key;
        
        if (!rootFile.empty())
            root = Er::Util::loadTextFile(rootFile);

        if (!certFile.empty())
            cert = Er::Util::loadTextFile(certFile);

        if (!keyFile.empty())
            key = Er::Util::loadTextFile(keyFile);

        Er::Client::Params params(&console, ep, ssl, root, cert, key);
        
        if (vm.count("version"))
        {
            version(&console, params, interval);
        }
        else if (vm.count("process"))
        {
            auto pid = vm["process"].as<int>();
            dumpProcess(&console, params, pid, interval);
        }
        else if (vm.count("processes"))
        {
            dumpProcesses(&console, params, interval);
        }
        else if (vm.count("procdiff"))
        {
            dumpProcessesDiff(&console, params, interval);
        }
        else if (vm.count("kill"))
        {
            auto tmp = vm["kill"].as<std::string>();
            std::vector<std::string> parts;
            boost::split(parts, tmp, boost::is_any_of(":"));
            if (parts.size() != 2)
            {
                std::cerr << "Expected <pid>:<signal> pair specified for \"kill\" command\n";
                return EXIT_FAILURE;
            }

            auto pid = std::strtoull(parts[0].c_str(), nullptr, 10);
            killProcess(&console, params, pid, parts[1]);
        }
        
        if (g_signalReceived)
        {
            std::cerr << "Exiting due to signal " << *g_signalReceived << "\n";
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
