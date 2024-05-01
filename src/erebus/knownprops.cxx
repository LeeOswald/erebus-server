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
    std::lock_guard l(s_registry->mutex);

    auto ret1 = s_registry->propsById.insert({ pi->id(), pi });
    if (!ret1.second)
    {
        throw Exception(ER_HERE(), Util::format("Property with ID %08x (%s) already registered", pi->id(), pi->id_str()));
    }

    auto ret2 = s_registry->propsByName.insert({ pi->id_str(), pi });
    if (!ret2.second)
    {
        throw Exception(ER_HERE(), Util::format("Property with ID %08x (%s) already registered", pi->id(), pi->id_str()));
    }

    LogDebug(log, LogNowhere(), "Registered property %08x (%s)", pi->id(), pi->id_str());
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

    auto it2 = s_registry->propsByName.find(pi->id_str());
    if (it2 != s_registry->propsByName.end())
    {
        s_registry->propsByName.erase(it2);
    }

    LogDebug(log, LogNowhere(), "Unregistered property %08x (%s)", pi->id(), pi->id_str());
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


#if ER_DEBUG
void Property::checkProperty()
{
    auto info_ = info ? info.get() : Er::lookupProperty(id).get();
    ErAssert(info_);
    auto& type_ = info_->type_info();
    if (type_ == typeid(bool))
        ErAssert(type == PropertyType::Bool);
    else if (type_ == typeid(int32_t))
        ErAssert(type == PropertyType::Int32);
    else if (type_ == typeid(uint32_t))
        ErAssert(type == PropertyType::UInt32);
    else if (type_ == typeid(int64_t))
        ErAssert(type == PropertyType::Int64);
    else if (type_ == typeid(uint64_t))
        ErAssert(type == PropertyType::UInt64);
    else if (type_ == typeid(double))
        ErAssert(type == PropertyType::Double);
    else if (type_ == typeid(std::string))
        ErAssert(type == PropertyType::String);
    else if (type_ == typeid(Bytes))
        ErAssert(type == PropertyType::Bytes);
    else
        ErAssert(!"Mismatched property type");
}
#endif

} // namespace Er {}
