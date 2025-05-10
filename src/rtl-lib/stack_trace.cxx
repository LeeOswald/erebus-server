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

    auto isBadName = [](std::string const& name)
    {
        return name.empty() ||
            (name == std::string_view("boost_stacktrace_impl_return_nullptr")) ||
            (name.starts_with("Er::Exception::Exception")
        );
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
            if (isBadName(name))
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

