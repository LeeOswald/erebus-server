#include <erebus/util/signalhandler.hxx>



namespace Er
{

namespace Util
{

SignalHandler::SignalHandler(const std::initializer_list<int>& signals) noexcept
    : m_signals()
{
    ::sigemptyset(&m_signals);

    for (auto signum : signals)
    {
        if (::sigaddset(&m_signals, signum) != 0)
        {
            assert(!"Invalid signal");
        }
    }

    if (::pthread_sigmask(SIG_BLOCK, &m_signals, nullptr) != 0)
    {
        assert("!Signal could not be blocked");
    }
}

SignalHandler::~SignalHandler() noexcept
{
    ::pthread_sigmask(SIG_UNBLOCK, &m_signals, nullptr);
}

int SignalHandler::wait() const
{
    int signum = 0;
    int ret = ::sigwait(&m_signals, &signum);
    assert(ret == 0);

    return signum;
}

int SignalHandler::waitHandler(std::function<bool(int)> handler) const
{
    while (true)
    {
        int signum = wait();
        if (handler(signum))
            return signum;
    }
}


} // namespace Util {}

} // namespace Er {}