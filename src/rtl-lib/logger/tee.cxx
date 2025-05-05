#include <erebus/rtl/log.hxx>
#include <erebus/rtl/util/null_mutex.hxx>
#include <erebus/rtl/util/unknown_base.hxx>

#include <mutex>
#include <shared_mutex>
#include <unordered_map>


namespace Er::Log
{

namespace
{

template <class MutexT>
class Tee
    : public Util::ReferenceCountedBase<Util::ObjectBase<ITee>>
{
    using Base = Util::ReferenceCountedBase<Util::ObjectBase<ITee>>;

public:
    ~Tee() = default;

    Tee() noexcept
        : Base()
    {
    }

    void write(RecordPtr r) override
    {
        std::shared_lock l(m_mutex);
        for (auto& sink : m_sinks)
        {
            sink.second->write(r);
        }
    }

    void write(AtomicRecordPtr a) override
    {
        std::shared_lock l(m_mutex);
        for (auto& sink : m_sinks)
        {
            sink.second->write(a);
        }
    }

    void flush() override
    {
        std::shared_lock l(m_mutex);
        for (auto& sink : m_sinks)
        {
            sink.second->flush();
        }
    }

    void addSink(std::string_view name, SinkPtr sink) override
    {
        std::unique_lock l(m_mutex);
        auto r = m_sinks.insert({ std::string(name), sink });
        ErAssert(r.second);
    }

    void removeSink(std::string_view name) override
    {
        std::unique_lock l(m_mutex);
        m_sinks.erase(std::string(name));
    }

    SinkPtr findSink(std::string_view name) override
    {
        std::unique_lock l(m_mutex);
        auto it = m_sinks.find(std::string(name));
        return (it != m_sinks.end()) ? it->second : SinkPtr();
    }

private:
    MutexT m_mutex;
    std::unordered_map<std::string, SinkPtr> m_sinks;
};


} // namespace {}


ER_RTL_EXPORT TeePtr makeTee(ThreadSafe mode)
{
    if (mode == ThreadSafe::Yes)
        return TeePtr(new Tee<std::shared_mutex>());
    else
        return TeePtr(new Tee<Util::NullSharedMutex>());
}


} // namespace Er::Log {}