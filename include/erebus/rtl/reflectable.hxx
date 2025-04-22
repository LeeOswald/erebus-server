#pragma once

#include <erebus/rtl/flags.hxx>
#include <erebus/rtl/property_format.hxx>
#include <erebus/rtl/string_literal.hxx>

#include <array>
#include <functional>

#include <boost/functional/hash.hpp>

namespace Er
{

template <class _Traits>
struct Reflectable
{
    using Traits = _Traits;
    
    using SelfType = typename Traits::SelfType;

    using FieldIds = typename Traits::FieldIds;

    static constexpr std::size_t FieldCount = FieldIds::_FieldCount;
    using FieldSet = BitSet<FieldCount, FieldIds>;
    
    using HashType = std::size_t;

    using FieldGetter = std::function<void const*(const SelfType& o)>;
    using FieldSetter = std::function<void*(SelfType& o)>;
    using FieldComparator = std::function<bool(const SelfType& l, const SelfType& r)>;
    using FieldHasher = std::function<void(HashType& seed, const SelfType& o)>;

    template <typename _Type>
    using FieldPtr = _Type SelfType::*;

    template <typename _Type>
    struct FieldOperators
    {
        void const* get(FieldPtr<_Type> field, const SelfType& o) noexcept
        {
            return &(o.*field);
        }

        void* set(FieldPtr<_Type> field, SelfType& o) noexcept
        {
            return &(o.*field);
        }

        bool equal(FieldPtr<_Type> field, const SelfType& l, const SelfType& r) noexcept
        {
            return l.*field == r.*field;
        }

        void hash(HashType& seed, FieldPtr<_Type> field, const SelfType& o) noexcept
        {
            boost::hash_combine(seed, o.*field);
        }
    };

    struct FieldInfo
    {
        unsigned id;
        std::string_view name;
        SemanticCode semantic;
        FieldGetter getter;
        FieldSetter setter;
        FieldComparator comparator;
        FieldHasher hasher;

        constexpr FieldInfo(unsigned id, std::string_view name, SemanticCode semantic, auto&& getter, auto&& setter, auto&& comparator, auto&& hasher) noexcept
            : id(id)
            , name(name)
            , semantic(semantic)
            , getter(std::forward<decltype(getter)>(getter))
            , setter(std::forward<decltype(setter)>(setter))
            , comparator(std::forward<decltype(comparator)>(comparator))
            , hasher(std::forward<decltype(hasher)>(hasher))
        {
        }
    };

    template <
        unsigned _Id,
        typename _Type,
        StringLiteral _Name,
        SemanticCode _SemanticCode
    >
    static constexpr FieldInfo reflectableField(FieldPtr<_Type> field) noexcept
    {
        return FieldInfo
        {
            _Id,
            std::string_view { _Name.data(), _Name.size() },
            _SemanticCode,
            [field](const SelfType& o) -> void const*
            {
                FieldOperators<_Type> ops;
                return ops.get(field, o);
            },
            [field](SelfType& o) -> void*
            {
                FieldOperators<_Type> ops;
                return ops.set(field, o);
            },
            [field](const SelfType& l, const SelfType& r) -> bool
            {
                FieldOperators<_Type> ops;
                return ops.equal(field, l, r);
            },
            [field](HashType& seed, const SelfType& o) -> void
            {
                FieldOperators<_Type> ops;
                ops.hash(seed, field, o);
            }
        };
    }

    using Fields = std::array<FieldInfo, FieldCount>;

    static Fields const& fields() noexcept
    {
        return SelfType::fields();
    }

    bool valid(unsigned id) const noexcept
    {
        ErAssert(id < FieldCount);
        return _valid[id];
    }

    void setValid(unsigned id, bool valid = true) noexcept
    {
        ErAssert(id < FieldCount);
        _valid.set(id, valid);
    }

    constexpr HashType hash() const noexcept
    {
        return _hash;
    }

    HashType updateHash() noexcept
    {
        HashType h = {};
        auto& flds = fields();
        for (auto& f : flds)
        {
            if (_valid[f.id])
                f.hasher(h, *static_cast<SelfType*>(this));
        }

        _hash = h;
        return h;
    }

    template <typename Ty>
    Ty const* get(unsigned id) const noexcept
    {
        ErAssert(id < FieldCount);
        if (!_valid[id])
            return nullptr;

        auto& flds = fields();
        return flds[id].getter(*static_cast<SelfType*>(this));
    }

protected:
    FieldSet _valid = {};
    HashType _hash = {};
};


} // namespace Er {}


#define ER_REFLECTABLE_IDS_BEGIN(Class, Traits) \
struct Class; \
struct Traits \
{ \
    using SelfType = Class; \
    struct FieldIds \
    { \
        enum : unsigned \
        { 


#define ER_REFLECTABLE_IDS_END() \
            , _FieldCount \
        }; \
    }; \
}; 



#define ER_REFLECTABLE_FILEDS_BEGIN(Class) \
static Fields const& fields() noexcept \
{ \
    static const Fields fields = \
    {


#define ER_REFLECTABLE_FIELD(Class, Id, Type, SemanticCode, field) \
        reflectableField<FieldIds::Id, Type, #field, SemanticCode>(&Class::field)


#define ER_REFLECTABLE_FILEDS_END() \
    }; \
    return fields; \
}