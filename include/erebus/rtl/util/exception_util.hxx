#pragma once

#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/log.hxx>



namespace Er::Util
{

ER_RTL_EXPORT void logException(Log::ILogger* log, Log::Level level, const Exception& e);


class ER_RTL_EXPORT ExceptionLogger
{
public:
    ExceptionLogger(Log::ILogger* log)
        : m_log(log)
    {}

    void operator()(const Exception& e);
    void operator()(const std::bad_alloc& e);
    void operator()(const std::exception& e);
    void operator()(const std::exception_ptr& ep);

    const std::string& lastError() const noexcept
    {
        return m_lastError;
    }

    const std::optional<ResultCode>& result() const noexcept
    {
        return m_result;
    }

private:
    Log::ILogger* m_log;
    std::string m_lastError;
    std::optional<ResultCode> m_result;
};

} // namespace Er::Util {}