#include <erebus/rtl/log.hxx>
#include <erebus/rtl/util/null_mutex.hxx>

#include <mutex>
#include <shared_mutex>
#include <unordered_map>


namespace Er::Log
{

namespace
{

template <class MutexT>
class Tee
    : public ITee
    , public boost::noncopyable
{
public:
    void write(Record::Ptr r) override
    {
        std::shared_lock l(m_mutex);
        for (auto& sink : m_sinks)
        {
            sink.second->write(r);
        }
    }

    void write(AtomicRecord a) override
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

    void addSink(std::string_view name, ISink::Ptr sink) override
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

    ISink::Ptr findSink(std::string_view name) override
    {
        std::unique_lock l(m_mutex);
        auto it = m_sinks.find(std::string(name));
        return (it != m_sinks.end()) ? it->second : ISink::Ptr();
    }

private:
    MutexT m_mutex;
    std::unordered_map<std::string, ISink::Ptr> m_sinks;
};


} // namespace {}


ER_RTL_EXPORT ITee::Ptr makeTee(ThreadSafe mode)
{
    if (mode == ThreadSafe::Yes)
        return std::make_shared<Tee<std::shared_mutex>>();
    else
        return std::make_shared<Tee<Util::NullSharedMutex>>();
}


} // namespace Er::Log {}