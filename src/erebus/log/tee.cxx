#include <erebus/log.hxx>
#include <erebus/null_mutex.hxx>

#include <shared_mutex>
#include <unordered_map>

namespace Er::Log
{

namespace
{

template <class MutexT>
class Tee
    : public ITee
    , public NonCopyable
{
public:
    ~Tee() = default;
    Tee() = default;

    void write(Record::Ptr r) override
    {
        if (r->level() < level())
            return;

        std::shared_lock l(m_mutex);
        for (auto& sink : m_sinks)
            sink.second->write(r);
    }

    void flush() override
    {
        std::shared_lock l(m_mutex);
        for (auto& sink : m_sinks)
            sink.second->flush();
    }

    void addSink(std::string_view name, ISink::Ptr sink) override
    {
        std::unique_lock l(m_mutex);
        m_sinks.insert({ std::string(name), sink });
    }

    void removeSink(std::string_view name) override
    {
        std::unique_lock l(m_mutex);
        m_sinks.erase(std::string(name));
    }

private:
    MutexT m_mutex;
    std::unordered_map<std::string, ISink::Ptr> m_sinks;
};

} // namespace {}



EREBUS_EXPORT ITee::Ptr makeTee(ThreadSafe mode)
{
    if (mode == ThreadSafe::No)
        return std::make_shared<Tee<Er::NullSharedMutex>>();
    else
        return std::make_shared<Tee<std::shared_mutex>>();
}

} // namespace Er::Log {}