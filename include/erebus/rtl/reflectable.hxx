#pragma once

#include <erebus/rtl/flags.hxx>
#include <erebus/rtl/property_format.hxx>
#include <erebus/rtl/string_literal.hxx>
#include <erebus/rtl/type_id.hxx>

#include <array>
#include <functional>

#include <boost/functional/hash.hpp>

namespace Er
{

template <class _Traits>
    requires
        requires(_Traits t)
        {
            typename _Traits::SelfType;
            typename _Traits::FieldIds;
            { _Traits::FieldCount } -> std::convertible_to<std::size_t>;
        }
struct Reflectable
{
    using Traits = _Traits;
    
    using SelfType = typename Traits::SelfType;

    using FieldIds = typename Traits::FieldIds;

    static constexpr std::size_t FieldCount = Traits::FieldCount;
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
        TypeIndex type;
        std::string_view name;
        SemanticCode semantic;
        FieldGetter getter;
        FieldSetter setter;
        FieldComparator comparator;
        FieldHasher hasher;

        constexpr FieldInfo(unsigned id, TypeIndex type, std::string_view name, SemanticCode semantic, auto&& getter, auto&& setter, auto&& comparator, auto&& hasher) noexcept
            : id(id)
            , type(type)
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
            typeId<_Type>().index(),
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

    Reflectable() = default;

    Reflectable(const Reflectable&) = default;
    Reflectable& operator=(const Reflectable&) = default;

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
        _hashValid = false;
    }

    constexpr HashType hash() const noexcept
    {
        if (!_hashValid)
            return updateHash();

        return _hash;
    }

    HashType updateHash() const noexcept
    {
        HashType h = {};
        auto& flds = fields();
        auto this_ = static_cast<SelfType const*>(this);
        for (auto& f : flds)
        {
            if (_valid[f.id])
                f.hasher(h, *this_);
        }

        _hash = h;
        _hashValid = true;
        return h;
    }

    TypeIndex type(unsigned id) const noexcept
    {
        ErAssert(id < FieldCount);
        auto& flds = fields();
        return flds[id].type;
    }

    template <typename _Type>
    _Type const* get(unsigned id) const noexcept
    {
        ErAssert(id < FieldCount);
        if (!_valid[id])
            return nullptr;

        auto& flds = fields();
#if ER_DEBUG
        ErAssert(flds[id].type == typeId<_Type>().index());
#endif

        auto this_ = static_cast<SelfType const*>(this);
        return static_cast<_Type const*>(flds[id].getter(*this_));
    }

    template <typename _Type>
    void set(unsigned id, _Type&& value)
    {
        ErAssert(id < FieldCount);
        auto& flds = fields();
#if ER_DEBUG
        ErAssert(flds[id].type == typeId<_Type>().index());
#endif

        auto this_ = static_cast<SelfType*>(this);
        auto raw = flds[id].setter(*this_);
        auto p = static_cast<std::remove_cv_t<_Type>*>(raw);
        ErAssert(p);

        *p = std::forward<_Type>(value);

        _valid.set(id, true);
        _hashValid = false;
    }

protected:
    FieldSet _valid = {};
    mutable HashType _hash = {};
    mutable bool _hashValid = false;
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
    static constexpr unsigned FieldCount = FieldIds::_FieldCount; \
}; 



#define ER_REFLECTABLE_FILEDS_BEGIN(Class) \
static Fields const& fields() noexcept \
{ \
    static const Fields fields = \
    {


#define ER_REFLECTABLE_FIELD(Class, Id, SemanticCode, field) \
        reflectableField<FieldIds::Id, decltype(Class::field), #field, SemanticCode>(&Class::field)


#define ER_REFLECTABLE_FILEDS_END() \
    }; \
    return fields; \
}


#define ErSet(Class, Id, obj, field, val) \
    obj.set(Class::Traits::FieldIds::Id, decltype(Class::field){val})

#define ErGetp(Class, Id, obj, field) \
    obj.get<decltype(Class::field)>(Class::Traits::FieldIds::Id)

#define ErGet(Class, Id, obj, field) \
    *obj.get<decltype(Class::field)>(Class::Traits::FieldIds::Id)