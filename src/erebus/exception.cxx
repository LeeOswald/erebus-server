#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>

namespace Er
{

namespace ExceptionProps
{

namespace Private
{

void registerAll()
{
    registerProperty(std::make_shared<PropertyInfoWrapper<DecodedError>>());
    registerProperty(std::make_shared<PropertyInfoWrapper<PosixErrorCode>>());
    registerProperty(std::make_shared<PropertyInfoWrapper<Win32ErrorCode>>());
}

} // namespace Private {}

} // namespace ExceptionProps {}

} // namespace Er {}
