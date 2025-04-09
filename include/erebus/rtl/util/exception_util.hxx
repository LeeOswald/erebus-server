#pragma once

#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/log.hxx>
#include <erebus/rtl/result.hxx>


namespace Er::Util
{

class ER_RTL_EXPORT ExceptionLogger
{
public:
    ExceptionLogger(Log::ILogger* log)
        : m_log(log)
    {}

    ResultCode operator()(const Exception& e);
    ResultCode operator()(const std::bad_alloc& e);
    ResultCode operator()(const std::bad_cast& e);
    ResultCode operator()(const std::length_error& e);
    ResultCode operator()(const std::out_of_range& e);
    ResultCode operator()(const std::invalid_argument& e);
    ResultCode operator()(const std::exception& e);
    ResultCode operator()(const std::exception_ptr& ep);

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