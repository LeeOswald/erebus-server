#include <erebus/rtl/rtl.hxx>

#include <atomic>
#include <iostream>
#include <sstream>
#include <syncstream>
#include <thread>

#if ER_ENABLE_STACKTRACE
    #include <boost/stacktrace.hpp>
#endif

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

#if ER_ENABLE_STACKTRACE
std::string formatFailedAssertion(std::source_location location, const char* expression, boost::stacktrace::stacktrace const& stack)
#else
std::string formatFailedAssertion(std::source_location location, const char* expression)
#endif
{
    std::ostringstream ss;
    ss << "\n----------------------------------------------------------\n";
    ss << "ASSERTION FAILED\n";
    ss << "Expression: " << expression << "\n";
    ss << "File: " << location.file_name() << "\n";
    ss << "Line: " << location.line() << "\n";
    ss << "Function: " << location.function_name() << "\n";
#if ER_ENABLE_STACKTRACE
    if (!stack.empty())
    {
        {
            ss << "Backtrace:\n";

            std::size_t skipped = 0;

            auto printSkipped = [&skipped, &ss]()
            {
                if (skipped > 0)
                {
                    if (skipped == 1)
                    {
                        ss << "    \?\?\?\n";
                    }
                    else
                    {
                        ss << "    [" << skipped << " frames skipped]\n";
                    }

                    skipped = 0;
                }
            };

            for (auto& frame : stack)
            {
                if (frame.empty())
                {
                    ++skipped;
                }
                else
                {
                    auto&& name = frame.name();
                    if (name == "boost_stacktrace_impl_return_nullptr")
                    {
                        ++skipped;
                    }
                    else
                    {
                        printSkipped();
                        ss << "    " << frame.name() << "\n";
                    }
                }
            }

            printSkipped();
        }
    }
#endif

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

#if ER_ENABLE_STACKTRACE
#if ER_DEBUG
    static const std::size_t StackFramesToSkip = 3;
#else
    static const std::size_t StackFramesToSkip = 2;
#endif
    static const std::size_t StackFramesToCapture = 256;

    auto msg = formatFailedAssertion(std::move(location), expression, boost::stacktrace::stacktrace{ StackFramesToSkip, StackFramesToCapture });
#else
    auto msg = formatFailedAssertion(std::move(location), expression);
#endif

    doPrintFailedAssertion(msg);

    g_activeAssertions.store(0, std::memory_order_release);
}


} // namespace Er {}