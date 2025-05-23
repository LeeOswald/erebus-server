#include <erebus/rtl/logger/simple_formatter.hxx>
#include <erebus/rtl/util/unknown_base.hxx>

#include <algorithm>

namespace Er::Log
{

namespace
{

class SimpleFormatterImpl
    : public Util::ReferenceCountedBase<Util::ObjectBase<IFormatter>>
    , public SimpleFormatter
{
    using Base = Util::ReferenceCountedBase<Util::ObjectBase<IFormatter>>;

public:
    ~SimpleFormatterImpl() = default;

    std::string format(const Record* r) const override
    {
        std::tm time{};
        std::uint32_t msec = 0;

        if (m_options[Option::DateTime] || m_options[Option::Time])
        {
            Time pt(r->time());
            if (m_options[Option::TzUtc])
                time = pt.toUtc();
            else
                time = pt.toLocalTime();

            msec = pt.subSecond() / 1000;
        }

        std::ostringstream ss;
        bool prefixEmpty = true;

        if (m_needPrefix)
            ss << "[";

        if (m_options[Option::DateTime])
        {
            ss << Er::format("{:04}/{:02}/{:02} {:02}:{:02}:{:02}.{:03}", time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec, msec);
            prefixEmpty = false;
        }
        else if (m_options[Option::Time])
        {
            ss << Er::format("{:02}:{:02}:{:02}.{:03}", time.tm_hour, time.tm_min, time.tm_sec, msec);
            prefixEmpty = false;
        }

        if (m_options[Option::Level])
        {
            if (!prefixEmpty)
                ss << " ";

            switch (r->level())
            {
            case Log::Level::Debug: ss << "D"; break;
            case Log::Level::Info: ss << "I"; break;
            case Log::Level::Warning: ss << "W"; break;
            case Log::Level::Error: ss << "E"; break;
            case Log::Level::Fatal: ss << "!"; break;
            default: ss << "?"; break;
            }

            prefixEmpty = false;
        }

        if (m_options[Option::Component] && !r->component().empty())
        {
            if (!prefixEmpty)
                ss << " ";

            ss << r->component();
            prefixEmpty = false;
        }

        if (m_options[Option::Tid])
        {
            if (!prefixEmpty)
                ss << " ";

            ss << "@" << r->tid();
            prefixEmpty = false;
        }

        if (m_needPrefix)
        {
            ss << "]";
            prefixEmpty = false;
        }

        if (!r->message().empty())
        {
            if (!prefixEmpty)
                ss << " ";

            for (unsigned i = 0; i < r->indent(); ++i)
                ss << m_indent;

            ss << r->message();
        }

        if (!m_options[Option::NoLf])
        {
            if (m_options[Option::CrLf])
                ss << "\r\n";
            else
                ss << "\n";
        }

        return ss.str();
    }
    
    SimpleFormatterImpl(Options options, unsigned indentSize)
        : m_options(options)
        , m_indent(std::clamp(indentSize, 1u, MaxIndent), ' ')
    {
        m_needPrefix = m_options[Option::DateTime] | m_options[Option::Time] |
            m_options[Option::Level] | m_options[Option::Tid];
    }

private:
    static constexpr unsigned MaxIndent = 64;

    const Options m_options;
    const std::string m_indent;
    bool m_needPrefix;
};


} // namespace {}


ER_RTL_EXPORT FormatterPtr makeSimpleFormatter(SimpleFormatter::Options options, unsigned indentSize)
{
    return FormatterPtr(new SimpleFormatterImpl(options, indentSize));
}

} // namespace Er::Log {}