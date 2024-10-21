#include <erebus/util/format.hxx>


namespace Er
{

namespace Util
{

EREBUS_EXPORT std::string cformatv(const char* format, va_list args)
{
    if (!format || !*format)
    {
        return std::string();
    }

    va_list args1;

    va_copy(args1, args);
    auto required = ::vsnprintf(nullptr, 0, format, args1);
    va_end(args1);

    va_list args2;
    va_copy(args2, args);

    std::string buffer;
    buffer.resize(required);

    ::vsnprintf(buffer.data(), required + 1, format, args2);

    va_end(args2);
    return buffer;
}

EREBUS_EXPORT std::string cformat(const char* format, ...)
{
    if (!format || !*format)
    {
        return std::string();
    }

    va_list args;
    va_start(args, format);
    auto buffer = cformatv(format, args);
    va_end(args);

    return buffer;
}


} // namespace Util {}

} // namespace Er {}


