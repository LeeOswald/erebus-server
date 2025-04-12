#include <erebus/rtl/rtl.hxx>

#include <atomic>
#include <iostream>
#include <sstream>
#include <syncstream>
#include <thread>


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
    ss << "\n----------------------------------------------------------\n";
    ss << "ASSERTION FAILED\n";
    ss << "Expression: " << expression << "\n";
    ss << "File: " << location.file_name() << "\n";
    ss << "Line: " << location.line() << "\n";
    ss << "Function: " << location.function_name() << "\n";
    ss << "----------------------------------------------------------\n";
    return ss.str();
}

} // namespace {}


ER_RTL_EXPORT void setPrintFailedAssertionFn(PrintFailedAssertionFn f) noexcept
{
    g_printAssertFn.store(f, std::memory_order_release);
}

ER_RTL_EXPORT void printFailedAssertion(std::source_location location, const char* expression) noexcept
{
    // we cannot afford two parallel active assertions
    long expected = 0;
    while (!g_activeAssertions.compare_exchange_strong(expected, 1, std::memory_order_acq_rel))
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto msg = formatFailedAssertion(std::move(location), expression);

    doPrintFailedAssertion(msg);

    g_activeAssertions.store(0, std::memory_order_release);
}


} // namespace Er {}