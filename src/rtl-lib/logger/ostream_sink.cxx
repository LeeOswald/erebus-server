#include <erebus/rtl/logger/ostream_sink.hxx>

#include "sink_base.hxx"


namespace Er::Log
{

namespace
{

class OStreamSink
    : public ISink
    , public Private::SinkBase
{
public:
    OStreamSink(std::ostream& stream, FormatterPtr&& formatter, FilterPtr&& filter)
        : Private::SinkBase(std::move(formatter), std::move(filter))
        , m_stream(stream)
    {
    }

    void write(RecordPtr r) override
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

    void write(AtomicRecordPtr&& a) override
    {
        while (auto r = a->pop())
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


ER_RTL_EXPORT ISink::Ptr makeOStreamSink(std::ostream& stream, FormatterPtr&& formatter, FilterPtr&& filter)
{
    return std::make_shared<OStreamSink>(stream, std::move(formatter), std::move(filter));
}

} // namespace Er::Log {}