#include "log.hxx"

#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/format.hxx>
#include <erebus-clt/erebus-clt.hxx>

#include <fstream>
#include <iostream>
#include <sstream>

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

void version(Er::Log::ILog* log, Er::Client::IStub* client)
{
    auto ver = client->version();

    log->write(Er::Log::Level::Info, "Server version %d.%d.%d", ver.major, ver.minor, ver.patch);
}

void exit(Er::Log::ILog* log, Er::Client::IStub* client)
{
    client->exit(false);

    log->write(Er::Log::Level::Info, "Server shutdown requested");
}

void restart(Er::Log::ILog* log, Er::Client::IStub* client)
{
    client->exit(true);

    log->write(Er::Log::Level::Info, "Server restart requested");
}

void run(Er::Log::ILog* log, const Er::Client::Params& params, std::string&& command)
{
    try
    {
        auto client = Er::Client::create(params);

        if (command == "version")
            version(log, client.get());
        else if (command == "exit")
            exit(log, client.get());
        else if (command == "restart")
            restart(log, client.get());
        else
            log->write(Er::Log::Level::Error, "Unsupported command [%s]", command.c_str());
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
    std::string certFile;
    std::string keyFile;

    try
    {
        namespace po = boost::program_options;
        po::options_description options("Command line options");
        options.add_options()
            ("help,h", "display this message")
            ("verbose,v", "display debug output")
            ("endpoint,e", po::value<std::string>(), "server endpoint")
            ("command,c", po::value<std::string>(), "execute command")
            ("root,r", po::value<std::string>(&rootFile), "root certificate file path")
            ("certificate,s", po::value<std::string>(&certFile), "certificate file path")
            ("key,k", po::value<std::string>(&keyFile), "private key file path")
        ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, options), vm);
        po::notify(vm);

        if (vm.count("help") || !vm.count("command") || !vm.count("endpoint"))
        {
            std::cerr << options << "\n";
            return EXIT_SUCCESS;
        }

        bool verbose = vm.count("verbose");
        auto ep = vm["endpoint"].as<std::string>();

        Er::Scope er;
        Er::Client::Scope cs;

        ErCtl::Log console(verbose ? Er::Log::Level::Debug : Er::Log::Level::Info);

        std::string root;
        std::string certificate;
        std::string key;
        if (!rootFile.empty())
            root = loadFile(rootFile);
        if (!certFile.empty())
            certificate = loadFile(certFile);
        if (!keyFile.empty())
            key = loadFile(keyFile);

        auto cmd = vm["command"].as<std::string>();

        Er::Client::Params params(ep, root, certificate, key);
        run(&console, params, std::move(cmd));
        
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
