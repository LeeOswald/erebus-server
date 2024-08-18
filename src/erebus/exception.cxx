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
    registerProperty(ResultCode::make_info(), log);
    registerProperty(DecodedError::make_info(), log);
    registerProperty(FileName::make_info(), log);
    registerProperty(DirectoryName::make_info(), log);
    registerProperty(PosixErrorCode::make_info(), log);
    registerProperty(Win32ErrorCode::make_info(), log);
}

void unregisterAll(Er::Log::ILog* log)
{
    unregisterProperty(lookupProperty(ResultCode::Id::value), log);
    unregisterProperty(lookupProperty(DecodedError::Id::value), log);
    unregisterProperty(lookupProperty(FileName::Id::value), log);
    unregisterProperty(lookupProperty(DirectoryName::Id::value), log);
    unregisterProperty(lookupProperty(PosixErrorCode::Id::value), log);
    unregisterProperty(lookupProperty(Win32ErrorCode::Id::value), log);
}

} // namespace Private {}

} // namespace ExceptionProps {}

} // namespace Er {}
