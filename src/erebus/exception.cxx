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
    registerProperty(new PropertyInfoWrapper<DecodedError>);
    registerProperty(new PropertyInfoWrapper<PosixErrorCode>);
    registerProperty(new PropertyInfoWrapper<Win32ErrorCode>);
}

} // namespace Private {}

} // namespace ExceptionProps {}

} // namespace Er {}
