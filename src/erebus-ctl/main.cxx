#include "log.hxx"

#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/format.hxx>
#include <erebus-clt/erebus-clt.hxx>

#include <fstream>
#include <iostream>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>


std::string loadFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        throw Er::Exception(ER_HERE(), Er::Util::format("Failed to open [%s]", path.c_str()));

    std::stringstream ss;
    ss << file.rdbuf();

    return ss.str();
}

void version(Er::Log::ILog* log, const Er::Client::Params& params)
{
    try
    {
        auto client = Er::Client::create(params);

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

void addUser(Er::Log::ILog* log, const Er::Client::Params& params, std::string_view name, std::string_view password)
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

void rmUser(Er::Log::ILog* log, const Er::Client::Params& params, std::string_view name)
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


int main(int argc, char* argv[])
{
#if ER_WINDOWS
    ::SetConsoleOutputCP(CP_UTF8);
#endif

    std::string rootFile;
    std::string user;
    std::string password;

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
            ("user", po::value<std::string>(&user), "user name")
            ("pwd", po::value<std::string>(&password), "user password")
            ("version", "display server version")
            ("exit", "shutdown server")
            ("restart", "restart server")
            ("adduser", po::value<std::string>(), "add user <name>:<password>")
            ("rmuser", po::value<std::string>(), "delete user <name>")
            ("listusers", "list existing users")
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

        Er::Scope er;
        Er::Client::LibParams cltParams(&console, console.level());
        Er::Client::Scope cs(cltParams);
                
        bool ssl = (vm.count("ssl") > 0);
        std::string root;
        
        if (!rootFile.empty())
            root = loadFile(rootFile);
        
        Er::Client::Params params(&console, ep, ssl, root, user, password);
        
        if (vm.count("version"))
        {
            version(&console, params);
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
        
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
