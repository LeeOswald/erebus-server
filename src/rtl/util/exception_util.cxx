#include <erebus/rtl/util/exception_util.hxx>


namespace Er::Util
{

namespace
{

class PropertyPrinter
{
public:
    ~PropertyPrinter()
    {
        m_log->unindent();
    }

    PropertyPrinter(Log::ILogger* log, Log::Level level)
        : m_log(log)
        , m_level(level)
    {
        m_log->indent();
    }

    bool operator()(const Property& prop, const Empty& v)
    {
        Log::write(m_log, m_level, "{}: {}", prop.name(), prop.str());
        return true;
    }

    bool operator()(const Property& prop, const Bool& v)
    {
        auto& formatter = findPropertyFormatter(prop.semantics());
        Log::write(m_log, m_level, "{}: {}", prop.name(), formatter(prop));
        return true;
    }

    bool operator()(const Property& prop, const std::int32_t& v)
    {
        auto& formatter = findPropertyFormatter(prop.semantics());
        Log::write(m_log, m_level, "{}: {}", prop.name(), formatter(prop));
        return true;
    }

    bool operator()(const Property& prop, const std::uint32_t& v)
    {
        auto& formatter = findPropertyFormatter(prop.semantics());
        Log::write(m_log, m_level, "{}: {}", prop.name(), formatter(prop));
        return true;
    }

    bool operator()(const Property& prop, const std::int64_t& v)
    {
        auto& formatter = findPropertyFormatter(prop.semantics());
        Log::write(m_log, m_level, "{}: {}", prop.name(), formatter(prop));
        return true;
    }

    bool operator()(const Property& prop, const std::uint64_t& v)
    {
        auto& formatter = findPropertyFormatter(prop.semantics());
        Log::write(m_log, m_level, "{}: {}", prop.name(), formatter(prop));
        return true;
    }

    bool operator()(const Property& prop, const double& v)
    {
        auto& formatter = findPropertyFormatter(prop.semantics());
        Log::write(m_log, m_level, "{}: {}", prop.name(), formatter(prop));
        return true;
    }

    bool operator()(const Property& prop, const std::string& v)
    {
        auto& formatter = findPropertyFormatter(prop.semantics());
        Log::write(m_log, m_level, "{}: {}", prop.name(), formatter(prop));
        return true;
    }

    bool operator()(const Property& prop, const Binary& v)
    {
        auto& formatter = findPropertyFormatter(prop.semantics());
        Log::write(m_log, m_level, "{}: {}", prop.name(), formatter(prop));
        return true;
    }

    bool operator()(const Property& prop, const PropertyMap& v)
    {
        Log::write(m_log, m_level, "{}: {", prop.name());

        PropertyPrinter nested(m_log, m_level);
        visit(v, nested);

        Log::write(m_log, m_level, "}", prop.name());
        return true;
    }

private:
    Log::ILogger* m_log;
    Log::Level m_level;
};

} // namespace {}


ResultCode ExceptionLogger::operator()(const Exception& e)
{
    Log::AtomicBlock lb(m_log);

    Log::writeln(m_log, Log::Level::Error, "--------------------------------------------------------------");
    Log::writeln(m_log, Log::Level::Error, e.message());

    if (e.location().function_name())
        Log::error(m_log, "in [{}]", e.location().function_name());

    if (e.location().file_name())
        Log::error(m_log, "at [{}]:{}", e.location().file_name(), e.location().line());

    {
        PropertyPrinter pp(m_log, Log::Level::Error);
        visit(e.properties(), pp);
    }

    Log::writeln(m_log, Log::Level::Error, "--------------------------------------------------------------");

    m_lastError = e.message();
    auto r = findProperty(e.properties(), ExceptionProps::ResultCode);
    if (r)
        m_result = *r->getInt32();

    return m_result ? *m_result : Result::Internal;
}

ResultCode ExceptionLogger::operator()(const std::bad_alloc& e)
{
    Log::error(m_log, "{}", e.what());
    m_lastError = e.what();
    m_result = Result::OutOfMemory;
    return Result::OutOfMemory;
}

ResultCode ExceptionLogger::operator()(const std::bad_cast& e)
{
    Log::error(m_log, "{}", e.what());
    m_lastError = e.what();
    m_result = Result::Internal;
    return Result::Internal;
}

ResultCode ExceptionLogger::operator()(const std::length_error& e)
{
    Log::error(m_log, "{}", e.what());
    m_lastError = e.what();
    m_result = Result::Internal;
    return Result::Internal;
}

ResultCode ExceptionLogger::operator()(const std::out_of_range& e)
{
    Log::error(m_log, "{}", e.what());
    m_lastError = e.what();
    m_result = Result::Internal;
    return Result::Internal;
}

ResultCode ExceptionLogger::operator()(const std::invalid_argument& e)
{
    Log::error(m_log, "{}", e.what());
    m_lastError = e.what();
    m_result = Result::Internal;
    return Result::Internal;
}

ResultCode ExceptionLogger::operator()(const std::exception& e)
{
    Log::error(m_log, "{}", e.what());
    m_lastError = e.what();
    m_result = Result::Internal;
    return Result::Internal;
}

ResultCode ExceptionLogger::operator()(const std::exception_ptr& ep)
{
    Log::error(m_log, "Unexpected exception");
    m_lastError = "Unexpected exception";
    m_result = Result::Internal;
    return Result::Internal;
}

} // namespace Er::Util {}
