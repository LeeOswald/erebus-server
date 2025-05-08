#include <erebus/rtl/util/exception_util.hxx>


namespace Er::Util
{

ER_RTL_EXPORT void logException(Log::ILogger* log, Log::Level level, const Exception& e)
{
    Log::AtomicBlock lb(log);

    Log::writeln(log, level, "--------------------------------------------------------------");
    Log::writeln(log, level, e.message());

    if (e.location().function_name())
        Log::write(log, level, "in [{}]", e.location().function_name());

    if (e.location().file_name())
        Log::write(log, level, "at [{}]:{}", e.location().file_name(), e.location().line());

    {
        Log::IndentScope is(log, level);

        for (auto& prop : e.properties())
        {
            if ((prop.nameStr() == ExceptionProperties::Brief::Name) || (prop.nameStr() == ExceptionProperties::Message::Name))
                continue;

            auto fmt = Er::findPropertyFormatter(prop.semantics());
            ErAssert(fmt);

            Log::write(log, level, "{}: {}", prop.nameStr(), fmt(prop));
        }
    }

    Log::writeln(log, level, "--------------------------------------------------------------");
}


void ExceptionLogger::operator()(const Exception& e)
{
    logException(m_log, Log::Level::Error, e);

    m_lastError = e.message();
    if (e.category() == GenericError)
        m_result = e.code();
}

void ExceptionLogger::operator()(const std::bad_alloc& e)
{
    Log::error(m_log, "{}", e.what());

    m_lastError = e.what();
    m_result = Result::OutOfMemory;
}

void ExceptionLogger::operator()(const std::exception& e)
{
    Log::error(m_log, "{}", e.what());

    m_lastError = e.what();
    m_result = Result::Internal;
}

void ExceptionLogger::operator()(const std::exception_ptr& ep)
{
    Log::error(m_log, "Unexpected exception");

    m_lastError = "Unexpected exception";
    m_result = Result::Internal;
}

} // namespace Er::Util {}
