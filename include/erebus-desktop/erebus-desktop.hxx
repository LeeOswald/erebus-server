#pragma once


#include <erebus/knownprops.hxx>
#include <erebus/log.hxx>


#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef EREBUSDESKTOP_EXPORTS
        #define EREBUSDESKTOP_EXPORT __declspec(dllexport)
    #else
        #define EREBUSDESKTOP_EXPORT __declspec(dllimport)
    #endif
#else
    #define EREBUSDESKTOP_EXPORT __attribute__((visibility("default")))
#endif


namespace Er
{

namespace Desktop
{

enum class IconSize : uint32_t
{
    Small = 16,
    large = 32
};

enum class IconState: uint32_t
{
    Pending,
    NotPresent,
    Found
};

struct IconStateFormatter
{
    void operator()(uint32_t v, std::ostream& s)
    {
        switch (static_cast<IconState>(v))
        {
            case IconState::Pending: s << "Pending"; break;
            case IconState::NotPresent: s << "Not Present"; break;
            case IconState::Found: s << "Found"; break;
            default: s << "???"; break;
        }
    }
};

struct IconFormatter
{
    void operator()(const Bytes& ico, std::ostream& s) 
    { 
        if (ico.empty())
            s << "[null icon]";
        else
            s << "[icon (" << ico.size() << " bytes)]";
    }
};    


namespace Props
{

using Icon = PropertyValue<Bytes, ER_PROPID("app.icon.png"), "Icon Bytes", PropertyComparator<Bytes>, IconFormatter>;
using IconName = PropertyValue<std::string, ER_PROPID("app.icon.name"), "Icon Name">;
using IconSize = PropertyValue<uint32_t, ER_PROPID("app.icon.size"), "Icon Size">;
using IconState = PropertyValue<uint32_t, ER_PROPID("app.icon.state"), "Icon State", PropertyComparator<uint32_t>, IconStateFormatter>;
using Pid = PropertyValue<uint64_t, ER_PROPID("app.pid"), "PID">;


namespace Private
{

inline void registerAll(Er::Log::ILog* log)
{
    registerProperty(std::make_shared<PropertyInfoWrapper<Icon>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<IconName>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<IconSize>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<IconState>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<Pid>>(), log);
}

inline void unregisterAll(Er::Log::ILog* log)
{
    unregisterProperty(lookupProperty(Icon::Id::value), log);
    unregisterProperty(lookupProperty(IconName::Id::value), log);
    unregisterProperty(lookupProperty(IconSize::Id::value), log);
    unregisterProperty(lookupProperty(IconState::Id::value), log);
    unregisterProperty(lookupProperty(Pid::Id::value), log);
}


} // namespace Private {}

} // namespace Props {}


namespace Requests
{

static const std::string_view QueryIcon = "QueryIcon";


} // namespace Requests {}

} // namespace Desktop {}

} // namespace Er {}
