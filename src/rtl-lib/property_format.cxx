#include <erebus/rtl/bool.hxx>
#include <erebus/rtl/format.hxx>
#include <erebus/rtl/property.hxx>
#include <erebus/rtl/system/packed_time.hxx>

#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>




namespace Er
{

namespace
{

std::string formatDefault(const Property& prop)
{
    return prop.str();
}

std::string formatDefaultAny(const std::any& any)
{
    std::ostringstream ss;

    if (!any.has_value())
        ss << "<empty>";
    else if (auto v = std::any_cast<std::string>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const std::string>>(&any))
        ss << v->get();
    else if (auto v = std::any_cast<bool>(&any))
        ss << std::boolalpha << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const bool>>(&any))
        ss << std::boolalpha << v->get();
    else if (auto v = std::any_cast<Bool>(&any))
        ss << (*v ? "True" : "False");
    else if (auto v = std::any_cast<std::reference_wrapper<const Bool>>(&any))
        ss << (v->get() ? "True" : "False");
    else if (auto v = std::any_cast<char>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const char>>(&any))
        ss << v->get();
    else if (auto v = std::any_cast<unsigned char>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const unsigned char>>(&any))
        ss << v->get();
    else if (auto v = std::any_cast<short>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const short>>(&any))
        ss << v->get();
    else if (auto v = std::any_cast<unsigned short>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const unsigned short>>(&any))
        ss << v->get();
    else if (auto v = std::any_cast<int>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const int>>(&any))
        ss << v->get();
    else if (auto v = std::any_cast<unsigned int>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const unsigned int>>(&any))
        ss << v->get();
    else if (auto v = std::any_cast<long>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const long>>(&any))
        ss << v->get();
    else if (auto v = std::any_cast<unsigned long>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const unsigned long>>(&any))
        ss << v->get();
    else if (auto v = std::any_cast<long long>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const long long>>(&any))
        ss << v->get();
    else if (auto v = std::any_cast<unsigned long long>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const unsigned long long>>(&any))
        ss << v->get();
    else if (auto v = std::any_cast<std::byte>(&any))
        ss << unsigned(*v);
    else if (auto v = std::any_cast<std::reference_wrapper<const std::byte>>(&any))
        ss << unsigned(v->get());
    else if (auto v = std::any_cast<std::int8_t>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const std::int8_t>>(&any))
        ss << v->get();
    else if (auto v = std::any_cast<std::uint8_t>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const std::uint8_t>>(&any))
        ss << v->get();
    else if (auto v = std::any_cast<std::int16_t>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const std::int16_t>>(&any))
        ss << v->get();
    else if (auto v = std::any_cast<std::uint16_t>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const std::uint16_t>>(&any))
        ss << v->get();
    else if (auto v = std::any_cast<std::int32_t>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const std::int32_t>>(&any))
        ss << v->get();
    else if (auto v = std::any_cast<std::uint32_t>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const std::uint32_t>>(&any))
        ss << v->get();
    else if (auto v = std::any_cast<std::int64_t>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const std::int64_t>>(&any))
        ss << v->get();
    else if (auto v = std::any_cast<std::uint64_t>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const std::uint64_t>>(&any))
        ss << v->get();
    else if (auto v = std::any_cast<float>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const float>>(&any))
        ss << v->get();
    else if (auto v = std::any_cast<double>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const double>>(&any))
        ss << v->get();
    else if (auto v = std::any_cast<long double>(&any))
        ss << *v;
    else if (auto v = std::any_cast<std::reference_wrapper<const long double>>(&any))
        ss << v->get();
    else
    {
        ss << "[" << any.type().name() << "]";
    }

    return ss.str();
}


template <std::integral _Ty>
std::string formatPointerImpl(const _Ty* v)
{
    ErAssert(v);

    if constexpr (sizeof(*v) == 4)
        return Er::format("{:08X}", *v);

    return Er::format("{:016X}", *v);
}

std::string formatPointer(const Property& prop)
{
    auto ty = prop.type();

    if (ty == Property::Type::UInt64)
        return formatPointerImpl(prop.getUInt64());
    else if (ty == Property::Type::UInt32)
        return formatPointerImpl(prop.getUInt32());
    
    ErAssert(!"Unsupported formatter");
    return prop.str();
}

std::string formatPointerAny(const std::any& any)
{
    if (auto v = std::any_cast<std::uint64_t>(&any))
        return formatPointerImpl(v);
    else if (auto v = std::any_cast<std::reference_wrapper<const std::uint64_t>>(&any))
        return formatPointerImpl(&v->get());
    else if (auto v = std::any_cast<std::uint32_t>(&any))
        return formatPointerImpl(v);
    else if (auto v = std::any_cast<std::reference_wrapper<const std::uint32_t>>(&any))
        return formatPointerImpl(&v->get());

    ErAssert(!"Unsupported formatter");
    return formatDefaultAny(any);
}

template <std::integral _Ty>
std::string formatFlagsImpl(const _Ty* v)
{
    ErAssert(v);

    if constexpr (sizeof(*v) == 4)
        return Er::format("{:08x}", *v);

    return Er::format("{:016x}", *v);
}

std::string formatFlags(const Property& prop)
{
    auto ty = prop.type();

    if (ty == Property::Type::UInt32)
        return formatFlagsImpl(prop.getUInt32());
    else if (ty == Property::Type::UInt64)
        return formatFlagsImpl(prop.getUInt64());

    ErAssert(!"Unsupported formatter");
    return prop.str();
}

std::string formatFlagsAny(const std::any& any)
{
    if (auto v = std::any_cast<std::uint32_t>(&any))
        return formatFlagsImpl(v);
    else if (auto v = std::any_cast<std::reference_wrapper<const std::uint32_t>>(&any))
        return formatFlagsImpl(&v->get());
    else if (auto v = std::any_cast<std::uint64_t>(&any))
        return formatFlagsImpl(v);
    else if (auto v = std::any_cast<std::reference_wrapper<const std::uint64_t>>(&any))
        return formatFlagsImpl(&v->get());

    ErAssert(!"Unsupported formatter");
    return formatDefaultAny(any);
}

std::string formatDateTimeImpl(const System::PackedTime::ValueType v)
{
    Er::System::PackedTime time{ v };

    auto tm = time.toUtc();

    return Er::format("{:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

std::string formatDateTime(const Property& prop)
{
    if (prop.type() == Property::Type::UInt64)
    {
        return formatDateTimeImpl(*prop.getUInt64());
    }

    ErAssert(!"Unsupported formatter");
    return prop.str();
}

std::string formatDateTimeAny(const std::any& any)
{
    if (auto v = std::any_cast<System::PackedTime::ValueType>(&any))
        return formatDateTimeImpl(*v);
    else if (auto v = std::any_cast<std::reference_wrapper<const System::PackedTime::ValueType>>(&any))
        return formatDateTimeImpl(v->get());
    else if (auto v = std::any_cast<System::PackedTime>(&any))
        return formatDateTimeImpl(v->value());
    else if (auto v = std::any_cast<std::reference_wrapper<const System::PackedTime>>(&any))
        return formatDateTimeImpl(v->get().value());

    ErAssert(!"Unsupported formatter");
    return formatDefaultAny(any);
}

std::string formatDurationImpl(const System::PackedTime::ValueType v)
{
    if (v < 2000)
        return Er::format("{} \u03bcs", v);
    else if (v, 2000000ULL)
        return Er::format("{} ms", v / 1000ULL);
    else
        return Er::format("{:.3f} s", v / 1000000.0);
}

std::string formatDuration(const Property& prop)
{
    if (prop.type() == Property::Type::UInt64)
    {
        return formatDurationImpl(*prop.getUInt64());
    }

    ErAssert(!"Unsupported formatter");
    return prop.str();
}

std::string formatDurationAny(const std::any& any)
{
    if (auto v = std::any_cast<System::PackedTime::ValueType>(&any))
        return formatDurationImpl(*v);
    else if (auto v = std::any_cast<std::reference_wrapper<const System::PackedTime::ValueType>>(&any))
        return formatDurationImpl(v->get());
    else if (auto v = std::any_cast<System::PackedTime>(&any))
        return formatDurationImpl(v->value());
    else if (auto v = std::any_cast<std::reference_wrapper<const System::PackedTime>>(&any))
        return formatDurationImpl(v->get().value());

    ErAssert(!"Unsupported formatter");
    return formatDefaultAny(any);
}

template <std::integral _Ty>
std::string formatPercentImpl(const _Ty* v)
{
    ErAssert(v);

    return Er::format("{}%", *v);
}

template <std::floating_point _Ty>
std::string formatPercentImpl(const _Ty* v)
{
    ErAssert(v);

    return Er::format("{:.2f}%", *v);
}

std::string formatPercent(const Property& prop)
{
    if (prop.type() == Property::Type::Double)
    {
        return formatPercentImpl(prop.getDouble());
    }
    else if (prop.type() == Property::Type::Int32)
    {
        return formatPercentImpl(prop.getInt32());
    }
    else if (prop.type() == Property::Type::UInt32)
    {
        return formatPercentImpl(prop.getUInt32());
    }
    else if (prop.type() == Property::Type::Int64)
    {
        return formatPercentImpl(prop.getInt64());
    }
    else if (prop.type() == Property::Type::UInt64)
    {
        return formatPercentImpl(prop.getUInt64());
    }

    ErAssert(!"Unsupported formatter");
    return prop.str();
}

std::string formatPercentAny(const std::any& any)
{
    if (auto v = std::any_cast<double>(&any))
        return formatPercentImpl(v);
    else if (auto v = std::any_cast<std::reference_wrapper<const double>>(&any))
        return formatPercentImpl(&v->get());
    else if (auto v = std::any_cast<std::int32_t>(&any))
        return formatPercentImpl(v);
    else if (auto v = std::any_cast<std::reference_wrapper<const std::int32_t>>(&any))
        return formatPercentImpl(&v->get());
    else if (auto v = std::any_cast<std::uint32_t>(&any))
        return formatPercentImpl(v);
    else if (auto v = std::any_cast<std::reference_wrapper<const std::uint32_t>>(&any))
        return formatPercentImpl(&v->get());
    else if (auto v = std::any_cast<std::int64_t>(&any))
        return formatPercentImpl(v);
    else if (auto v = std::any_cast<std::reference_wrapper<const std::int64_t>>(&any))
        return formatPercentImpl(&v->get());
    else if (auto v = std::any_cast<std::uint64_t>(&any))
        return formatPercentImpl(v);
    else if (auto v = std::any_cast<std::reference_wrapper<const std::uint64_t>>(&any))
        return formatPercentImpl(&v->get());

    ErAssert(!"Unsupported formatter");
    return formatDefaultAny(any);
}

template <std::integral _Ty>
std::string formatSizeImpl(const _Ty* v)
{
    ErAssert(v);
    auto size = static_cast<std::uint64_t>(*v);

    if (size < 1024 * 9)
        return Er::format("{} bytes", size);
    else if (size < 1024ULL * 1024 * 9)
        return Er::format("{:.2f} KiB", size / 1024.0);
    else if (size < 1024ULL * 1024 * 1024 * 9)
        return Er::format("{:.2f} MiB", size / 1024 * 1024.0);

    return Er::format("{:.2f} GiB", size / 1024 * 1024 * 1024.0);
}

std::string formatSize(const Property& prop)
{
    if (prop.type() == Property::Type::Int32)
    {
        return formatSizeImpl(prop.getInt32());
    }
    else if (prop.type() == Property::Type::UInt32)
    {
        return formatSizeImpl(prop.getUInt32());
    }
    else if (prop.type() == Property::Type::Int64)
    {
        return formatSizeImpl(prop.getInt64());
    }
    else if (prop.type() == Property::Type::UInt64)
    {
        return formatSizeImpl(prop.getUInt64());
    }

    ErAssert(!"Unsupported formatter");
    return prop.str();
}

std::string formatSizeAny(const std::any& any)
{
    if (auto v = std::any_cast<std::int32_t>(&any))
        return formatSizeImpl(v);
    else if (auto v = std::any_cast<std::reference_wrapper<const std::int32_t>>(&any))
        return formatSizeImpl(&v->get());
    else if (auto v = std::any_cast<std::uint32_t>(&any))
        return formatSizeImpl(v);
    else if (auto v = std::any_cast<std::reference_wrapper<const std::uint32_t>>(&any))
        return formatSizeImpl(&v->get());
    else if (auto v = std::any_cast<std::int64_t>(&any))
        return formatSizeImpl(v);
    else if (auto v = std::any_cast<std::reference_wrapper<const std::int64_t>>(&any))
        return formatSizeImpl(&v->get());
    else if (auto v = std::any_cast<std::uint64_t>(&any))
        return formatSizeImpl(v);
    else if (auto v = std::any_cast<std::reference_wrapper<const std::uint64_t>>(&any))
        return formatSizeImpl(&v->get());

    ErAssert(!"Unsupported formatter");
    return formatDefaultAny(any);
}

struct Registry
{
    std::shared_mutex mutex;
    std::unordered_map<SemanticCode, PropertyFormatter> propertyFormatters;
    std::unordered_map<SemanticCode, AnyFormatter> anyFormatters;

    Registry()
    {
        propertyFormatters.insert({ Semantics::Pointer, formatPointer });
        propertyFormatters.insert({ Semantics::Flags, formatFlags });
        propertyFormatters.insert({ Semantics::AbsoluteTime, formatDateTime });
        propertyFormatters.insert({ Semantics::Duration, formatDuration });
        propertyFormatters.insert({ Semantics::Size, formatSize });
        propertyFormatters.insert({ Semantics::Percent, formatPercent });


        anyFormatters.insert({ Semantics::Pointer, formatPointerAny });
        anyFormatters.insert({ Semantics::Flags, formatFlagsAny });
        anyFormatters.insert({ Semantics::AbsoluteTime, formatDateTimeAny });
        anyFormatters.insert({ Semantics::Duration, formatDurationAny });
        anyFormatters.insert({ Semantics::Size, formatSizeAny });
        anyFormatters.insert({ Semantics::Percent, formatPercentAny });
    }
};


Registry& registry()
{
    static Registry r;
    return r;
}


} // namespace {}


ER_RTL_EXPORT PropertyFormatter& findPropertyFormatter(SemanticCode code)
{
    auto& r = registry();

    std::shared_lock l(r.mutex);
    auto it = r.propertyFormatters.find(code);
    if (it != r.propertyFormatters.end())
        return it->second;

    static PropertyFormatter def{ formatDefault };
    return def;
}

ER_RTL_EXPORT AnyFormatter& findAnyFormatter(SemanticCode code)
{
    auto& r = registry();

    std::shared_lock l(r.mutex);
    auto it = r.anyFormatters.find(code);
    if (it != r.anyFormatters.end())
        return it->second;

    static AnyFormatter def{ formatDefaultAny };
    return def;
}

ER_RTL_EXPORT void registerPropertyFormatter(SemanticCode code, PropertyFormatter&& f)
{
    auto& r = registry();

    std::unique_lock l(r.mutex);
    r.propertyFormatters.insert({ code, std::move(f) });
}

ER_RTL_EXPORT void registerAnyFormatter(SemanticCode code, AnyFormatter&& f)
{
    auto& r = registry();

    std::unique_lock l(r.mutex);
    r.anyFormatters.insert({ code, std::move(f) });
}

} // namespace Er {}
