#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>
#include <erebus/util/format.hxx>


#include <shared_mutex>
#include <unordered_map>

namespace Er
{

namespace
{

struct Registry
{
    struct Entry
    {
        IPropertyInfo::Ptr ptr;
        long ref = 1;

        Entry(IPropertyInfo::Ptr ptr) : ptr(ptr) {}
    };

    std::shared_mutex mutex;
    std::unordered_map<PropId, Entry> propsById;
    std::unordered_map<std::string, Entry> propsByName;
};
    
Registry* s_registry = nullptr;


} // namespace {}

namespace Private
{

EREBUS_EXPORT void initializeKnownProps()
{
    ErAssert(!s_registry);
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
    std::unique_lock l(s_registry->mutex);

    int success = 0;

    auto ret1 = s_registry->propsById.insert({ pi->id(), pi });
    if (!ret1.second)
    {
        if (std::strcmp(ret1.first->second.ptr->id_str(), pi->id_str()) != 0)
            throw Exception(ER_HERE(), 
                Util::format("Existing property %08x (%s) prevents %s from being registered", pi->id(), ret1.first->second.ptr->id_str(), pi->id_str()));

        ret1.first->second.ref += 1;
    }
    else
    {
        ++success;
    }

    auto ret2 = s_registry->propsByName.insert({ pi->id_str(), pi });
    if (!ret2.second)
    {
        if (ret1.first->second.ptr->id() != pi->id())
            throw Exception(ER_HERE(), 
                Util::format("Existing property %s (%08x) prevents %08x from being registered", ret1.first->second.ptr->id_str(), ret1.first->second.ptr->id(), pi->id()));

        ret1.first->second.ref += 1;
    }
    else
    {
        ++success;
    }

    if (success > 0)
        ErLogDebug(log, ErLogNowhere(), "Registered property %08x (%s)", pi->id(), pi->id_str());
}

EREBUS_EXPORT void unregisterProperty(IPropertyInfo::Ptr pi, Er::Log::ILog* log) noexcept
{
    if (!pi)
        return;
        
    std::unique_lock l(s_registry->mutex);

    int success = 0;

    auto it1 = s_registry->propsById.find(pi->id());
    if (it1 != s_registry->propsById.end())
    {
        if (!--it1->second.ref)
        {
            s_registry->propsById.erase(it1);
            ++success;
        }
    }

    auto it2 = s_registry->propsByName.find(pi->id_str());
    if (it2 != s_registry->propsByName.end())
    {
        if (!--it2->second.ref)
        {
            s_registry->propsByName.erase(it2);
            ++success;
        }
    }

    if (success > 0)
        ErLogDebug(log, ErLogNowhere(), "Unregistered property %08x (%s)", pi->id(), pi->id_str());
}

EREBUS_EXPORT IPropertyInfo::Ptr lookupProperty(PropId id) noexcept
{
    std::shared_lock l(s_registry->mutex);

    auto it = s_registry->propsById.find(id);
    if (it == s_registry->propsById.end())
        return IPropertyInfo::Ptr();

    return it->second.ptr;
}

EREBUS_EXPORT IPropertyInfo::Ptr lookupProperty(const char* id) noexcept
{
    std::shared_lock l(s_registry->mutex);

    auto it = s_registry->propsByName.find(id);
    if (it == s_registry->propsByName.end())
        return IPropertyInfo::Ptr();

    return it->second.ptr;
}


} // namespace Er {}
