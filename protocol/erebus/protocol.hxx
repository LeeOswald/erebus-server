#pragma once

#include <erebus/erebus.hxx>
#include <erebus/property.hxx>

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

    std::visit(
        [&out](auto&& arg)
        {
            Private::assignPropertyImpl(out, arg);
        }, 
        source.value
    );
}

} // namespace Protocol {}
    
} // namespace Er {}