#include "log.hxx"

#include <erebus/util/exceptionutil.hxx>
#include <erebus-clt/erebus-clt.hxx>

#include <iostream>

#include <boost/program_options.hpp>


void version(Er::Log::ILog* log, Er::Client::IStub* client)
{
    auto ver = client->version();

    log->write(Er::Log::Level::Info, "Server version %d.%d.%d", ver.major, ver.minor, ver.patch);
}

void exit(Er::Log::ILog* log, Er::Client::IStub* client)
{
    client->exit(false);
}

void restart(Er::Log::ILog* log, Er::Client::IStub* client)
{
    client->exit(true);
}

void run(Er::Log::ILog* log, std::string&& address, std::string&& command)
{
    try
    {
        auto client = Er::Client::create(address);

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

    try
    {
        namespace po = boost::program_options;
        po::options_description options("Command line options");
        options.add_options()
            ("help,h", "display this message")
            ("verbose,v", "display debug output")
            ("address,a", po::value<std::string>(), "server address:port")
            ("command,c", po::value<std::string>(), "execute command")
        ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, options), vm);
        po::notify(vm);

        if (vm.count("help") || !vm.count("command"))
        {
            std::cerr << options << "\n";
            return EXIT_SUCCESS;
        }

        bool verbose = (vm.count("verbose") > 0);

        std::string addr("127.0.0.1:6665");
        if (vm.count("address"))
        {
            addr = vm["address"].as<std::string>();
        }

        Er::initialize();
        Er::Client::initialize();

        ErCtl::Log console(verbose ? Er::Log::Level::Debug : Er::Log::Level::Info);

        auto cmd = vm["command"].as<std::string>();

        run(&console, std::move(addr), std::move(cmd));
        
        Er::Client::finalize();
        Er::finalize();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
