#include <erebus/rtl/logger/ostream_sink.hxx>


namespace Er::Log
{

namespace
{

class OStreamSink
    : public SinkBase
{
public:
    OStreamSink(std::ostream& stream, IFormatter::Ptr&& formatter, Filter&& filter)
        : SinkBase(std::move(formatter), std::move(filter))
        , m_stream(stream)
    {
    }

    void write(Record::Ptr r) override
    {
        if (!filter(r.get()))
            return;

        auto formatted = format(r.get());
        const auto available = formatted.length();
        if (!available)
            return;

        const auto data = formatted.data();

        m_stream.write(data, available);
    }

    void write(AtomicRecord a) override
    {
        for (auto r : a)
            write(r);
    }

    void flush() override
    {
        m_stream.flush();
    }

private:
    std::ostream& m_stream;
};

} // namespace {}


ER_RTL_EXPORT ISink::Ptr makeOStreamSink(std::ostream& stream, IFormatter::Ptr&& formatter, Filter&& filter)
{
    return std::make_shared<OStreamSink>(stream, std::move(formatter), std::move(filter));
}

} // namespace Er::Log {}