#include <erebus/rtl/logger/win32_debugger_sink.hxx>
#include <erebus/rtl/util/utf16.hxx>

#include "sink_base.hxx"


namespace Er::Log
{

namespace
{

class DebuggerSink
    : public Private::SinkBase
{
public:
    DebuggerSink(FormatterPtr&& formatter, FilterPtr&& filter)
        : Private::SinkBase(std::move(formatter), std::move(filter))
    {
    }

    void write(RecordPtr r) override
    {
        if (!::IsDebuggerPresent())
            return;

        if (!filter(r.get()))
            return;

        auto formatted = format(r.get());
        if (formatted.empty())
            return;

        auto u16 = Util::utf8ToUtf16(formatted);
        ::OutputDebugStringW(u16.c_str());
    }

    void write(AtomicRecordPtr a) override
    {
        auto count = a->size();
        for (decltype(count) i = 0; i < count; ++i)
            write(a->get(i));
    }

    void flush() override
    {
    }
};

} // namespace {}


ER_RTL_EXPORT SinkPtr makeDebuggerSink(FormatterPtr&& formatter, FilterPtr&& filter)
{
    return SinkPtr(new DebuggerSink(std::move(formatter), std::move(filter)));
}

} // namespace Er::Log {}