#include <erebus/ipc/grpc/protocol.hxx>


namespace Er::Ipc::Grpc
{

namespace
{

void marshalScalar(const Property& source, erebus::Property_scalar* dest)
{
    auto ty = source.type();
    switch (ty)
    {
    case Er::Property::Type::Bool:
        dest->set_v_bool(*source.getBool());
        break;
    case Er::Property::Type::Int32:
        dest->set_v_int32(*source.getInt32());
        break;
    case Er::Property::Type::UInt32:
        dest->set_v_uint32(*source.getUInt32());
        break;
    case Er::Property::Type::Int64:
        dest->set_v_int64(*source.getInt64());
        break;
    case Er::Property::Type::UInt64:
        dest->set_v_uint64(*source.getUInt64());
        break;
    case Er::Property::Type::Double:
        dest->set_v_double(*source.getDouble());
        break;
    case Er::Property::Type::String:
        dest->set_v_string(*source.getString());
        break;
    case Er::Property::Type::Binary:
        dest->set_v_binary(source.getBinary()->bytes());
        break;
    }
}

Property unmarshalScalar(const erebus::Property& source)
{
    auto& scalar = source.v_scalar();
    if (scalar.has_v_bool())
        return Property(source.name(), scalar.v_bool());
    else if (scalar.has_v_int32())
        return Property(source.name(), scalar.v_int32());
    else if (scalar.has_v_uint32())
        return Property(source.name(), scalar.v_uint32());
    else if (scalar.has_v_int64())
        return Property(source.name(), scalar.v_int64());
    else if (scalar.has_v_uint64())
        return Property(source.name(), scalar.v_uint64());
    else if (scalar.has_v_double())
        return Property(source.name(), scalar.v_double());
    else if (scalar.has_v_string())
        return Property(source.name(), scalar.v_string());
    else if (scalar.has_v_binary())
        return Property(source.name(), Binary(scalar.v_binary()));

    return {};
}

void marshalMap(const Property& source, erebus::Property_object* dest)
{
    auto& sourceMap = *source.getMap();
    auto destMap = dest->mutable_v_map();
    
    for (auto& sourceProp : sourceMap)
    {
        erebus::Property destProp;
        marshalProperty(sourceProp.second, destProp);

        destMap->emplace(std::string(sourceProp.first.data(), sourceProp.first.length()), destProp);
    }
}

Property unmarshalMap(const std::string& name, const erebus::Property_object& source)
{
    PropertyMap destMap;
    
    auto& sourceMap = source.v_map();
    for (auto it = sourceMap.begin(); it != sourceMap.end(); ++it)
    {
        auto prop = unmarshalProperty(it->second);
        addProperty(destMap, std::move(prop));
    }

    return Property(name, std::move(destMap));
}

void marshalVector(const Property& source, erebus::Property_array* dest)
{
    auto& sourceVector = *source.getVector();
    auto destVector = dest->mutable_v_vector();

    for (auto& sourceProp : sourceVector)
    {
        erebus::Property destProp;
        marshalProperty(sourceProp, destProp);

        destVector->Add(std::move(destProp));
    }
}

Property unmarshalVector(const std::string& name, const erebus::Property_array& source)
{
    PropertyVector destVector;

    auto& sourceVector = source.v_vector();
    for (auto it = sourceVector.begin(); it != sourceVector.end(); ++it)
    {
        auto prop = unmarshalProperty(*it);
        addProperty(destVector, std::move(prop));
    }

    return Property(name, std::move(destVector));
}

} // namespace {}


void marshalProperty(const Property& source, erebus::Property& dest)
{
    dest.Clear();

    dest.set_name(source.name().data(), source.name().length());

    auto ty = source.type();
    switch (ty)
    {
    case Er::Property::Type::Empty:
        break;

    case Er::Property::Type::Bool:
    case Er::Property::Type::Int32:
    case Er::Property::Type::UInt32:
    case Er::Property::Type::Int64:
    case Er::Property::Type::UInt64:
    case Er::Property::Type::Double:
    case Er::Property::Type::String:
    case Er::Property::Type::Binary:
        marshalScalar(source, dest.mutable_v_scalar());
        break;

    case Er::Property::Type::Map:
        marshalMap(source, dest.mutable_v_object());
        break;

    case Er::Property::Type::Vector:
        marshalVector(source, dest.mutable_v_array());
        break;
    }
}

Property unmarshalProperty(const erebus::Property& source)
{
    if (source.has_v_scalar())
        return unmarshalScalar(source);
    else if (source.has_v_object())
        return unmarshalMap(source.name(), source.v_object());
    else if (source.has_v_array())
        return unmarshalVector(source.name(), source.v_array());

    return {};
}

void marshalException(const Exception& source, erebus::Exception& dest)
{
    dest.Clear();

    if (source.category()->local())
    {
        // we cannot simply marshal error codes of local error categories
        
        auto msg = source.category()->message(source.code());
        ExceptionProperties::Message prop(std::move(msg));
        
        auto out = dest.add_properties();
        marshalProperty(prop, *out);
    }
    else
    {
        dest.set_code(source.code());
        dest.set_category(std::string(source.category()->name()));
    }

    for (auto& prop : source.properties())
    {
        auto out = dest.add_properties();
        marshalProperty(prop, *out);
    }
}

Exception unmarshalException(const erebus::Exception& source)
{
    Error err;
    if (source.has_category())
    {
        auto cat = lookupErrorCategory(source.category());
        if (cat)
        {
            if (source.has_code())
            {
                err = Error(source.code(), cat);
            }
        }
    }

    if (!err)
        err = Error(Result::Internal, GenericError);

    Exception dest(std::source_location::current(), err);

    int propCount = source.properties_size();
    for (int i = 0; i < propCount; ++i)
    {
        auto prop = unmarshalProperty(source.properties(i));
        dest.add(std::move(prop));
    }

    return dest;
}

} // namespace Er::Ipc::Grpc {}