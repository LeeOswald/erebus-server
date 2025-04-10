#include <erebus/rtl/format.hxx>
#include <erebus/rtl/property2.hxx>
#include <erebus/rtl/system/packed_time.hxx>

#include <mutex>
#include <shared_mutex>
#include <unordered_map>




namespace Er
{

namespace
{

std::string formatDefault(const Property2& prop)
{
    return prop.str();
}

std::string formatHex(const Property2& prop)
{
    auto ty = prop.type();
    
    if (ty == Property2::Type::Int32)
    {
        auto v = prop.getInt32();
        ErAssert(v);
        return Er::format("{:x}", *v);
    }
    else if (ty == Property2::Type::UInt32)
    {
        auto v = prop.getUInt32();
        ErAssert(v);
        return Er::format("{:x}", *v);
    }
    else if (ty == Property2::Type::Int64)
    {
        auto v = prop.getInt64();
        ErAssert(v);
        return Er::format("{:x}", *v);
    }
    else if (ty == Property2::Type::UInt64)
    {
        auto v = prop.getUInt64();
        ErAssert(v);
        return Er::format("{:x}", *v);
    }
    
    ErAssert(!"Unsupported formatter");
    return prop.str();
}

std::string formatAddress(const Property2& prop)
{
    auto ty = prop.type();

    if (ty == Property2::Type::UInt32)
    {
        auto v = prop.getUInt32();
        ErAssert(v);
        return Er::format("{:08X}", *v);
    }
    else if (ty == Property2::Type::UInt64)
    {
        auto v = prop.getUInt64();
        ErAssert(v);
        return Er::format("{:016X}", *v);
    }
    
    ErAssert(!"Unsupported formatter");
    return prop.str();
}

std::string formatDouble(const Property2& prop)
{
    if (prop.type() == Property2::Type::Double)
    {
        auto v = prop.getDouble();
        ErAssert(*v);
        if (prop.semantics() == Semantics::Scientific)
            return Er::format("{:e}", *v);
        else if (prop.semantics() == Semantics::Fixed)
            return Er::format("{:f}", *v);
        else if (prop.semantics() == Semantics::Fixed3)
            return Er::format("{:.3f}", *v);
    }

    ErAssert(!"Unsupported formatter");
    return prop.str();
}

std::string formatDateTime(const Property2& prop)
{
    if (prop.type() == Property2::Type::UInt64)
    {
        auto v = prop.getUInt64();
        ErAssert(*v);
        Er::System::PackedTime time{*v};
        if (prop.semantics() == Semantics::UtcDate)
        {
            auto tm = time.toUtc();
            return Er::format("{:02d}/{:02d}/{:04d}", tm.tm_mday, tm.tm_mon, tm.tm_year);
        }
        else if (prop.semantics() == Semantics::LocalDate)
        {
            auto tm = time.toLocalTime();
            return Er::format("{:02d}/{:02d}/{:04d}", tm.tm_mday, tm.tm_mon, tm.tm_year);
        }
        else if (prop.semantics() == Semantics::UtcTime)
        {
            auto tm = time.toUtc();
            return Er::format("{:02d}:{:02d}:{:02d}.{:03d}", tm.tm_hour, tm.tm_min, tm.tm_sec, time.milliseconds());
        }
        else if (prop.semantics() == Semantics::LocalTime)
        {
            auto tm = time.toLocalTime();
            return Er::format("{:02d}:{:02d}:{:02d}.{:03d}", tm.tm_hour, tm.tm_min, tm.tm_sec, time.milliseconds());
        }
        else if (prop.semantics() == Semantics::UtcDateTime)
        {
            auto tm = time.toUtc();
            return Er::format("{:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}", tm.tm_mday, tm.tm_mon, tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);
        }
        else if (prop.semantics() == Semantics::LocalDateTime)
        {
            auto tm = time.toLocalTime();
            return Er::format("{:02d}/{:02d}/{:04d} {:02d}:{:02d}:{:02d}", tm.tm_mday, tm.tm_mon, tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);
        }
    }

    ErAssert(!"Unsupported formatter");
    return prop.str();
}

std::string formatDuration(const Property2& prop)
{
    if (prop.type() == Property2::Type::UInt64)
    {
        auto v = prop.getUInt64();
        ErAssert(*v);
        if (prop.semantics() == Semantics::Microseconds)
            return Er::format("{} \u03bcs", *v);
        else if (prop.semantics() == Semantics::Microseconds)
            return Er::format("{} ms", *v / 1000ULL);
        else if (prop.semantics() == Semantics::Seconds)
            return Er::format("{} ms", *v / 1000000ULL);
    }

    ErAssert(!"Unsupported formatter");
    return prop.str();
}

std::string formatPercent(const Property2& prop)
{
    if (prop.type() == Property2::Type::Double)
    {
        auto v = prop.getDouble();
        ErAssert(*v);
        return Er::format("{:.2f}%", *v);
    }
    else if (prop.type() == Property2::Type::Int32)
    {
        auto v = prop.getInt32();
        ErAssert(*v);
        return Er::format("{}%", *v);
    }
    else if (prop.type() == Property2::Type::UInt32)
    {
        auto v = prop.getUInt32();
        ErAssert(*v);
        return Er::format("{}%", *v);
    }
    else if (prop.type() == Property2::Type::Int64)
    {
        auto v = prop.getInt64();
        ErAssert(*v);
        return Er::format("{}%", *v);
    }
    else if (prop.type() == Property2::Type::UInt64)
    {
        auto v = prop.getUInt64();
        ErAssert(*v);
        return Er::format("{}%", *v);
    }

    ErAssert(!"Unsupported formatter");
    return prop.str();
}

struct Registry
{
    std::shared_mutex mutex;
    std::unordered_map<SemanticCode, PropertyFormatter> formatters;

    Registry()
    {
        formatters.insert({ Semantics::Default, formatDefault });
        formatters.insert({ Semantics::Hex, formatHex });
        formatters.insert({ Semantics::Address, formatAddress });
        formatters.insert({ Semantics::Scientific, formatDouble });
        formatters.insert({ Semantics::Fixed, formatDouble });
        formatters.insert({ Semantics::Fixed3, formatDouble });
        formatters.insert({ Semantics::UtcDate, formatDateTime });
        formatters.insert({ Semantics::UtcDateTime, formatDateTime });
        formatters.insert({ Semantics::UtcTime, formatDateTime });
        formatters.insert({ Semantics::LocalDate, formatDateTime });
        formatters.insert({ Semantics::LocalDateTime, formatDateTime });
        formatters.insert({ Semantics::LocalTime, formatDateTime });
        formatters.insert({ Semantics::Microseconds, formatDuration });
        formatters.insert({ Semantics::Milliseconds, formatDuration });
        formatters.insert({ Semantics::Seconds, formatDuration });
        formatters.insert({ Semantics::Percent, formatPercent });
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
    auto it = r.formatters.find(code);
    if (it != r.formatters.end())
        return it->second;

    static PropertyFormatter def{ formatDefault };
    return def;
}

ER_RTL_EXPORT void registerPropertyFormatter(SemanticCode code, PropertyFormatter&& f)
{
    auto& r = registry();

    std::unique_lock l(r.mutex);
    r.formatters.insert({ code, std::move(f) });
}

} // namespace Er {}
