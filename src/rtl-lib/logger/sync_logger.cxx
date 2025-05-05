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

    void doWrite(RecordPtr r) override
    {
        m_tee->write(r);
    }

    void doWrite(AtomicRecordPtr a) override
    {
        m_tee->write(a);
    }

    bool flush(std::chrono::milliseconds timeout) override
    {
        return m_tee->flush(timeout);
    }
};


} // namespace {}


ER_RTL_EXPORT LoggerPtr makeSyncLogger(std::string_view component)
{
    return LoggerPtr(new SyncLogger(component));
}

} // namespace Er::Log {}