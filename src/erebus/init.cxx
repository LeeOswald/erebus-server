#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>

namespace Er
{

EREBUS_EXPORT void initialize()
{
    Er::ExceptionProps::Private::registerAll();
}

EREBUS_EXPORT void finalize()
{

}

} // namespace Er {}
