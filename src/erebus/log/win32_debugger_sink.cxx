#include <erebus/log.hxx>
#include <erebus/util/utf16.hxx>

namespace Er::Log
{

namespace
{

class DebuggerSink
    : public ISink
    , public NonCopyable
{
public:
    DebuggerSink(IFormatter::Ptr formatter, IFilter::Ptr filter)
        : m_filter(filter)
        , m_formatter(formatter)
    {
    }

    void write(Record::Ptr r) override
    {
        if (!::IsDebuggerPresent())
            return;

        if (r->level() < level())
            return;

        if (m_filter && !m_filter->filter(r.get()))
            return;

        auto formatted = m_formatter->format(r.get());
        if (formatted.empty())
            return;

        auto u16 = Util::utf8ToUtf16(formatted);
        ::OutputDebugStringW(u16.c_str());
    }

    void flush() override
    {
    }

private:
    const IFilter::Ptr m_filter;
    const IFormatter::Ptr m_formatter;
};

} // namespace {}


EREBUS_EXPORT ISink::Ptr makeDebuggerSink(IFormatter::Ptr formatter, IFilter::Ptr filter)
{
    return std::make_shared<DebuggerSink>(formatter, filter);
}

} // namespace Er::Log {}