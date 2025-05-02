#include <erebus/rtl/logger/win32_debugger_sink.hxx>
#include <erebus/rtl/util/utf16.hxx>


namespace Er::Log
{

namespace
{

class DebuggerSink
    : public SinkBase
{
public:
    DebuggerSink(IFormatter::Ptr&& formatter, Filter&& filter)
        : SinkBase(std::move(formatter), std::move(filter))
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

    void write(AtomicRecordPtr&& a) override
    {
        while (auto r = a->pop())
            write(r);
    }

    void flush() override
    {
    }
};

} // namespace {}


ER_RTL_EXPORT ISink::Ptr makeDebuggerSink(IFormatter::Ptr&& formatter, Filter&& filter)
{
    return std::make_shared<DebuggerSink>(std::move(formatter), std::move(filter));
}

} // namespace Er::Log {}