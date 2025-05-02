#include <erebus/rtl/log.hxx>
#include <erebus/rtl/util/unknown_base.hxx>


namespace Er::Log
{

namespace
{

struct Record
    : public Util::SharedBase<Util::ObjectBase<IRecord>>
{
private:
    using Base = Util::SharedBase<Util::ObjectBase<IRecord>>;

public:
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


} // namespace {}


ER_RTL_EXPORT [[nodiscard]] RecordPtr makeRecord(Level level, Time::ValueType time, uintptr_t tid, const std::string& message)
{
    return SharedPtr<IRecord>{ new Record(level, time, tid, message) };
}

ER_RTL_EXPORT [[nodiscard]] RecordPtr makeRecord(Level level, Time::ValueType time, uintptr_t tid, std::string&& message)
{
    return SharedPtr<IRecord>{ new Record(level, time, tid, std::move(message)) };
}


} // namespace Er::Log {}