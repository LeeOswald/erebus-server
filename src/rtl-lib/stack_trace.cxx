#include <erebus/rtl/stack_trace.hxx>


namespace Er
{

void filterStackTrace(const StackTrace& stack, StackFrameCallback&& filter)
{
    if (stack.empty())
        return;

    std::size_t skipped = 0;

    auto filterSkipped = [&skipped, &filter]()
    {
        if (skipped > 0)
        {
            filter(StackFrame{skipped});

            skipped = 0;
        }
    };

    auto isBadName = [](std::string const& name) -> bool
    {
        static const std::string_view BadNames[] = 
        {
            {"boost_stacktrace_impl_return_nullptr"},
            {"boost::stacktrace"},
            {"Er::Exception::Exception"},
            {"Er::printFailedAssertion"},
            {"__static_initialization_and_destruction"},
            {"Er::Program::terminateHandler"},
            {"Er::Program::staticTerminateHandler"},
        };

        if (name.empty())
            return true;

        for (auto& bad: BadNames)
        {
            if (name.starts_with(bad))
                return true;
        }

        return false;
    };

    auto it = stack.begin();
    while (it != stack.end())
    {
        auto& frame = *it;
        
        if (frame.empty() || isBadName(frame.name()))
        {
            ++it;
            continue;
        }

        break;
    }

    while (it != stack.end())
    {
        auto& frame = *it;
        ++it;

        if (frame.empty())
        {
            ++skipped;
        }
        else
        {
            auto name = frame.name();
            if (name.empty() && frame.address() != 0)
            {
                filter(StackFrame{ frame.address(), std::string{"\?\?\?"} });
            }
            else if (isBadName(name))
            {
                ++skipped;
            }
            else
            {
                filterSkipped();
                filter(StackFrame{ frame.address(), std::move(name) });
            }
        }
    }

    filterSkipped();
}


} // namespace Er {}

