#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>

namespace Er
{

EREBUS_EXPORT void initialize()
{
    Er::Private::initializeKnownProps();

    Er::ExceptionProps::Private::registerAll();
}

EREBUS_EXPORT void finalize()
{
    Er::Private::finalizeKnownProps();
}

} // namespace Er {}
