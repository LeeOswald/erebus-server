#include <erebus/rtl/logger/ostream_sink.hxx>

#include "sink_base.hxx"


namespace Er::Log
{

namespace
{

class OStreamSink
    : public Private::SinkBase
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

    void write(AtomicRecordPtr a) override
    {
        auto& recs = a->get();
        for (auto& r : recs)
            write(r);
    }

    bool flush(std::chrono::milliseconds) override
    {
        m_stream.flush();
        return true;
    }

private:
    std::ostream& m_stream;
};

} // namespace {}


ER_RTL_EXPORT SinkPtr makeOStreamSink(std::ostream& stream, FormatterPtr&& formatter, FilterPtr&& filter)
{
    return SinkPtr(new OStreamSink(stream, std::move(formatter), std::move(filter)));
}

} // namespace Er::Log {}