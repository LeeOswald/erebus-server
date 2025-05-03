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
    : public Util::SharedBase<Util::ObjectBase<IAtomicRecord>>
{
    using Base = Util::SharedBase<Util::ObjectBase<IAtomicRecord>>;

public:
    ~AtomicRecord() = default;

    AtomicRecord()
        : Base()
    {
    }

    explicit AtomicRecord(std::vector<RecordPtr>&& records)
        : Base()
        , m_records(std::move(records))
    {
    }

    std::size_t size() const noexcept override
    {
        return m_records.size();
    }

    RecordPtr get(std::size_t index) const noexcept override
    {
        ErAssert(index < m_records.size());
        if (index < m_records.size())
            return m_records[index];

        return {};
    }

private:
    std::vector<RecordPtr> m_records;
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

ER_RTL_EXPORT AtomicRecordPtr makeAtomicRecord(std::vector<RecordPtr>&& v)
{
    return AtomicRecordPtr{ new AtomicRecord(std::move(v)) };
}

} // namespace Er::Log {}