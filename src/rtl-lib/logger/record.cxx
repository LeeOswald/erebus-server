#include <erebus/rtl/log.hxx>
#include <erebus/rtl/util/unknown_base.hxx>

#include <queue>


namespace Er::Log
{

namespace
{

struct Record
    : public Util::SharedBase<Util::ObjectBase<IRecord>>
{
public:
    ~Record() = default;

    Record(Level level, Time::ValueType time, uintptr_t tid, auto&& message)
        : m_level(level)
        , m_time(time)
        , m_tid(tid)
        , m_message(std::forward<decltype(message)>(message))
    {
    }

    Level level() const noexcept override
    {
        return m_level;
    }

    Time::ValueType time() const noexcept override
    {
        return m_time;
    }

    std::uintptr_t tid() const noexcept override
    {
        return m_tid;
    }

    std::string_view component() const noexcept override
    {
        return m_component;
    }

    const std::string& message() const noexcept override
    {
        return m_message;
    }

    std::uint32_t indent() const noexcept override
    {
        return m_indent;
    }

    void setComponent(std::string_view component) noexcept override
    {
        m_component = component;
    }

    void setIndent(unsigned indent) noexcept override
    {
        m_indent = indent;
    }

private:
    std::string_view m_component;
    const Level m_level = Level::Info;
    const Time::ValueType m_time;
    const uintptr_t m_tid = 0;
    const std::string m_message;
    std::uint32_t m_indent = 0;
};


class AtomicRecord
    : public Util::DisposableBase<Util::ObjectBase<IAtomicRecord>>
{
    using Base = Util::DisposableBase<Util::ObjectBase<IAtomicRecord>>;

public:
    ~AtomicRecord() = default;

    AtomicRecord()
        : Base(nullptr)
    {
    }

    explicit AtomicRecord(const std::queue<RecordPtr> records)
        : Base(nullptr)
        , m_records(records)
    {
    }

    bool empty() const noexcept
    {
        return m_records.empty();
    }

    void push(RecordPtr r) override
    {
        m_records.push(r);
    }

    RecordPtr pop() override
    {
        if (!m_records.empty())
        {
            auto r = m_records.front();
            m_records.pop();
            return r;
        }

        return {};
    }

    IAtomicRecord* clone() const override
    {
        return new AtomicRecord{ m_records };
    }

private:
    std::queue<RecordPtr> m_records;
};


} // namespace {}


ER_RTL_EXPORT RecordPtr makeRecord(Level level, Time::ValueType time, uintptr_t tid, const std::string& message)
{
    return RecordPtr{ new Record(level, time, tid, message) };
}

ER_RTL_EXPORT RecordPtr makeRecord(Level level, Time::ValueType time, uintptr_t tid, std::string&& message)
{
    return RecordPtr{ new Record(level, time, tid, std::move(message)) };
}

ER_RTL_EXPORT AtomicRecordPtr makeAtomicRecord()
{
    return AtomicRecordPtr{ new AtomicRecord() };
}

} // namespace Er::Log {}