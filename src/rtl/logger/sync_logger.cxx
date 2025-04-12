#include "logger_base.hxx"


namespace Er::Log
{

namespace
{


class SyncLogger
    : public Private::LoggerBase
{
    using Base = Private::LoggerBase;

public:
    ~SyncLogger() = default;

    SyncLogger(std::string_view component)
        : Base(component, makeTee(ThreadSafe::Yes))
    {
    }

    void write(Record::Ptr r) override
    {
        if (!r) [[unlikely]]
            return;

        if (r->level() < m_level)
            return;

        auto indent = m_threadData.data().indent;
        if (indent > 0)
            r->setIndent(indent);

        if (!m_component.empty() && r->component().empty())
            r->setComponent(m_component);

        m_tee->write(r);
    }

    void flush() override
    {
    }
};


} // namespace {}


ER_RTL_EXPORT ILogger::Ptr makeSyncLogger(std::string_view component)
{
    return std::make_shared<SyncLogger>(component);
}

} // namespace Er::Log {}