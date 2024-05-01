#include <erebus/exception.hxx>




namespace Er
{

namespace Private
{

EREBUS_EXPORT void failAssert(Location&& location, const char* expression)
{
    throw Er::Exception(std::move(location), std::string_view("ASSERTION FAILED"), Er::ExceptionProps::FailedAssertion(expression));
}

} // namespace Private {}

} // namespace Er {}