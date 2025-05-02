#include <erebus/rtl/log.hxx>
#include <erebus/rtl/util/unknown_base.hxx>


#include <mutex>
#include <queue>

namespace Er::Log::Private
{

class NullLogger
    : public Util::SharedBase<Util::ObjectBase<ILogger>>
{
    using Base = Util::SharedBase<Util::ObjectBase<ILogger>>;

public:
    ~NullLogger() = default;

    NullLogger() noexcept
        : Base()
    {
    }

    void indent() noexcept override
    {
    }

    void unindent() noexcept override
    {
    }

    void beginBlock() noexcept override
    {
    }

    void endBlock() noexcept override
    {
    }

    void write(RecordPtr r) override
    {
        std::lock_guard l(m_mutex);
        m_pending.push(r);
    }

    void write(AtomicRecordPtr&& a) override
    {
        std::lock_guard l(m_mutex);

        while (auto r = a->pop())
            m_pending.push(r);
    }

    RecordPtr pop()
    {
        std::lock_guard l(m_mutex);
        if (m_pending.empty())
            return {};

        auto r = m_pending.front();
        m_pending.pop();

        return r;
    }

    void flush() override
    {
    }

    void addSink(std::string_view name, SinkPtr sink) override
    {
    }

    void removeSink(std::string_view name) override
    {
    }

    SinkPtr findSink(std::string_view name) override
    {
        return SinkPtr();
    }

private:
    std::mutex m_mutex;
    std::queue<RecordPtr> m_pending;
};


} // namespace Er::Log::Private {}
