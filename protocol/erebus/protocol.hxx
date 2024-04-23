#pragma once


#include <erebus/property.hxx>
#include <erebus/util/format.hxx>

#include <erebus/erebus.pb.h>



namespace Er
{
    
namespace Protocol
{
    
namespace Private
{

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
    auto info = Er::lookupProperty(id);
    if (!info) [[unlikely]]
    {
        throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported property 0x%08x", id));
    }
    else
    {
        auto type = info->type();
        if (type == PropertyType::Bool)
            return Property(id, source.v_bool(), info.get());
        else if (type == PropertyType::Int32)
            return Property(id, source.v_int32(), info.get());
        else if (type == PropertyType::UInt32)
            return Property(id, source.v_uint32(), info.get());
        else if (type == PropertyType::Int64)
            return Property(id, source.v_int64(), info.get());
        else if (type == PropertyType::UInt64)
            return Property(id, source.v_uint64(), info.get());
        else if (type == PropertyType::Double)
            return Property(id, source.v_double(), info.get());
        else if (type == PropertyType::String)
            return Property(id, source.v_string(), info.get());
        else if (type == PropertyType::Bytes)
            return Property(id, Bytes(source.v_bytes()), info.get());
        else
            throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported property %s type %s", info->id_str(), info->type_info().name()));
    }
}

} // namespace Protocol {}
    
} // namespace Er {}