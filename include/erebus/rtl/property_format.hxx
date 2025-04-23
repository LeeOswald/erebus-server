#pragma once


#include <functional>


namespace Er
{

class Property;

using SemanticCode = std::uint32_t;


namespace Semantics
{

enum : SemanticCode
{
    Default = 0,
    Hex,
    Address,
    Scientific,
    Fixed,
    Fixed3,              // precision = 3
    UtcTime,             // PackedTime::ValueType formatted as UTC time 
    UtcDate,             // PackedTime::ValueType formatted as UTC date
    UtcDateTime,         // PackedTime::ValueType formatted as UTC date & time
    LocalTime,
    LocalDate,
    LocalDateTime,
    Microseconds,        // PackedTime::ValueType formatted as microseconds
    Milliseconds,
    Seconds,
    Percent,
};

} // namespace Semantics {}


using PropertyFormatter = std::function<std::string(const Property&)>;


[[nodiscard]] ER_RTL_EXPORT PropertyFormatter& findPropertyFormatter(SemanticCode code);
ER_RTL_EXPORT void registerPropertyFormatter(SemanticCode code, PropertyFormatter&& f);




} // namespace Er {}