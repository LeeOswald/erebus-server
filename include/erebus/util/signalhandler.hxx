#pragma once

#include <erebus/erebus.hxx>

#include <csignal>
#include <future>

namespace Er
{

namespace Util
{

#if ER_POSIX

class EREBUS_EXPORT SignalHandler final
    : public NonCopyable
{
public:
    ~SignalHandler() noexcept;
    explicit SignalHandler(const std::initializer_list<int>& signals) noexcept;

    int wait() const;
    int waitHandler(std::function<bool(int)> handler) const;

    auto asyncWaitHandler(std::function<bool(int)> handler) const
    {
        return std::async(
            std::launch::async,
            &SignalHandler::waitHandler,
            this,
            std::move(handler)
        );
    }

private:
    sigset_t m_signals;
};

#endif


} // namespace Util {}

} // namespace Er {}