#include <erebus/rtl/log.hxx>
#include <erebus/rtl/util/unknown_base.hxx>


#include <mutex>
#include <queue>

namespace Er::Log::Private
{

class NullLogger
    : public ILogger
{
public:
    ~NullLogger() = default;

    NullLogger() noexcept
    {
    }

    void addRef() noexcept override
    {
    }

    void release() noexcept override
    {
    }

    IUnknown* queryInterface(std::string_view iid) noexcept
    {
        if ((IID == IUnknown::IID) ||
            (IID == ISink::IID) ||
            (IID == ITee::IID) ||
            (IID == ILogger::IID))
        {
            return this;
        }

        return nullptr;
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
        auto& s = state();

        std::lock_guard l(s.mutex);
        s.pending.push(r);
    }

    void write(AtomicRecordPtr a) override
    {
        auto& s = state();

        std::lock_guard l(s.mutex);

        auto& recs = a->get();
        for (auto& r: recs)
            s.pending.push(r);
    }

    RecordPtr pop()
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

    void addSink(std::string_view name, SinkPtr sink) override
    {
    }

    void removeSink(std::string_view name) override
    {
    }

    SinkPtr findSink(std::string_view name) override
    {
        return SinkPtr{};
    }

private:
    struct State
    {
        std::mutex mutex;
        std::queue<RecordPtr> pending;
    };

    static State& state()
    {
        static State s;
        return s;
    }
};


} // namespace Er::Log::Private {}
