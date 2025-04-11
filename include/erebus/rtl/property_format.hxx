#pragma once


#include <functional>


namespace Er
{

class Property;

using SemanticCode = std::uint32_t;


namespace Semantics
{

constexpr SemanticCode Default = 0;
constexpr SemanticCode Hex = 1;
constexpr SemanticCode Address = 2;
constexpr SemanticCode Scientific = 3;
constexpr SemanticCode Fixed = 4;
constexpr SemanticCode Fixed3 = 5;              // precision = 3
constexpr SemanticCode UtcTime = 6;             // PackedTime::ValueType formatted as UTC time 
constexpr SemanticCode UtcDate = 7;             // PackedTime::ValueType formatted as UTC date
constexpr SemanticCode UtcDateTime = 8;         // PackedTime::ValueType formatted as UTC date & time
constexpr SemanticCode LocalTime = 9;
constexpr SemanticCode LocalDate = 10;
constexpr SemanticCode LocalDateTime = 11;
constexpr SemanticCode Microseconds = 12;       // PackedTime::ValueType formatted as microseconds
constexpr SemanticCode Milliseconds = 13;
constexpr SemanticCode Seconds = 14;
constexpr SemanticCode Percent = 15;

} // namespace Semantics {}


using PropertyFormatter = std::function<std::string(const Property&)>;


[[nodiscard]] ER_RTL_EXPORT PropertyFormatter& findPropertyFormatter(SemanticCode code);
ER_RTL_EXPORT void registerPropertyFormatter(SemanticCode code, PropertyFormatter&& f);




} // namespace Er {}