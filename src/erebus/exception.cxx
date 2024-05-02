#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>

namespace Er
{

namespace ExceptionProps
{

namespace Private
{

void registerAll(Er::Log::ILog* log)
{
    registerProperty(std::make_shared<PropertyInfoWrapper<ResultCode>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<DecodedError>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<PosixErrorCode>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<Win32ErrorCode>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<FailedAssertion>>(), log);
}

void unregisterAll(Er::Log::ILog* log)
{
    unregisterProperty(lookupProperty(ResultCode::Id::value), log);
    unregisterProperty(lookupProperty(DecodedError::Id::value), log);
    unregisterProperty(lookupProperty(PosixErrorCode::Id::value), log);
    unregisterProperty(lookupProperty(Win32ErrorCode::Id::value), log);
    unregisterProperty(lookupProperty(FailedAssertion::Id::value), log);
}

} // namespace Private {}

} // namespace ExceptionProps {}

} // namespace Er {}
