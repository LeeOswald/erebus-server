#pragma once

#include <erebus/rtl/flags.hxx>
#include <erebus/rtl/property_format.hxx>
#include <erebus/rtl/string_literal.hxx>

#include <array>
#include <functional>

#include <boost/functional/hash.hpp>

namespace Er
{

template <class _Type>
struct ReflectableTraits;


template <class _Traits>
struct Reflectable
{
    using Traits = _Traits;
    
    using SelfType = typename Traits::SelfType;

    using FieldIds = typename Traits::FieldIds;

    static constexpr std::size_t FieldCount = FieldIds::_FieldCount;
    using FieldSet = BitSet<FieldCount, FieldIds>;
    
    using HashType = std::size_t;

    using FieldComparator = std::function<bool(const SelfType& l, const SelfType& r)>;
    using FieldHasher = std::function<void(HashType& seed, const SelfType& o)>;

    struct FieldInfo
    {
        unsigned id;
        std::string_view name;
        SemanticCode semantic;
        FieldComparator comparator;
        FieldHasher hasher;

        constexpr FieldInfo(unsigned id, std::string_view name, SemanticCode semantic, auto&& comparator) noexcept
            : id(id)
            , name(name)
            , semantic(semantic)
            , comparator(std::forward<decltype(comparator)>(comparator))
        {
        }
    };

    template <
        unsigned _Id, 
        typename _Type, 
        StringLiteral _Name,
        SemanticCode _SemanticCode
    >
        requires std::is_default_constructible_v<_Type>
    struct ReflectableField
    {
        using Type = _Type;

        using FieldPtr = ReflectableField SelfType::*;

        static constexpr unsigned Id = _Id;
        static constexpr std::string_view Name = std::string_view{ _Name.data(), _Name.size() };
        static constexpr SemanticCode Semantic = _SemanticCode;

        Type value = Type{};

        ReflectableField() noexcept(noexcept(std::is_nothrow_default_constructible_v<Type>)) = default;
        
        ReflectableField(auto&&... args) noexcept(noexcept(std::is_nothrow_constructible_v<Type, decltype(args)...>))
            : value(std::forward<args>(args...))
        {
        }

        struct Operators
        {
            bool equal(FieldPtr field, const SelfType& l, const SelfType& r) noexcept
            {
                return (l.*field).value == (r.*field).value;
            }

            void hash(HashType& seed, FieldPtr field, const SelfType& o) noexcept
            {
                boost::hash_combine(seed, (o.*field).value);
            }
        };

        static constexpr FieldInfo fieldInfo(FieldPtr field) noexcept
        {
            return FieldInfo
            { 
                Id, 
                Name, 
                Semantic,
                [field](const SelfType& l, const SelfType& r) -> bool
                {
                    Operators ops;
                    return ops.equal(field, l, r);
                }
            };
        }
    };

    using Fields = std::array<FieldInfo, FieldCount>;

    static Fields const& fields() noexcept
    {
        return SelfType::fields();
    }
};


} // namespace Er {}