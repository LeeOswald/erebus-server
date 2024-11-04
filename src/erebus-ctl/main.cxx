#include "common.hxx"
#include "request_runner.hxx"
#include "stream_runner.hxx"

#include <erebus/program.hxx>
#include <erebus/util/file.hxx>

#include <erebus-desktop/erebus-desktop.hxx>
#include <erebus-processmgr/erebus-processmgr.hxx>
#include <erebus-srv/global_requests.hxx>

#include <boost/algorithm/string.hpp>


std::mutex& bulkLogWrite() noexcept
{
    static std::mutex m;
    return m;
}

namespace
{

class ErebusClientApplication final
    : public Er::Program
{
public:
    ErebusClientApplication() noexcept = default;

private:
    void addLoggers(Er::Log::ILog* logger) override
    {
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
    }

    void addCmdLineOptions(boost::program_options::options_description& options) override
    {
        options.add_options()
            ("endpoint,e", boost::program_options::value<std::string>(&m_endpoint), "server address")
            ("ssl", "use SSL/TLS")
            ("root_ca", boost::program_options::value<std::string>(&m_rootCA), "root CA certificate file")
            ("certificate", boost::program_options::value<std::string>(&m_certificate), "certificate file")
            ("private_key", boost::program_options::value<std::string>(&m_privateKey), "private key file")
            ("loop,l", boost::program_options::value<int>(&m_interval)->default_value(0), "request repeat interval")
            ("parallel,p", boost::program_options::value<int>(&m_parallel)->default_value(1), "thread count")
            ("request,r", boost::program_options::value<std::string>(&m_request), "unary request")
            ("stream,s", boost::program_options::value<std::string>(&m_stream), "streaming request")
            ("domain,d", boost::program_options::value<std::string>(&m_domain), "request domain")
            ("arg,a", boost::program_options::value<std::vector<std::string>>(&m_args), "property <domain>.<id>:<value>")
            ;
    }

    bool doLoadConfiguration() override
    {
        if (m_endpoint.empty())
        {
            std::cerr << "Server address expected\n";
            return false;
        }

        if (m_ssl = (options().count("ssl") > 0))
        {
            {
                if (m_rootCA.empty())
                {
                    std::cerr << "Root CA certificate path expected\n";
                    return false;
                }

                m_rootCALoaded = Er::protectedCall<std::string>(
                    log(),
                    [this]()
                {
                    return Er::Util::loadTextFile(m_rootCA);
                }
                );

                if (m_rootCALoaded.empty())
                {
                    std::cerr << "Valid root CA certificate expected\n";
                    return false;
                }
            }

            {
                if (m_privateKey.empty())
                {
                    std::cerr << "Private key path expected\n";
                    return false;
                }

                m_privateKeyLoaded = Er::protectedCall<std::string>(
                    log(),
                    [this]()
                    {
                        return Er::Util::loadTextFile(m_privateKey);
                    }
                );

                if (m_privateKeyLoaded.empty())
                {
                    std::cerr << "Valid private key expected\n";
                    return false;
                }
            }

            {
                if (m_certificate.empty())
                {
                    std::cerr << "Certificate path expected\n";
                    return false;
                }

                m_certificateLoaded = Er::protectedCall<std::string>(
                    log(),
                    [this]()
                    {
                        return Er::Util::loadTextFile(m_certificate);
                    }
                );

                if (m_certificateLoaded.empty())
                {
                    std::cerr << "Valid certificate expected\n";
                    return false;
                }
            }
        }

        return true;
    }

    bool doInitialize() override
    {
        Er::Client::initialize(log());

        Er::Server::Props::Private::registerAll(log());
        Er::ProcessMgr::Private::registerAll(log());
        Er::Desktop::Props::Private::registerAll(log());

        m_parsedArgs = pargseArgs(m_args);
        
        return true;
    }

    void doFinalize() noexcept override
    {
        Er::ProcessMgr::Private::unregisterAll(log());
        Er::Desktop::Props::Private::unregisterAll(log());
        Er::Server::Props::Private::unregisterAll(log());

        Er::Client::finalize();
    }

    int doRun(int argc, char** argv) override
    {
        Er::Client::ChannelParams params(m_endpoint, m_ssl, m_rootCALoaded, m_certificateLoaded, m_privateKeyLoaded);
        auto channel = Er::Client::createChannel(params);

        std::unique_ptr<RequestRunner> requestRunner;
        std::unique_ptr<StreamRunner> streamRunner;

        if (options().count("request") > 0)
            requestRunner.reset(new RequestRunner(log(), channel, m_request, m_domain, m_parsedArgs, m_interval, m_parallel));

        if (options().count("stream") > 0)
            streamRunner.reset(new StreamRunner(log(), channel, m_stream, m_domain, m_parsedArgs, m_interval, m_parallel));

        exitCondition().waitValue(true);

        if (signalReceived())
        {
            Er::Log::warning(log(), "Exiting due to signal {}", signalReceived());
        }

        return 0;
    }

    static Er::PropertyBag pargseArgs(const std::vector<std::string>& args)
    {
        Er::PropertyBag parsed;

        for (auto& a : args)
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

private:
    std::string m_endpoint;
    bool m_ssl = false;
    std::string m_rootCA;
    std::string m_rootCALoaded;
    std::string m_certificate;
    std::string m_certificateLoaded;
    std::string m_privateKey;
    std::string m_privateKeyLoaded;
    int m_interval = 0;
    int m_parallel = 0;
    std::string m_request;
    std::string m_stream;
    std::string m_domain;
    std::vector<std::string> m_args;
    Er::PropertyBag m_parsedArgs;
};


} // namespace {}


int main(int argc, char* argv[])
{
    ErebusClientApplication::globalStartup(argc, argv);
    ErebusClientApplication app;

    auto resut = app.run(argc, argv);

    ErebusClientApplication::globalShutdown();
    return resut;
}
