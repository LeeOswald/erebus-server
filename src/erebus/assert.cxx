#include <erebus/exception.hxx>

#include <sstream>


namespace Er
{

namespace Private
{

EREBUS_EXPORT void failAssert(Location&& location, const char* expression)
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

    throw Er::Exception(std::move(tempLocation), ss.str(), Er::ExceptionProps::FailedAssertion(expression));
}

} // namespace Private {}

} // namespace Er {}