#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>
#include <erebus/util/format.hxx>


#include <mutex>
#include <unordered_map>

namespace Er
{

namespace
{

struct Registry
{
    std::mutex mutex;
    std::unordered_map<PropId, IPropertyInfo::Ptr> propsById;
    std::unordered_map<std::string, IPropertyInfo::Ptr> propsByName;
};
    
Registry* s_registry = nullptr;


} // namespace {}

namespace Private
{

EREBUS_EXPORT void initializeKnownProps()
{
    assert(!s_registry);
    s_registry = new Registry;
}

EREBUS_EXPORT void finalizeKnownProps()
{
    if (s_registry)
    {
        auto p = s_registry;
        s_registry = nullptr;
        delete p;
    }
}

} // namespace Private {}



EREBUS_EXPORT void registerProperty(IPropertyInfo::Ptr pi, Er::Log::ILog* log)
{
    std::lock_guard l(s_registry->mutex);

    auto ret1 = s_registry->propsById.insert({ pi->id(), pi });
    if (!ret1.second)
    {
        throw Exception(ER_HERE(), Util::format("Property with ID %08x (%s) already registered", pi->id(), pi->idstr()));
    }

    auto ret2 = s_registry->propsByName.insert({ pi->idstr(), pi });
    if (!ret2.second)
    {
        throw Exception(ER_HERE(), Util::format("Property with ID %08x (%s) already registered", pi->id(), pi->idstr()));
    }

    LogDebug(log, LogNowhere(), "Registered property %08x (%s)", pi->id(), pi->idstr());
}

EREBUS_EXPORT void unregisterProperty(IPropertyInfo::Ptr pi, Er::Log::ILog* log) noexcept
{
    if (!pi)
        return;
        
    std::lock_guard l(s_registry->mutex);

    auto it1 = s_registry->propsById.find(pi->id());
    if (it1 != s_registry->propsById.end())
    {
        s_registry->propsById.erase(it1);
    }

    auto it2 = s_registry->propsByName.find(pi->idstr());
    if (it2 != s_registry->propsByName.end())
    {
        s_registry->propsByName.erase(it2);
    }

    LogDebug(log, LogNowhere(), "Unregistered property %08x (%s)", pi->id(), pi->idstr());
}

EREBUS_EXPORT IPropertyInfo::Ptr lookupProperty(PropId id) noexcept
{
    std::lock_guard l(s_registry->mutex);

    auto it = s_registry->propsById.find(id);
    if (it == s_registry->propsById.end())
        return IPropertyInfo::Ptr();

    return it->second;
}

EREBUS_EXPORT IPropertyInfo::Ptr lookupProperty(const char* id) noexcept
{
    std::lock_guard l(s_registry->mutex);

    auto it = s_registry->propsByName.find(id);
    if (it == s_registry->propsByName.end())
        return IPropertyInfo::Ptr();

    return it->second;
}

} // namespace Er {}
