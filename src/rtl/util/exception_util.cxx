#include <erebus/rtl/util/exception_util.hxx>


namespace Er::Util
{

ResultCode ExceptionLogger::operator()(const Exception& e)
{
    auto result = e.find(ExceptionProps::Result);
    if (result)
    {
        m_result = static_cast<ResultCode>(result->getInt32());
    }
    else
    {
        m_result = Result::Failure;
    }

    if (!e.message().empty())
    {
        Log::error(m_log, "{}", e.message());
        m_lastError = e.message();
    }
    else if (result)
    {
        auto code = static_cast<ResultCode>(result->getInt32());
        auto decoded = resultToString(code);
        if (!decoded.empty())
        {
            m_lastError = Er::format("{}: {}", int(code), decoded);
            Log::error(m_log, "{}", m_lastError);
        }
        else
        {
            m_lastError = Er::format("Unexpected error {}", int(code));
            Log::error(m_log, "{}", m_lastError);
        }
    }
    else
    {
        Log::error(m_log, "Unexpected exception");
        m_lastError = "Unexpected exception";
    }

    if (e.location().function_name())
        Log::error(m_log, "in [{}]", e.location().function_name());

    if (e.location().file_name())
        Log::error(m_log, "at [{}]:{}", e.location().file_name(), e.location().line());

    for (auto& prop : e.properties())
    {
        Log::error(m_log, "{}: {}", prop.info()->readableName(), prop.info()->format(prop));
    }

    return result ? static_cast<ResultCode>(result->getInt32()) : Result::Internal;
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
