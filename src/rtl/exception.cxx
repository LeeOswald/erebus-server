#include <erebus/rtl/util/exception_util.hxx>

namespace Er
{


ER_RTL_EXPORT [[nodiscard]] Exception makeExceptionFromResult(std::source_location location, std::string&& message, ResultCode code)
{
    Exception x(location, std::move(message));

    x.add(Property(ExceptionProps::ResultCode, std::int32_t(code)));

    auto decoded = resultToString(code);
    if (!decoded.empty())
        x.add(Property(ExceptionProps::DecodedError, std::move(decoded)));

    return x;
}

} // namespace Er {}