#include <erebus/rtl/log.hxx>
#include <erebus/rtl/system/thread.hxx>
#include <erebus/rtl/util/thread_data.hxx>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>


namespace Er::Log
{

namespace
{


class SyncLogger
    : public ILogger
    , public boost::noncopyable
{
public:
    ~SyncLogger()
    {
    }

    SyncLogger(std::string_view component)
        : m_component(component)
        , m_tee(makeTee(ThreadSafe::Yes))
    {
    }

    void indent() noexcept override
    {
        auto& td = m_threadData.data();
        ++td.indent;
    }

    void unindent() noexcept override
    {
        auto& td = m_threadData.data();
        ErAssert(td.indent > 0);
        --td.indent;
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

    void addSink(std::string_view name, ISink::Ptr sink) override
    {
        m_tee->addSink(name, sink);
    }

    void removeSink(std::string_view name) override
    {
        m_tee->removeSink(name);
    }

    ISink::Ptr findSink(std::string_view name) override
    {
        return m_tee->findSink(name);
    }

private:
    struct PerThread
    {
        unsigned indent = 0;
    };

    using ThreadDataHolder = ThreadData<PerThread>;
    std::string_view m_component;
    ITee::Ptr m_tee;
    ThreadDataHolder m_threadData;
};


} // namespace {}


ER_RTL_EXPORT ILogger::Ptr makeSyncLogger(std::string_view component)
{
    return std::make_shared<SyncLogger>(component);
}

} // namespace Er::Log {}