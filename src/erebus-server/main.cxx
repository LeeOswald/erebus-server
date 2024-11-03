#include <erebus/knownprops.hxx>
#include <erebus/program.hxx>
#include <erebus/system/user.hxx>
#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/file.hxx>
#include <erebus/util/pidfile.hxx>

#include <erebus-srv/erebus-srv.hxx>

#include "config.hxx"
#include "coreservice.hxx"
#include "pluginmgr.hxx"

#include <iostream>

namespace
{

class ErebusServerApplication final
    : public Er::Program
{
public:
    ErebusServerApplication() noexcept = default;

private:
    void addCmdLineOptions(boost::program_options::options_description& options) override
    {
        options.add_options()
            ("config", boost::program_options::value<std::string>(&m_cfgFile)->default_value("erebus-server.cfg"), "configuration file path")
#if ER_LINUX
            ("noroot", "don't require root privileges")
#endif
            ;
    }

    bool doLoadConfiguration() override
    {
        try
        {
            if (m_cfgFile.empty())
            {
                std::cerr << "Configuration file expected\n";
                return false;
            }

            m_config = Er::Private::loadConfig(m_cfgFile);
            return true;
        }
        catch (std::exception& e)
        {
            std::cerr << "Failed to load the configuration: " << e.what() << std::endl;
        }

        return false;
    }

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

        {
            auto fileSink = Er::Log::makeFileSink(
                Er::Log::ThreadSafe::No,
                m_config.logFile,
                Er::Log::SimpleFormatter::make({ Er::Log::SimpleFormatter::Option::DateTime, Er::Log::SimpleFormatter::Option::Level, Er::Log::SimpleFormatter::Option::Tid }),
                Er::Log::IFilter::Ptr{},
                m_config.keepLogs,
                m_config.maxLogSize
            );

            logger->addSink("file", fileSink);
        }
    }

    bool doInitialize() override
    {
        // create pidfile
        std::unique_ptr<Er::Util::PidFile> pidFile;
        if (!m_config.pidFile.empty())
        {
            try
            {
                pidFile.reset(new Er::Util::PidFile(m_config.pidFile));
            }
            catch (std::exception& e)
            {
                Er::Log::warning(log(), "Failed to create PID file {}: {}", m_config.pidFile, e.what());
                auto existing = Er::Util::PidFile::read(m_config.pidFile);
                if (existing)
                    Er::Log::warning(log(), "Found running server instance with PID {}", *existing);

                return false;
            }
        }

        auto user = Er::System::User::current();
        Er::Log::info(log(), "Starting as user {}", user.name);

        Er::Server::initialize(log());

        auto serverParams = makeServerParams();
        Er::Log::info(log(), "Creating a new server instance");
        m_server = Er::Server::create(serverParams);

        Er::Log::info(log(), "Creating core service");
        m_coreService.reset(new Erp::Server::CoreService(log()));
        m_coreService->registerService(m_server.get());

        loadPlugins();

        return true;
    }

    Er::Server::Params makeServerParams()
    {
        Er::Server::Params serverParams(log());
        serverParams.endpoints.reserve(m_config.endpoints.size());

        for (auto& ep : m_config.endpoints)
        {
            if (ep.ssl)
            {
                if (!Er::protectedCall<bool>(log(), [&ep, &serverParams]()
                {
                    std::string rootCA;
                    if (!ep.rootCA.empty())
                        rootCA = Er::Util::loadTextFile(ep.rootCA);

                    std::string certificate;
                    if (!ep.certificate.empty())
                        certificate = Er::Util::loadTextFile(ep.certificate);

                    std::string privateKey;
                    if (!ep.privateKey.empty())
                        privateKey = Er::Util::loadTextFile(ep.privateKey);

                    serverParams.endpoints.push_back(Er::Server::Params::Endpoint(ep.endpoint, rootCA, certificate, privateKey));

                    return true;
                }))
                {
                    Er::Log::info(log(), "SSL certificates could not be loaded for [{}]", ep.endpoint);
                }
            }
            else
            {
                serverParams.endpoints.push_back(Er::Server::Params::Endpoint(ep.endpoint));
            }
        }

        return serverParams;
    }

    void loadPlugins()
    {
        Er::Server::PluginParams pluginParams;
        pluginParams.log = log();
        pluginParams.container = m_server.get();

        m_pluginMgr.reset(new Er::Private::PluginMgr(pluginParams));
        for (auto& plugin : m_config.plugins)
        {
            if (!plugin.enabled)
            {
                Er::Log::info(log(), "Skipping plugin [{}]", plugin.path);
                continue;
            }

            Er::Log::info(log(), "Loading plugin [{}]", plugin.path);

            auto success = Er::protectedCall<bool>(log(), [this, &plugin]()
            {
                m_pluginMgr->load(plugin.path, plugin.args);
                return true;
            });

            if (!success)
                Er::Log::error(log(), "Failed to load plugin [{}]", plugin.path);
        }
    }

    void doFinalize() noexcept override
    {
        Er::Log::info(log(), "Unloading plugins");

        if (m_pluginMgr)
        {
            m_pluginMgr->unloadAll();
            m_pluginMgr.reset();
        }

        if (m_server)
        {
            if (m_coreService)
            {
                m_coreService->unregisterService(m_server.get());
                m_coreService.reset();
            }

            m_server.reset();
        }

        Er::Server::finalize();
    }

    int doRun() override
    {
        exitCondition().waitValue(true);

        if (signalReceived())
        {
            Er::Log::warning(log(), "Exiting due to signal {}", signalReceived());
        }

        return 0;
    }

    std::string m_cfgFile;
    Er::Private::ServerConfig m_config;
    Er::Server::IServer::Ptr m_server;
    std::unique_ptr<Erp::Server::CoreService> m_coreService;
    std::unique_ptr<Er::Private::PluginMgr> m_pluginMgr;
};


} // namespace {}


int main(int argc, char* argv[], char* env[])
{
#if ER_LINUX
    bool noroot = ErebusServerApplication::optionPresent(argc, argv, "--noroot", nullptr);
    if (!noroot && (::geteuid() != 0))
    {
        std::cerr << "Root privileges required" << std::endl;
        std::exit(EXIT_FAILURE);
    }
#endif

    ErebusServerApplication::globalStartup(argc, argv);
    ErebusServerApplication app;

    auto resut = app.run(argc, argv);

    ErebusServerApplication::globalShutdown();
    return resut;
}
