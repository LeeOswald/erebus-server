#include <erebus/erebus.hxx>
#include <erebus/stacktrace.hxx>

#include <sstream>


namespace Er
{

DecodedStackTrace EREBUS_EXPORT decodeStackTrace(const StackTrace& stack)
{
    DecodedStackTrace decoded;
    if (stack.get().empty())
        return decoded;

    decoded.reserve(stack.get().size());
    bool skip = true;
    for (auto& frame : stack.get())
    {
        if (!frame.empty())
        {
            auto name = frame.name();
            if (skip && (name.starts_with("boost::stacktrace") || name.starts_with("Er::StackTrace")))
            {
                // skip stacktrace stuff
            }
            else
            {
                skip = false;
                decoded.push_back(std::move(name));
            }
        }
        else if (!skip)
        {
            decoded.emplace_back("<NULL>");
        }
    }

    return decoded;
}



} // namespace Er {}