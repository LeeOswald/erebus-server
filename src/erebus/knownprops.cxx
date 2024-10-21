#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>

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
        std::string domain;
        IPropertyInfo::Ptr ptr;
        long ref = 1;

        Entry(std::string_view domain, IPropertyInfo::Ptr ptr) 
            : domain(domain)
            , ptr(ptr)
        {}
    };

    std::shared_mutex mutex;
    std::unordered_multimap<PropId, Entry> propsById;
    std::unordered_multimap<std::string, Entry> propsByName;
};
    
Registry* s_registry = nullptr;


auto _lookupProperty(std::string_view domain, IPropertyInfo::Ptr pi) noexcept
{
    decltype (s_registry->propsById)::iterator it1 = s_registry->propsById.end();
    auto r1 = s_registry->propsById.equal_range(pi->id());
    for (auto it = r1.first; it != r1.second; ++it)
    {
        if (it->second.domain == domain)
        {
            it1 = it;
        }
    }

    decltype (s_registry->propsByName)::iterator it2 = s_registry->propsByName.end();
    auto r2 = s_registry->propsByName.equal_range(pi->id_str());
    for (auto it = r2.first; it != r2.second; ++it)
    {
        if (it->second.domain == domain)
        {
            it2 = it;
        }
    }

    return std::pair(it1, it2);
}


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



EREBUS_EXPORT void registerProperty(std::string_view domain, IPropertyInfo::Ptr pi, Er::Log::ILog* log)
{
    std::unique_lock l(s_registry->mutex);

    int success = 0;

    auto existing = _lookupProperty(domain, pi);
    if (existing.first == s_registry->propsById.end())
    {
        s_registry->propsById.insert({ pi->id(), { domain, pi } });
        ++success;
    }
    else
    {
        auto& entry = existing.first->second;
        if (std::strcmp(entry.ptr->id_str(), pi->id_str()) != 0)
        {
            ErThrow(Er::format(
                    "Trying to register property {}:{:08x} [{}] while [{}] exists with the same ID", 
                    std::string(domain).c_str(), 
                    pi->id(), 
                    pi->id_str(), 
                    entry.ptr->id_str()
            ));
        }

        entry.ref += 1;
    }

    if (existing.second == s_registry->propsByName.end())
    {
        s_registry->propsByName.insert({ pi->id_str(), { domain, pi } });
        ++success;
    }
    else
    {
        auto& entry = existing.first->second;
        if (entry.ptr->id() != pi->id())
        {
            ErThrow(Er::format(
                    "Trying to register property {}:{:08x} [{}] while {:08x} exists with the same name", 
                    std::string(domain).c_str(), 
                    pi->id(), 
                    pi->id_str(), 
                    entry.ptr->id()
            ));
        }

        entry.ref += 1;
    }

    if (success > 0)
        Er::Log::Debug(log) << "Registered property " << std::hex << std::setw(8) << std::setfill('0') << pi->id() << " (" << domain << "." << pi->id_str() << ")";
}

EREBUS_EXPORT void unregisterProperty(std::string_view domain, IPropertyInfo::Ptr pi, Er::Log::ILog* log) noexcept
{
    if (!pi)
        return;
        
    std::unique_lock l(s_registry->mutex);

    int success = 0;

    auto existing = _lookupProperty(domain, pi);
    if (existing.first != s_registry->propsById.end())
    {
        s_registry->propsById.erase(existing.first);
        ++success;
    }

    if (existing.second != s_registry->propsByName.end())
    {
        s_registry->propsByName.erase(existing.second);
        ++success;
    }

    if (success > 0)
        Er::Log::Debug(log) << "Unregistered property " << std::hex << std::setw(8) << std::setfill('0') << pi->id() << " (" << domain << "." << pi->id_str() << ")";
}

EREBUS_EXPORT IPropertyInfo::Ptr lookupProperty(std::string_view domain, PropId id) noexcept
{
    std::shared_lock l(s_registry->mutex);

    auto r = s_registry->propsById.equal_range(id);
    for (auto it = r.first; it != r.second; ++it)
    {
        if (it->second.domain == domain)
        {
            return it->second.ptr;
        }
    }

    return IPropertyInfo::Ptr();
}

EREBUS_EXPORT IPropertyInfo::Ptr lookupProperty(std::string_view domain, const char* id) noexcept
{
    std::shared_lock l(s_registry->mutex);

    auto r = s_registry->propsByName.equal_range(id);
    for (auto it = r.first; it != r.second; ++it)
    {
        if (it->second.domain == domain)
        {
            return it->second.ptr;
        }
    }

    return IPropertyInfo::Ptr();
}


} // namespace Er {}
