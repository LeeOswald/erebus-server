#pragma once

#include <erebus/proctree/proctree.hxx>
#include <erebus/rtl/flags.hxx>
#include <erebus/rtl/murmur.hxx>

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

    using FieldComparator = bool(*)(const SelfType& l, const SelfType& r) noexcept;
    using FieldHasher = void(*)(HashValueType& seed, const SelfType& o) noexcept;

    struct FieldInfo
    {
        FieldIndex index;
        std::string_view name;
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

    static const FieldInfo Fields[FieldCount];
};


using ReflectableProcessProperties = Reflectable<ProcessProperties>;


} // Er::ProcessTree {}