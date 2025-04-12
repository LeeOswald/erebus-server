#include <erebus/rtl/log.hxx>

#include <mutex>
#include <queue>

namespace Er::Log
{

struct NullLogger
    : public ILogger
    , public boost::noncopyable
{
    ~NullLogger() = default;
    NullLogger() = default;

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

    void write(Record::Ptr r) override
    {
        auto& s = state();

        std::lock_guard l(s.mutex);
        s.pending.push(r);
    }

    void write(AtomicRecord a) override
    {
        auto& s = state();

        std::lock_guard l(s.mutex);

        for (auto r : a)
            s.pending.push(r);
    }

    Record::Ptr pop()
    {
        auto& s = state();

        std::lock_guard l(s.mutex);
        if (s.pending.empty())
            return {};

        auto r = s.pending.front();
        s.pending.pop();

        return r;
    }

    void flush() override
    {
    }

    void addSink(std::string_view name, ISink::Ptr sink) override
    {
    }

    void removeSink(std::string_view name) override
    {
    }

    ISink::Ptr findSink(std::string_view name) override
    {
        return ISink::Ptr();
    }

private:
    struct State
    {
        std::mutex mutex;
        std::queue<Record::Ptr> pending;
    };

    static State& state() noexcept
    {
        static State s;
        return s;
    }
};


} // namespace Er::Log {}
