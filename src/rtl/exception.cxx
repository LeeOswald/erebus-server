#include <erebus/rtl/exception.hxx>

namespace Er
{

namespace ExceptionProps
{

const PropertyInfo Result{ PropertyType::Int32, "Er.Exception.result", "Result" };
const PropertyInfo DecodedError{ PropertyType::String, "Er.Exception.decoded_error", "Decoded error" };


} // namespace ExceptionProps {}

ER_RTL_EXPORT [[nodiscard]] Exception makeExceptionFromResult(std::source_location location, std::string&& message, ResultCode code)
{
    Exception x(location, std::move(message));

    x.add(Property(std::int32_t(code), ExceptionProps::Result));

    auto decoded = resultToString(code);
    if (!decoded.empty())
        x.add(Property(std::move(decoded), ExceptionProps::DecodedError));

    return x;
}

} // namespace Er {}