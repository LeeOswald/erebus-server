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
    registerProperty(ExceptionProps::Domain, ResultCode::make_info(), log);
    registerProperty(ExceptionProps::Domain, DecodedError::make_info(), log);
    registerProperty(ExceptionProps::Domain, FileName::make_info(), log);
    registerProperty(ExceptionProps::Domain, DirectoryName::make_info(), log);
    registerProperty(ExceptionProps::Domain, PosixErrorCode::make_info(), log);
    registerProperty(ExceptionProps::Domain, Win32ErrorCode::make_info(), log);
}

void unregisterAll(Er::Log::ILog* log)
{
    unregisterProperty(ExceptionProps::Domain, lookupProperty(Domain, ResultCode::Id::value), log);
    unregisterProperty(ExceptionProps::Domain, lookupProperty(Domain, DecodedError::Id::value), log);
    unregisterProperty(ExceptionProps::Domain, lookupProperty(Domain, FileName::Id::value), log);
    unregisterProperty(ExceptionProps::Domain, lookupProperty(Domain, DirectoryName::Id::value), log);
    unregisterProperty(ExceptionProps::Domain, lookupProperty(Domain, PosixErrorCode::Id::value), log);
    unregisterProperty(ExceptionProps::Domain, lookupProperty(Domain, Win32ErrorCode::Id::value), log);
}

} // namespace Private {}

} // namespace ExceptionProps {}

} // namespace Er {}
