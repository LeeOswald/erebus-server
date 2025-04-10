#include <erebus/rtl/platform.hxx>

#include <atomic>
#include <iostream>
#include <sstream>
#include <syncstream>


namespace Er
{

namespace
{

std::atomic<PrintFailedAssertionFn> g_printAssertFn{ nullptr };
std::atomic<long> g_activeAssertions{ 0 };

void defaultPrintFailedAssertionFn(std::string_view message)
{
    std::osyncstream(std::cerr) << message << std::endl; // force flush
}

void doPrintFailedAssertion(std::string_view message)
{
    auto f = g_printAssertFn.load(std::memory_order_acquire);
    if (f)
        f(message);
    else
        defaultPrintFailedAssertionFn(message);
}

std::string formatFailedAssertion(std::source_location location, const char* expression)
{
    std::ostringstream ss;
    ss << "ASSERTION FAILED\n";
    ss << "Expression: " << expression << "\n";
    ss << "File: " << location.file_name() << "\n";
    ss << "Line: " << location.line() << "\n";
    ss << "Function: " << location.function_name() << "\n";

    return ss.str();
}

} // namespace {}


ER_RTL_EXPORT void setPrintFailedAssertionFn(PrintFailedAssertionFn f) noexcept
{
    g_printAssertFn.store(f, std::memory_order_release);
}

ER_RTL_EXPORT void printFailedAssertion(std::source_location location, const char* expression) noexcept
{
    if (++g_activeAssertions == 1)
    {
        auto msg = formatFailedAssertion(std::move(location), expression);

        doPrintFailedAssertion(msg);
    }
    else
    {
        // we cannot afford nested assertions
        std::abort();
    }

    --g_activeAssertions;
}


} // namespace Er {}