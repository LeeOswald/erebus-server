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
    void operator()(const Binary& ico, std::ostream& s) 
    { 
        if (ico.empty())
            s << "[null icon]";
        else
            s << "[icon (" << ico.size() << " bytes)]";
    }
};    


namespace Props
{

constexpr const std::string_view Domain = "app";

using Icon = PropertyValue<Binary, ER_PROPID("icon_png"), "Icon Bytes", IconFormatter>;
using IconName = PropertyValue<std::string, ER_PROPID("icon_name"), "Icon Name">;
using IconSize = PropertyValue<uint32_t, ER_PROPID("icon_size"), "Icon Size">;
using IconState = PropertyValue<uint32_t, ER_PROPID("icon_state"), "Icon State", IconStateFormatter>;
using Pid = PropertyValue<uint64_t, ER_PROPID("pid"), "PID">;


namespace Private
{

inline void registerAll(Er::Log::ILog* log)
{
    registerProperty(Domain, Icon::make_info(), log);
    registerProperty(Domain, IconName::make_info(), log);
    registerProperty(Domain, IconSize::make_info(), log);
    registerProperty(Domain, IconState::make_info(), log);
    registerProperty(Domain, Pid::make_info(), log);
}

inline void unregisterAll(Er::Log::ILog* log)
{
    unregisterProperty(Domain, lookupProperty(Domain, Icon::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, IconName::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, IconSize::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, IconState::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, Pid::Id::value), log);
}


} // namespace Private {}

} // namespace Props {}


namespace Requests
{

static const std::string_view QueryIcon = "QueryIcon";


} // namespace Requests {}

} // namespace Desktop {}

} // namespace Er {}
