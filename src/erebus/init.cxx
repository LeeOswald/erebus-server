#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>

#include <atomic>

namespace Er
{

static std::atomic<long> g_initialized = 0;

EREBUS_EXPORT void initialize()
{
    if (g_initialized.fetch_add(1, std::memory_order_acq_rel) == 0)
    {
        Er::Private::initializeKnownProps();

        Er::ExceptionProps::Private::registerAll();
    }
}

EREBUS_EXPORT void finalize()
{
    if (g_initialized.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        Er::Private::finalizeKnownProps();
    }
}

} // namespace Er {}
