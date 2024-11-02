#pragma once

#include <erebus/condition.hxx>
#include <erebus/log.hxx>

#include <boost/program_options.hpp>

namespace Er
{

class EREBUS_EXPORT Program
    : public NonCopyable
{
public:
    static void globalStartup(int argc, char** argv) noexcept;
    static void globalShutdown() noexcept;

    virtual ~Program();
    Program() noexcept;

    static bool optionPresent(int argc, char** argv, const char* longName, const char* shortName) noexcept
    {
        for (int i = 1; i < argc; ++i)
        {
            if (longName && !std::strcmp(argv[i], longName))
                return true;

            if (shortName && !std::strcmp(argv[i], shortName))
                return true;
        }

        return false;
    }

    static constexpr Program& instance() noexcept
    {
        ErAssert(s_instance);
        return *s_instance;
    }

    static constexpr bool isDaemon() noexcept
    {
        return s_isDaemon;
    }

    static constexpr Event& exitCondition() noexcept
    {
        return s_exitCondition;
    }

    static int signalReceived() noexcept
    {
        return s_signalReceived;
    }

    constexpr const boost::program_options::variables_map& options() const noexcept
    {
        return m_options;
    }

    bool verboseLogging() const noexcept;
    Log::ILog* log() noexcept
    {
        return m_logger.get();
    }

    int run(int argc, char** argv) noexcept;

protected:
    virtual void addCmdLineOptions(boost::program_options::options_description& options);
    virtual void displayHelp(const boost::program_options::options_description& options);
    virtual bool doLoadConfiguration();
    virtual void addLoggers(Log::ILog* logger) = 0;
    virtual bool doInitialize() = 0;
    virtual void doFinalize() noexcept = 0;
    virtual int doRun() = 0;

private:
    bool initialize(int argc, char** argv) noexcept;
    void finalize() noexcept;
    bool loadConfiguration(int argc, char** argv) noexcept;
    
    static void staticTerminateHandler();
    static void staticSignalHandler(int signo);
    static void staticPrintAssertFn(std::string_view message);

    bool initializeRtl() noexcept;
    void finalizeRtl() noexcept;
    
    static Program* s_instance;
    static bool s_isDaemon;
    static Event s_exitCondition;
    static int s_signalReceived;

#if ER_POSIX
    struct SignalWaiter
    {
        Util::SignalHandler sh;
        std::future<int> fu;

        SignalWaiter(const std::initializer_list<int>& signals)
            : sh(signals)
            , fu(sh.asyncWaitHandler(
                [](int signo)
                {
                    staticSignalHandler(signo);
                    return true;
                }))
        {)
    };

    static std::unique_ptr<SignalWaiter> s_signalWaiter;
#endif

    boost::program_options::variables_map m_options;
    Log::ILog::Ptr m_logger;
};

} // namespace Er {}