#include <erebus/util/condition.hxx>

namespace Er
{
    
namespace Util
{    

Condition::Condition(bool autoReset, bool initiallySignaled)
    : m_signaled(initiallySignaled)
    , m_autoReset(autoReset)
{
}

void Condition::set() noexcept
{
    {
        std::lock_guard l(m_mutex);
        m_signaled = true;
    }

    if (m_autoReset)
        m_cv.notify_one();
    else
        m_cv.notify_all();
}

void Condition::reset() noexcept
{
    std::lock_guard l(m_mutex);
    m_signaled = false;
}

void Condition::wait() noexcept
{
    std::unique_lock l(m_mutex);

    m_cv.wait(l, [this]() { return m_signaled; });

    if (m_autoReset)
        m_signaled = false;
}

bool Condition::wait(std::chrono::milliseconds duration) noexcept
{
    std::unique_lock l(m_mutex);

    auto success = m_cv.wait_for(l, duration, [this]() { return m_signaled; });

    if (m_autoReset)
        m_signaled = false;

    return success;
}


} // namespace Util {}

} // namespace Er {}
