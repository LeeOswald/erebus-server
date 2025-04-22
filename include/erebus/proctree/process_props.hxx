#pragma once

#include <erebus/proctree/proctree.hxx>
#include <erebus/rtl/flags.hxx>
#include <erebus/rtl/property_format.hxx>

#include <array>
#include <functional>

#include <boost/functional/hash.hpp>


namespace Er::ProcessTree
{


struct ProcessProperties
{
    struct FieldIndices
    {
        enum : Er::Flag
        {
            Pid,
            PPid,
            PGrp,
            Tpgid,
            Session,
            Ruid,
            Comm,
            CmdLine,
            Exe,
            StartTime,
            State,
            User,
            ThreadCount,
            STime,
            UTime,
            CpuUsage,
            Tty,
            _FieldCount
        };
    };

    std::uint64_t pid;
    std::uint64_t ppid;
    std::uint64_t pgrp;
    std::uint64_t tpgid;
    std::uint64_t session;
    std::uint64_t ruid;
    std::string comm;
    std::string cmdLine;
    std::string exe;
    std::uint64_t startTime;
    std::uint32_t state;
    std::string userName;
    std::int64_t threadCount;
    double sTime;
    double uTime;
    double cpuUsage;
    std::int32_t tty;
};


template <class Type>
    requires 
        requires(const Type v)
        {
            typename Type::FieldIndices;
            { Type::FieldIndices::_FieldCount } -> std::convertible_to<std::size_t>;
        }
struct Reflectable
    : public Type
{
    using SelfType = Type;
    using FieldIndex = Er::Flag;
    using FieldIndices = typename Type::FieldIndices;
    using HashValueType = std::size_t;

    static constexpr std::size_t FieldCount = FieldIndices::_FieldCount;

    using FieldSet = BitSet<FieldCount, FieldIndices>;

    using FieldComparator = std::function<bool(const SelfType& l, const SelfType& r)>;
    using FieldHasher = std::function<void(HashValueType& seed, const SelfType& o)>;

    struct FieldInfo
    {
        FieldIndex index;
        std::string_view name;
        SemanticCode semantic = Semantics::Default;
        FieldComparator comparator;
        FieldHasher hasher;

        constexpr FieldInfo(FieldIndex index, auto&& name, auto&& comparator, auto&& hasher) noexcept
            : index(index)
            , name(std::forward<decltype(name)>(name))
            , comparator(std::forward<decltype(comparator)>(comparator))
            , hasher(std::forward<decltype(hasher)>(hasher))
        {
        }
    };

    using Fields = std::array<FieldInfo, FieldCount>;

    static Fields const& fields() noexcept;

    FieldSet _valid = {};
    
    constexpr HashValueType hash() const noexcept
    {
        return _hash;
    }

    HashValueType updateHash() noexcept
    {
        HashValueType h = {};
        auto& fields = fields();
        for (auto& f : fields)
        {
            if (_valid[f.index])
                f.hasher(h, *this);
        }

        _hash = h;
        return h;
    }

    bool equals(const SelfType& o) const noexcept
    {
        auto& fields = fields();
        for (auto& f : fields)
        {
            if (!f.comparator(*this, o))
                return false;
        }

        return true;
    }

private:
    HashValueType _hash = {};
};

#define ER_REFLECTABLE_FIELD(Class, FieldId, Field) \
FieldInfo{ \
    Class::FieldIndices::FieldId, \
    #FieldId, \
    [](const Class& l, const Class& r) noexcept { return l.Field == r.Field; }, \
    [](HashValueType& seed, const Class& o) noexcept { boost::hash_combine(seed, o.Field); } \
} 

#define ER_REFLECTABLE_FIELD_CMP(Class, FieldId, Field, Cmp) \
FieldInfo{ \
    Class::FieldIndices::FieldId, \
    #FieldId, \
    Cmp, \
    [](HashValueType& seed, const Class& o) noexcept { boost::hash_combine(seed, o.Field); } \
} 

#define ER_REFLECTABLE_FIELDS_BEGIN(Class) \
inline Class::Fields const& Class::fields() noexcept \
{ \
    static const Fields fields = { \


#define ER_REFLECTABLE_FIELDS_END() \
    }; \
    return fields; \
}



using ReflectableProcessProperties = Reflectable<ProcessProperties>;


ER_REFLECTABLE_FIELDS_BEGIN(ReflectableProcessProperties)

    ER_REFLECTABLE_FIELD(ProcessProperties, Pid, pid),
    ER_REFLECTABLE_FIELD(ProcessProperties, PPid, ppid),
    ER_REFLECTABLE_FIELD(ProcessProperties, PGrp, pgrp),
    ER_REFLECTABLE_FIELD(ProcessProperties, Tpgid, tpgid),
    ER_REFLECTABLE_FIELD(ProcessProperties, Session, session),
    ER_REFLECTABLE_FIELD(ProcessProperties, Ruid, ruid),
    ER_REFLECTABLE_FIELD(ProcessProperties, Comm, comm),
    ER_REFLECTABLE_FIELD(ProcessProperties, CmdLine, cmdLine),
    ER_REFLECTABLE_FIELD(ProcessProperties, Exe, exe),
    ER_REFLECTABLE_FIELD(ProcessProperties, StartTime, startTime),
    ER_REFLECTABLE_FIELD(ProcessProperties, State, state),
    ER_REFLECTABLE_FIELD(ProcessProperties, User, userName),
    ER_REFLECTABLE_FIELD(ProcessProperties, ThreadCount, threadCount),
    ER_REFLECTABLE_FIELD_CMP(ProcessProperties, Pid, sTime, [](const ProcessProperties& l, const ProcessProperties& r) noexcept { return std::abs(l.sTime - r.sTime) < 0.01; }),
    ER_REFLECTABLE_FIELD_CMP(ProcessProperties, Pid, uTime, [](const ProcessProperties& l, const ProcessProperties& r) noexcept { return std::abs(l.uTime - r.uTime) < 0.01; }),
    ER_REFLECTABLE_FIELD_CMP(ProcessProperties, CpuUsage, cpuUsage, [](const ProcessProperties& l, const ProcessProperties& r) noexcept { return std::abs(l.cpuUsage - r.cpuUsage) < 0.01; }),
    ER_REFLECTABLE_FIELD(ProcessProperties, Tty, tty)

ER_REFLECTABLE_FIELDS_END()

} // Er::ProcessTree {}