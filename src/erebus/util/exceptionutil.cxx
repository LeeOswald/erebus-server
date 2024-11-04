#include <erebus/knownprops.hxx>
#include <erebus/util/exceptionutil.hxx>

#include <sstream>

namespace Er
{

namespace Util
{

namespace
{

constexpr size_t kIndentSize = 4;

void formatException(const Er::Exception& e, std::ostringstream& out, int level);


void formatException(const std::exception& e, std::ostringstream& out, int level)
{
    std::string indent(level * kIndentSize, ' ');

    if (level > 0)
    {
        if (level > 1)
        {
            std::string indent0((level - 1) * kIndentSize, ' ');
            out << '\n' << indent0 << "------------------------------------------------\n";
        }
        else
        {
            out << '\n' << indent << "------------------------------------------------\n";
        }
    }

    auto message = e.what();
    if (!message || !*message)
        out << indent << "Unknown exception";
    else
        out << indent << e.what();

    try
    {
        std::rethrow_if_nested(e);
    }
    catch (Er::Exception& nested)
    {
        formatException(nested, out, level + 1);
    }
    catch (std::exception& nested)
    {
        formatException(nested, out, level + 1);
    }
    catch (...)
    {
    }
}

void formatException(const Er::Exception& e, std::ostringstream& out, int level)
{
    std::string indent(level * kIndentSize, ' ');
    std::string smallIndent(kIndentSize / 2, ' ');

    if (level > 0)
    {
        if (level > 1)
        {
            std::string indent0((level - 1) * kIndentSize, ' ');
            out << '\n' << indent0 << "------------------------------------------------\n";
        }
        else
        {
            out << '\n' << indent << "------------------------------------------------\n";
        }
    }

    auto message = e.message();
    if (message)
    {
        out << indent << *message;
    }
    else
    {
        out << indent << e.what();
    }

    auto properties = e.properties();
    if (properties)
    {
        for (auto& prop: *properties)
        {
            out << "\n" << indent << smallIndent;

            auto pi = lookupProperty(Er::ExceptionProps::Domain, prop.id);
            if (pi)
            {
                out << pi->name() << ": " << pi->to_string(prop);
            }
            else
            {
                out << "0x" << std::hex << ": " << prop.to_string();
            }
        }
    }

    auto location = e.location();
    if (location)
    {
        if (location->source)
        {
            out << "\n" << indent << smallIndent << "File: " << location->source->file_name();
            out << "\n" << indent << smallIndent << "Line: " << location->source->line();
            out << "\n" << indent << smallIndent << "Function: " << location->source->function_name();
        }

        DecodedStackTrace ds;
        const DecodedStackTrace* stack = nullptr;
        if (location->decoded)
        {
            stack = &(*location->decoded);
        }
        else if (location->stack)
        {
            ds = decodeStackTrace(*location->stack);
            stack = &ds;
        }

        if (stack && !stack->empty())
        {
            out << "\n" << indent << smallIndent << "Call Stack:";
            for (auto& frame : *stack)
            {
                out << "\n" << indent << smallIndent << smallIndent;
                if (!frame.empty())
                    out << frame;
                else
                    out << "???";
            }
        }
    }

    try
    {
        std::rethrow_if_nested(e);
    }
    catch (Er::Exception& nested)
    {
        formatException(nested, out, level + 1);
    }
    catch (std::exception& nested)
    {
        formatException(nested, out, level + 1);
    }
    catch (...)
    {
    }
}

} // namespace {}


EREBUS_EXPORT std::string formatException(const std::exception& e) noexcept
{
    try
    {
        std::ostringstream out;

        formatException(e, out, 0);

        return out.str();
    }
    catch (...)
    {
    }

    return std::string();
}

EREBUS_EXPORT std::string formatException(const Er::Exception& e) noexcept
{
    try
    {
        std::ostringstream out;

        formatException(e, out, 0);

        return out.str();
    }
    catch (...)
    {

    }

    return std::string();
}

EREBUS_EXPORT void logException(Log::ILog* log, Log::Level level, const std::exception& e) noexcept
{
    Log::writeln(log, level, formatException(e));
}

EREBUS_EXPORT void logException(Log::ILog* log, Log::Level level, const Er::Exception& e) noexcept
{
    Log::writeln(log, level, formatException(e));
}

} // namespace Util {}

} // namespace Er {}
