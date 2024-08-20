#pragma once

#include <gtest/gtest.h>


#include <erebus/erebus.hxx>
#include <erebus/knownprops.hxx>
#include <erebus/syncstream.hxx>

#include <iostream>

extern Er::Log::ILog* g_log;



namespace TestProps
{

constexpr const std::string_view Domain = "Test";

using BoolProp = Er::PropertyValue<bool, ER_PROPID("test_bool"), "Bool", Er::PropertyFormatter<bool>>;
using Int32Prop = Er::PropertyValue<int32_t, ER_PROPID("test_int32"), "Int32", Er::PropertyFormatter<int32_t>>;
using UInt32Prop = Er::PropertyValue<uint32_t, ER_PROPID("test_uint32"), "UInt32", Er::PropertyFormatter<uint32_t>>;
using Int64Prop = Er::PropertyValue<int64_t, ER_PROPID("test_int64"), "Int64", Er::PropertyFormatter<int64_t>>;
using UInt64Prop = Er::PropertyValue<uint64_t, ER_PROPID("test_uint64"), "UInt64", Er::PropertyFormatter<uint64_t>>;
using DoubleProp = Er::PropertyValue<double, ER_PROPID("test_double"), "Double", Er::PropertyFormatter<double>>;
using StringProp = Er::PropertyValue<std::string, ER_PROPID("test_string"), "String", Er::PropertyFormatter<std::string>>;
using BytesProp = Er::PropertyValue < Er::Bytes, ER_PROPID("test_bytes"), "Bytes", Er::NullPropertyFormatter> ;


inline void registerAll(Er::Log::ILog* log)
{
    Er::registerProperty(Domain, BoolProp::make_info(), log);
    Er::registerProperty(Domain, Int32Prop::make_info(), log);
    Er::registerProperty(Domain, UInt32Prop::make_info(), log);
    Er::registerProperty(Domain, Int64Prop::make_info(), log);
    Er::registerProperty(Domain, UInt64Prop::make_info(), log);
    Er::registerProperty(Domain, DoubleProp::make_info(), log);
    Er::registerProperty(Domain, StringProp::make_info(), log);
    Er::registerProperty(Domain, BytesProp::make_info(), log);
}

inline void unregisterAll(Er::Log::ILog* log)
{
    Er::unregisterProperty(Domain, Er::lookupProperty(Domain, BoolProp::Id::value), log);
    Er::unregisterProperty(Domain, Er::lookupProperty(Domain, Int32Prop::Id::value), log);
    Er::unregisterProperty(Domain, Er::lookupProperty(Domain, UInt32Prop::Id::value), log);
    Er::unregisterProperty(Domain, Er::lookupProperty(Domain, Int64Prop::Id::value), log);
    Er::unregisterProperty(Domain, Er::lookupProperty(Domain, UInt64Prop::Id::value), log);
    Er::unregisterProperty(Domain, Er::lookupProperty(Domain, DoubleProp::Id::value), log);
    Er::unregisterProperty(Domain, Er::lookupProperty(Domain, StringProp::Id::value), log);
    Er::unregisterProperty(Domain, Er::lookupProperty(Domain, BytesProp::Id::value), log);
}


} // TestProps {}


struct InstanceCounter 
{
    static int instances;

    InstanceCounter() 
    { 
        ++instances; 
    }
    
    InstanceCounter(const InstanceCounter&) 
    { 
        ++instances; 
    }
    
    InstanceCounter& operator=(const InstanceCounter&) 
    { 
        ++instances; 
        return *this;
    }
    
    ~InstanceCounter() 
    { 
        --instances; 
    }
};


