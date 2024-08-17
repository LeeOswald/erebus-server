#pragma once


#include <erebus/exception.hxx>
#include <erebus/property.hxx>
#include <erebus/knownprops.hxx>
#include <erebus/util/format.hxx>

#include <erebus/erebus.pb.h>



namespace Er
{
    
namespace Protocol
{
    
namespace Private
{

inline void assignPropertyImpl(erebus::Property& out, const Empty& val)
{
}

inline void assignPropertyImpl(erebus::Property& out, bool val)
{
    out.set_v_bool(val);
}

inline void assignPropertyImpl(erebus::Property& out, int32_t val)
{
    out.set_v_int32(val);
}

inline void assignPropertyImpl(erebus::Property& out, uint32_t val)
{
    out.set_v_uint32(val);
}

inline void assignPropertyImpl(erebus::Property& out, int64_t val)
{
    out.set_v_int64(val);
}

inline void assignPropertyImpl(erebus::Property& out, uint64_t val)
{
    out.set_v_uint64(val);
}

inline void assignPropertyImpl(erebus::Property& out, double val)
{
    out.set_v_double(val);
}

inline void assignPropertyImpl(erebus::Property& out, const std::string& val)
{
    out.set_v_string(val);
}

inline void assignPropertyImpl(erebus::Property& out, const Bytes& val)
{
    out.set_v_bytes(val.bytes());
}

} // namespace Private {}


inline void assignProperty(erebus::Property& out, const Property& source)
{
    out.set_id(source.id);

    if (source.value.valueless_by_exception()) [[unlikely]]
    {
        out.clear_value();
    }
    else
    {
        std::visit(
            [&out](auto&& arg)
            {
                Private::assignPropertyImpl(out, arg);
            },
            source.value
        );
    }
}

inline Property getProperty(const erebus::Property& source)
{
    auto id = source.id();
    if (source.has_v_bool())
        return Property(PropId(id), source.v_bool());
    else if (source.has_v_int32())
        return Property(PropId(id), source.v_int32());
    else if (source.has_v_uint32())
        return Property(PropId(id), source.v_uint32());
    else if (source.has_v_int64())
        return Property(PropId(id), source.v_int64());
    else if (source.has_v_uint64())
        return Property(PropId(id), source.v_uint64());
    else if (source.has_v_double())
        return Property(PropId(id), source.v_double());
    else if (source.has_v_string())
        return Property(PropId(id), source.v_string());
    else if (source.has_v_bytes())
        return Property(PropId(id), Er::Bytes(source.v_bytes()));
    else
        ErThrow(Er::Util::format("Unsupported property %08x", id));
}

namespace Props
{

using RemoteSystemDesc = PropertyValue<std::string, ER_PROPID("erebus_remote_sys"), "Remote system">;
using ServerVersionString = PropertyValue<std::string, ER_PROPID("erebus_server_version"), "Erebus server version">;

namespace Private
{

inline void registerAll(Er::Log::ILog* log)
{
    registerProperty(std::make_shared<PropertyInfoWrapper<RemoteSystemDesc>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<ServerVersionString>>(), log);
}

inline void unregisterAll(Er::Log::ILog* log)
{
    unregisterProperty(lookupProperty(RemoteSystemDesc::Id::value), log);
    unregisterProperty(lookupProperty(ServerVersionString::Id::value), log);
}

} // namespace Private {}

} // namespace Props {}

namespace GenericRequests
{

static const std::string_view GetVersion = "GetVersion";

} // namespace GenericRequests {}


} // namespace Protocol {}
    
} // namespace Er {}