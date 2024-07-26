#pragma once

#include <gtest/gtest.h>


#include <erebus/erebus.hxx>
#include <erebus/knownprops.hxx>
#include <erebus/syncstream.hxx>

#include <iostream>

extern Er::Log::ILog* g_log;



namespace TestProps
{

using BoolProp = Er::PropertyValue<bool, ER_PROPID("test_bool"), "Bool", Er::PropertyComparator<bool>, Er::PropertyFormatter<bool>>;
using Int32Prop = Er::PropertyValue<int32_t, ER_PROPID("test_int32"), "Int32", Er::PropertyComparator<int32_t>, Er::PropertyFormatter<int32_t>>;
using UInt32Prop = Er::PropertyValue<uint32_t, ER_PROPID("test_uint32"), "UInt32", Er::PropertyComparator<uint32_t>, Er::PropertyFormatter<uint32_t>>;
using Int64Prop = Er::PropertyValue<int64_t, ER_PROPID("test_int64"), "Int64", Er::PropertyComparator<int64_t>, Er::PropertyFormatter<int64_t>>;
using UInt64Prop = Er::PropertyValue<uint64_t, ER_PROPID("test_uint64"), "UInt64", Er::PropertyComparator<uint64_t>, Er::PropertyFormatter<uint64_t>>;
using DoubleProp = Er::PropertyValue<double, ER_PROPID("test_double"), "Double", Er::PropertyComparator<double>, Er::PropertyFormatter<double>>;
using StringProp = Er::PropertyValue<std::string, ER_PROPID("test_string"), "String", Er::PropertyComparator<std::string>, Er::PropertyFormatter<std::string>>;
using BytesProp = Er::PropertyValue < Er::Bytes, ER_PROPID("test_bytes"), "Bytes", Er::PropertyComparator<Er::Bytes>, Er::NullPropertyFormatter> ;


inline void registerAll(Er::Log::ILog* log)
{
    Er::registerProperty(std::make_shared<Er::PropertyInfoWrapper<BoolProp>>(), log);
    Er::registerProperty(std::make_shared<Er::PropertyInfoWrapper<Int32Prop>>(), log);
    Er::registerProperty(std::make_shared<Er::PropertyInfoWrapper<UInt32Prop>>(), log);
    Er::registerProperty(std::make_shared<Er::PropertyInfoWrapper<Int64Prop>>(), log);
    Er::registerProperty(std::make_shared<Er::PropertyInfoWrapper<UInt64Prop>>(), log);
    Er::registerProperty(std::make_shared<Er::PropertyInfoWrapper<DoubleProp>>(), log);
    Er::registerProperty(std::make_shared<Er::PropertyInfoWrapper<StringProp>>(), log);
    Er::registerProperty(std::make_shared<Er::PropertyInfoWrapper<BytesProp>>(), log);
}

inline void unregisterAll(Er::Log::ILog* log)
{
    Er::unregisterProperty(Er::lookupProperty(BoolProp::Id::value), log);
    Er::unregisterProperty(Er::lookupProperty(Int32Prop::Id::value), log);
    Er::unregisterProperty(Er::lookupProperty(UInt32Prop::Id::value), log);
    Er::unregisterProperty(Er::lookupProperty(Int64Prop::Id::value), log);
    Er::unregisterProperty(Er::lookupProperty(UInt64Prop::Id::value), log);
    Er::unregisterProperty(Er::lookupProperty(DoubleProp::Id::value), log);
    Er::unregisterProperty(Er::lookupProperty(StringProp::Id::value), log);
    Er::unregisterProperty(Er::lookupProperty(BytesProp::Id::value), log);
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


