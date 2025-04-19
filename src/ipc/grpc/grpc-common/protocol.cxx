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

} // namespace Er::Ipc::Grpc {}