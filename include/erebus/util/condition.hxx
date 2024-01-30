#pragma once

#include <erebus/erebus.hxx>

#include <condition_variable>
#include <mutex>


namespace Er
{

namespace Util
{

//
// stateful condition variable
//

class EREBUS_EXPORT Condition final
    : public boost::noncopyable
{
public:
    enum class Reset
    {
        Auto,
        Manual
    };

    explicit Condition(Reset reset, bool initiallySignaled = false);

    void set() noexcept;
    void reset() noexcept;
    void wait() noexcept;
    bool wait(std::chrono::milliseconds duration) noexcept;

private:
    std::condition_variable m_cv;
    std::mutex m_mutex;
    bool m_signaled;
    bool m_autoReset;
};


} // namespace Util {}

} // namespace Er {}
