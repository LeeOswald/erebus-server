#include "log.hxx"

#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/format.hxx>
#include <erebus-clt/erebus-clt.hxx>
#include <erebus-processmgr/processprops.hxx>

#include <fstream>
#include <iostream>
#include <sstream>
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

std::string loadFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        throw Er::Exception(ER_HERE(), Er::Util::format("Failed to open [%s]", path.c_str()));

    std::stringstream ss;
    ss << file.rdbuf();

    return ss.str();
}

void version(Er::Client::IClient* client, Er::Log::ILog* log)
{
    try
    {
        auto ver = client->version();

        log->write(Er::Log::Level::Info, "Server version %d.%d.%d", ver.major, ver.minor, ver.patch);
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

void version(Er::Log::ILog* log, const Er::Client::Params& params, int interval)
{
    try
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
    catch (Er::Exception& e)
    {
        Er::Util::logException(log, Er::Log::Level::Error, e);
    }
    catch (std::exception& e)
    {
        Er::Util::logException(log, Er::Log::Level::Error, e);
    }
}

void addUser(Er::Log::ILog* log, const Er::Client::Params& params, const std::string& name, const std::string& password)
{
    try
    {
        auto client = Er::Client::create(params);

        client->addUser(name, password);

        Er::Log::Info(log) << "User " << name << " created successfully";
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

void rmUser(Er::Log::ILog* log, const Er::Client::Params& params, const std::string& name)
{
    try
    {
        auto client = Er::Client::create(params);

        client->removeUser(name);

        Er::Log::Info(log) << "User " << name << " deleted successfully";
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

void listUsers(Er::Log::ILog* log, const Er::Client::Params& params)
{
    try
    {
        auto client = Er::Client::create(params);

        auto users = client->listUsers();

        for (auto& u : users)
        {
            log->write(Er::Log::Level::Info, "Found user: %s", u.name.c_str());
        }
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

void exit(Er::Log::ILog* log, const Er::Client::Params& params)
{
    try
    {
        auto client = Er::Client::create(params);

        client->exit(false);

        log->write(Er::Log::Level::Info, "Server shutdown requested");
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

void restart(Er::Log::ILog* log, const Er::Client::Params& params)
{
    try
    {
        auto client = Er::Client::create(params);

        client->exit(true);

        log->write(Er::Log::Level::Info, "Server restart requested");
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

void dumpProcess(Er::Client::IClient* client, Er::Log::ILog* log, int pid)
{
    try
    {
        Er::PropertyBag req;
        req.insert({ Er::ProcessProps::Pid::Id::value, Er::Property(Er::ProcessProps::Pid::Id::value, uint64_t(pid)) });
        auto info = client->request(Er::ProcessRequests::ProcessDetails, req);
        auto it = info.find(Er::ProcessProps::Valid::Id::value);
        if (it == info.end())
        {
            log->write(Er::Log::Level::Error, "No data for PID %d", pid);
        }
        else
        {
            auto valid = std::any_cast<bool>(it->second.value);
            if (!valid)
            {   
                it = info.find(Er::ProcessProps::Error::Id::value);
                if (it != info.end())
                    log->write(Er::Log::Level::Error, "Invalid stat for PID %d", pid);
                else
                    log->write(Er::Log::Level::Error, "Invalid stat for PID %d: %s", pid, std::any_cast<std::string>(it->second.value).c_str());
            }
            else
            {
                log->write(Er::Log::Level::Info, "Dumping process %d", pid);
                for (auto it = info.begin(); it != info.end(); ++it)
                {
                    if (
                        (it->second.id != Er::ProcessProps::Valid::Id::value) &&
                        (it->second.id != Er::ProcessProps::Error::Id::value)
                    )
                    {
                        auto propInfo = Er::lookupProperty(it->second.id).get();
                        if (!propInfo)
                        {
                            log->write(Er::Log::Level::Warning, "0x%08x: ???", it->second.id);
                        }
                        else
                        {
                            std::ostringstream ss;
                            propInfo->format(it->second, ss);

                            log->write(Er::Log::Level::Info, "%s: %s", propInfo->name(), ss.str().c_str());
                        }
                    }
                }
            }
        }
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

void dumpProcess(Er::Log::ILog* log, const Er::Client::Params& params, int pid, int interval)
{
    try
    {
        auto client = Er::Client::create(params);

        while (!g_signalReceived)
        {
            dumpProcess(client.get(), log, pid);

            if (interval <= 0)
                break;

            std::this_thread::sleep_for(std::chrono::seconds(interval));
        }
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
    std::string creds;
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
            ("user", po::value<std::string>(&creds), "user <name>:<password>")
            ("version", "display server version")
            ("exit", "shutdown server")
            ("restart", "restart server")
            ("adduser", po::value<std::string>(), "add user <name>:<password>")
            ("rmuser", po::value<std::string>(), "delete user <name>")
            ("listusers", "list existing users")
            ("loop", po::value<int>(&interval), "repeat the request with an interval")
            ("process", po::value<int>(), "view process info for PID")
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

        Er::LibScope er;
        Er::Client::LibParams cltParams(&console, console.level());
        Er::Client::LibScope cs(cltParams);
                
        bool ssl = (vm.count("ssl") > 0);
        std::string root;
        
        if (!rootFile.empty())
            root = loadFile(rootFile);

        std::string user;
        std::string password;
        if (vm.count("user"))
        {
            std::vector<std::string> parts;
            boost::split(parts, creds, boost::is_any_of(":"));
            if (parts.size() != 2)
            {
                std::cerr << "Expected <user>:<password> pair specified for \"user\" arg\n";
                return EXIT_FAILURE;
            }

            user = std::move(parts[0]);
            password = std::move(parts[1]);
        }
        
        Er::Client::Params params(&console, ep, ssl, root, user, password);
        
        if (vm.count("version"))
        {
            version(&console, params, interval);
        }
        else if (vm.count("exit"))
        {
            exit(&console, params);
        }
        else if (vm.count("restart"))
        {
            restart(&console, params);
        }
        else if (vm.count("adduser"))
        {
            auto namePwd = vm["adduser"].as<std::string>();
            std::vector<std::string> parts;
            boost::split(parts, namePwd, boost::is_any_of(":"));
            if (parts.size() != 2)
            {
                std::cerr << "Expected <user>:<password> pair specified for \"adduser\" command\n";
                return EXIT_FAILURE;
            }

            addUser(&console, params, parts[0], parts[1]);
        }
        else if (vm.count("rmuser"))
        {
            auto name = vm["rmuser"].as<std::string>();
            rmUser(&console, params, name);
        }
        else if (vm.count("listusers"))
        {
            listUsers(&console, params);
        }
        else if (vm.count("process"))
        {
            auto pid = vm["process"].as<int>();
            dumpProcess(&console, params, pid, interval);
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
