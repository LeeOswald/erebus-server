#include "logger_base.hxx"

#include <erebus/rtl/empty.hxx>

namespace Er::Log
{

namespace
{


class SyncLogger
    : public Private::LoggerBase<Empty>
{
    using Base = Private::LoggerBase<Empty>;

public:
    ~SyncLogger() = default;

    SyncLogger(std::string_view component)
        : Base(component, makeTee(ThreadSafe::Yes))
    {
    }

    void doWrite(Record::Ptr r) override
    {
        m_tee->write(r);
    }

    void doWrite(AtomicRecord a) override
    {
        m_tee->write(a);
    }

    void flush() override
    {
        m_tee->flush();
    }
};


} // namespace {}


ER_RTL_EXPORT ILogger::Ptr makeSyncLogger(std::string_view component)
{
    return std::make_shared<SyncLogger>(component);
}

} // namespace Er::Log {}