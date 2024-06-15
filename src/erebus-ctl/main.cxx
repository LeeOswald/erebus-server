#include "process.hxx"

#include <erebus/protocol.hxx>
#include <erebus/util/file.hxx>
#include <erebus/util/format.hxx>
#include <erebus-desktop/erebus-desktop.hxx>
#include <erebus-desktop/protocol.hxx>
#include <erebus-processmgr/processprops.hxx>

#include <iostream>
#include <thread>

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>


std::optional<int> g_signalReceived;

namespace
{

void signalHandler(int signo)
{
    g_signalReceived = signo;
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

void iconBy(
    Er::Log::ILog* log, 
    Er::Client::IClient* client, 
    std::optional<std::string> iconName,
    std::optional<std::string> iconExe,
    std::optional<std::string> iconComm,
    std::optional<uint64_t> iconPid,  
    uint32_t iconSize, 
    const std::string& outFile
    )
{
    Er::PropertyBag req;
    Er::addProperty<Er::Desktop::Props::IconSize>(req, iconSize);
    
    if (iconName)
    {
        Er::addProperty<Er::Desktop::Props::IconName>(req, *iconName);
        ErLogInfo(log, ErLogNowhere(), "Requested icon by name [%s]...", iconName->c_str());
    }

    if (iconExe)
    {
        Er::addProperty<Er::Desktop::Props::Exe>(req, *iconExe);
        ErLogInfo(log, ErLogNowhere(), "Requested icon by exe [%s]...", iconExe->c_str());
    }

    if (iconComm)
    {
        Er::addProperty<Er::Desktop::Props::Comm>(req, *iconComm);
        ErLogInfo(log, ErLogNowhere(), "Requested icon by comm [%s]...", iconComm->c_str());
    }

    if (iconPid)
    {
        Er::addProperty<Er::Desktop::Props::Pid>(req, *iconPid);
        ErLogInfo(log, ErLogNowhere(), "Requested icon by PID %zu...", *iconPid);
    }
        

    auto reply = client->request(Er::Desktop::Requests::QueryIcon, req);
    while (!g_signalReceived)
    {
        dumpPropertyBag(reply, log);

        auto status = Er::getProperty<Er::Desktop::Props::IconState>(reply);
        if (!status)
        {
            ErLogError(log, ErLogNowhere(), "No status returned");
            break;
        }

        if (*status == static_cast<uint32_t>(Er::Desktop::IconState::Pending))
        {
            reply = client->request(Er::Desktop::Requests::QueryIcon, req);
            continue;
        }

        break;
    }    
}

void iconBy(
    Er::Log::ILog* log, 
    const Er::Client::Params& params, 
    std::optional<std::string> iconName,
    std::optional<std::string> iconExe,
    std::optional<std::string> iconComm,
    std::optional<uint64_t> iconPid, 
    uint32_t iconSize, 
    const std::string& outFile
    )
{
    protectedCall(
        log,
        [log, &params, iconName, iconExe, iconComm, iconPid, iconSize, outFile]()
        {
            auto client = Er::Client::create(params);

            iconBy(log, client.get(), iconName, iconExe, iconComm, iconPid, iconSize, outFile);
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
    std::string outFile;

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
            ("icon", po::value<std::string>(), "retrieve icon by name")
            ("iconcomm", po::value<std::string>(), "retrieve icon by process\'s comm")
            ("iconexe", po::value<std::string>(), "retrieve icon by process\'s exe path")
            ("iconpid", po::value<uint64_t>(), "retrieve icon by process\'s PID")
            ("iconsize", po::value<int>(), "icon size (16|32)")
            ("out", po::value<std::string>(&outFile), "output file name")
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
        Er::Desktop::Props::Private::registerAll(&console);
                
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

        uint32_t iconSize = 16;
        if (vm.count("iconsize"))
        {
            iconSize = static_cast<uint32_t>(vm["iconsize"].as<int>());
            if ((iconSize != 16) && (iconSize != 32))
            {
                std::cerr << "Unsupported icon size\n";
                return EXIT_FAILURE;
            }
        }

        std::optional<std::string> iconName;
        std::optional<std::string> iconExe;
        std::optional<std::string> iconComm;
        std::optional<uint64_t> iconPid;
        if (vm.count("icon"))
            iconName = vm["icon"].as<std::string>();
        if (vm.count("iconexe"))
            iconExe = vm["iconexe"].as<std::string>();        
        if (vm.count("iconcomm"))
            iconComm = vm["iconcomm"].as<std::string>();
        if (vm.count("iconpid"))
            iconPid = vm["iconpid"].as<uint64_t>();

        iconBy(&console, params, iconName, iconExe, iconComm, iconPid, iconSize, outFile);
        
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
