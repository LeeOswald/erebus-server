#include <erebus/assert.hxx>

#include <atomic>
#include <iostream>
#include <sstream>
#include <syncstream>


namespace Er
{

static std::atomic<PrintFailedAssertionFn> g_printAssertFn{ nullptr };
static std::atomic<long> g_activeAssertions{ 0 };

static void defaultPrintFailedAssertionFn(std::string_view message)
{
    std::osyncstream(std::cerr) << message << std::endl; // force flush
}

EREBUS_EXPORT void setPrintFailedAssertionFn(PrintFailedAssertionFn f) noexcept
{
    g_printAssertFn.store(f, std::memory_order_release);
}

static void doPrintFailedAssertion(std::string_view message)
{
    auto f = g_printAssertFn.load(std::memory_order_acquire);
    if (f)
        f(message);
    else
        defaultPrintFailedAssertionFn(message);
}

static std::string formatFailedAssertion(Location&& location, const char* expression)
{
    std::ostringstream ss;
    ss << "ASSERTION FAILED\n";
    
    if (expression)
    {
        ss << "Expression: " << expression << "\n";
    }

    Location tempLocation(std::move(location));

    if (tempLocation.source)
    {
        ss << "File: " << tempLocation.source->file() << "\n";
        ss << "Line: " << tempLocation.source->line() << "\n";
    }

    if (!tempLocation.decoded && tempLocation.stack)
    {
        tempLocation.decoded = Er::decodeStackTrace(*tempLocation.stack);
    }

    if (tempLocation.decoded && !tempLocation.decoded->empty())
    {
        std::ostringstream stk;
        bool empty = true;
        for (auto& frm: *tempLocation.decoded)
        {
            if (!frm.empty())
            {
                if (!empty)
                    stk << " <- ";
                else
                    empty = false;

                stk << frm;
            }
        }

        ss << "Stack: " << stk.str() << "\n";
    }

    return ss.str();
}

EREBUS_EXPORT void printFailedAssertion(Location&& location, const char* expression) noexcept
{
    if (++g_activeAssertions == 1)
    {
        try
        {
            auto msg = formatFailedAssertion(std::move(location), expression);

            doPrintFailedAssertion(msg);
        }
        catch (...)
        {
        }
    }

    --g_activeAssertions;
}


} // namespace Er {}