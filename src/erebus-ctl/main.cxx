#include "common.hxx"

#include <erebus/stopwatch.hxx>
#include <erebus/protocol.hxx>
#include <erebus/util/file.hxx>
#include <erebus-desktop/erebus-desktop.hxx>
#include <erebus-processmgr/erebus-processmgr.hxx>

#include <chrono>
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


Er::PropertyBag pargseArgs(const std::vector<std::string>& args)
{
    Er::PropertyBag parsed;

    for (auto& a: args)
    {
        std::vector<std::string> parts;
        boost::split(parts, a, boost::is_any_of(".:"));
        if (parts.size() != 3)
        {
            ErThrow(Er::format("Invalid format of domain.property_id:value in [{}]", a));
        }

        auto propInfo = Er::lookupProperty(parts[0], parts[1].c_str());
        if (!propInfo)
        {
            ErThrow(Er::format("Unknown property [{}.{}]", parts[0], parts[1]));
        }

        Er::PropId id = ER_PROPID_(parts[1].c_str());

        switch (propInfo->type())
        {
        case Er::PropertyType::Bool:
            {
                Er::Bool v;

                if ((parts[2] == "true") || (parts[2] == "1"))
                    v = Er::True;
                else if ((parts[2] == "false") || (parts[2] == "0"))
                    v = Er::False;
                else
                    ErThrow(Er::format("Invalid value [{}] for bool property [{}.{}]", parts[2], parts[0], parts[1]));

                Er::addProperty(parsed, Er::Property(id, v));
            }
            break;

        case Er::PropertyType::Int32:
            {
                int32_t v = std::strtol(parts[2].c_str(), nullptr, 10);
                Er::addProperty(parsed, Er::Property(id, v));
            }
            break;

        case Er::PropertyType::Int64:
            {
                int64_t v = std::strtoll(parts[2].c_str(), nullptr, 10);
                Er::addProperty(parsed, Er::Property(id, v));
            }
            break;

        case Er::PropertyType::UInt32:
            {
                uint32_t v = std::strtoul(parts[2].c_str(), nullptr, 10);
                Er::addProperty(parsed, Er::Property(id, v));
            }
            break;

        case Er::PropertyType::UInt64:
            {
                uint64_t v = std::strtoull(parts[2].c_str(), nullptr, 10);
                Er::addProperty(parsed, Er::Property(id, v));
            }
            break;
        
        case Er::PropertyType::Double:
            {
                double v = std::strtod(parts[2].c_str(), nullptr);
                Er::addProperty(parsed, Er::Property(id, v));
            }
            break;

        case Er::PropertyType::String:
            Er::addProperty(parsed, Er::Property(id, parts[2]));
            break;

        default:
            ErThrow(Er::format("Unsupported property type {} for property [{}.{}]", int(propInfo->type()), parts[0], parts[1]));
        }
    }

    return parsed;
}


bool issueRequest(
    Er::Log::ILog* log, 
    const Er::Client::ChannelParams& params, 
    const std::string& request,
    const std::string& domain,
    const std::vector<std::string>& args,
    int interval
    )
{
    return Er::protectedCall<bool>(
        log,
        [log, &params, &request, &domain, &args, interval]()
        {
            auto channel = Er::Client::createChannel(params);
            auto client = Er::Client::createClient(channel, log);

            auto requestArgs = pargseArgs(args);

            Er::PropertyBag result;

            Er::Stopwatch<> sw;
            {
                Er::Stopwatch<>::Scope sws(sw);
                result = client->request(request, requestArgs, std::chrono::milliseconds(5000));
            }

            while (!g_signalReceived)
            {
                dumpPropertyBag(domain, result, log);

                if (interval <= 0)
                    break;

                Er::Log::write(log, Er::Log::Level::Info, "RTT: {} ms", sw.value().count());
                Er::Log::writeln(log, Er::Log::Level::Info, "------------------------------------------------------");
                sw.reset();
                
                std::this_thread::sleep_for(std::chrono::seconds(interval));

                {
                    Er::Stopwatch<>::Scope sws(sw);
                    result = client->request(request, requestArgs, std::chrono::milliseconds(5000));
                }
            }

            return true;
        }
    );
}

bool receiveStream(
    Er::Log::ILog* log, 
    const Er::Client::ChannelParams& params, 
    const std::string& request,
    const std::string& domain,
    const std::vector<std::string>& args
    )
{
    return Er::protectedCall<bool>(
        log,
        [log, &params, &request, &domain, &args]()
        {
            auto channel = Er::Client::createChannel(params);
            auto client = Er::Client::createClient(channel, log);

            auto requestArgs = pargseArgs(args);

            Er::Stopwatch<> sw;
            sw.start();

            client->requestStream(
                request, 
                requestArgs, 
                std::chrono::milliseconds(5000),
                [log, domain, &sw](Er::PropertyBag&& item) -> bool
                {
                    if (g_signalReceived)
                        return false;
                    
                    sw.stop();

                    dumpPropertyBag(domain, item, log);
                    Er::Log::write(log, Er::Log::Level::Info, "Time delta: {} ms", sw.value().count());
                    
                    sw.reset();
                    sw.start();
                    return true;
                }
            );

            return true;
        }
    );
}

Er::Log::ILog::Ptr openLog(bool verbose)
{
    auto logger = Er::Log::makeAsyncLogger();

#if ER_WINDOWS
    if (::IsDebuggerPresent())
    {
        auto debugger = Er::Log::makeDebuggerSink(
            Er::Log::SimpleFormatter::make({ Er::Log::SimpleFormatter::Option::Time, Er::Log::SimpleFormatter::Option::Level, Er::Log::SimpleFormatter::Option::Tid }),
            Er::Log::SimpleFilter::make(Er::Log::Level::Debug, Er::Log::Level::Fatal)
        );

        logger->addSink("debugger", debugger);
    }
#endif

    {
        auto stdoutSink = Er::Log::makeOStreamSink(
            std::cout,
            Er::Log::SimpleFormatter::make({ Er::Log::SimpleFormatter::Option::Time, Er::Log::SimpleFormatter::Option::Level, Er::Log::SimpleFormatter::Option::Tid }),
            Er::Log::SimpleFilter::make(Er::Log::Level::Debug, Er::Log::Level::Warning)
        );

        logger->addSink("stdout", stdoutSink);

        auto stderrSink = Er::Log::makeOStreamSink(
            std::cerr,
            Er::Log::SimpleFormatter::make({ Er::Log::SimpleFormatter::Option::Time, Er::Log::SimpleFormatter::Option::Level, Er::Log::SimpleFormatter::Option::Tid }),
            Er::Log::SimpleFilter::make(Er::Log::Level::Error, Er::Log::Level::Fatal)
        );

        logger->addSink("stderr", stderrSink);
    }

    if (verbose)
        logger->setLevel(Er::Log::Level::Debug);
    else
        logger->setLevel(Er::Log::Level::Info);

    return logger;
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
    std::string request;
    std::string domain;
    std::string stream;
    std::vector<std::string> args;

    int result = EXIT_FAILURE;

    try
    {
        namespace po = boost::program_options;
        po::options_description options("Command line options");
        options.add_options()
            ("help,?", "display this message")
            ("verbose,v", "display debug output")
            ("endpoint,e", po::value<std::string>(), "server endpoint")
            ("ssl", "enable SSL")
            ("root", po::value<std::string>(&rootFile), "root certificate file path")
            ("cert", po::value<std::string>(&certFile), "client certificate file path")
            ("key", po::value<std::string>(&keyFile), "client certificate key file path")
            ("loop", po::value<int>(&interval)->default_value(0), "repeat the request with an interval")
            ("request,r", po::value<std::string>(&request), "request id")
            ("stream,s", po::value<std::string>(&stream), "stream request id")
            ("domain,d", po::value<std::string>(&domain), "domain")
            ("arg,a", po::value<std::vector<std::string>>(&args), "property <domain>.<id>:<value>")
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
        auto logger = openLog(verbose);

        Er::LibScope er(logger.get());
        Er::Client::LibParams cltParams(logger.get(), logger->level());
        Er::Client::LibScope cs(cltParams);

        Er::ProcessMgr::Private::registerAll(logger.get());
        Er::Desktop::Props::Private::registerAll(logger.get());
                
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

        Er::Client::ChannelParams params(ep, ssl, root, cert, key);
        
        if (!request.empty())
        {
            if (issueRequest(logger.get(), params, request, domain, args, interval))
                result = EXIT_SUCCESS;
        }
        else if (!stream.empty())
        {
            if (receiveStream(logger.get(), params, stream, domain, args))
                result = EXIT_SUCCESS;
        }

        if (g_signalReceived)
        {
            std::cerr << "Exiting due to signal " << *g_signalReceived << "\n";
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
        
    }

    return result;
}
