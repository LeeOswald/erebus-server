#pragma once

#include <any>
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
    Pointer,                    // memory address
    Flags,                      // bit mask
    AbsoluteTime,               // UTC time
    Duration,
    Percent,
    Size,                       // bytes
};

} // namespace Semantics {}


using PropertyFormatter = std::function<std::string(const Property&)>;
using AnyFormatter = std::function<std::string(const std::any&)>;


[[nodiscard]] ER_RTL_EXPORT PropertyFormatter& findPropertyFormatter(SemanticCode code);
[[nodiscard]] ER_RTL_EXPORT AnyFormatter& findAnyFormatter(SemanticCode code);

ER_RTL_EXPORT void registerPropertyFormatter(SemanticCode code, PropertyFormatter&& f);
ER_RTL_EXPORT void registerAnyFormatter(SemanticCode code, AnyFormatter&& f);




} // namespace Er {}