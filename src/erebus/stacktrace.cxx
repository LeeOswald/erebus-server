#include <erebus/stacktrace.hxx>

#include <sstream>


namespace Er
{

DecodedStackTrace EREBUS_EXPORT decodeStackTrace(const StackTrace& stack)
{
    DecodedStackTrace decoded;
    if (stack.empty())
        return decoded;

    decoded.reserve(stack.size());
    for (auto& frame : stack)
    {
        if (!frame.empty())
            decoded.emplace_back(frame.name());
        else
            decoded.emplace_back("<NULL>");
    }

    return decoded;
}



} // namespace Er {}