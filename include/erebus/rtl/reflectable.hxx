#pragma once

#include <erebus/rtl/flags.hxx>
#include <erebus/rtl/property.hxx>
#include <erebus/rtl/string_literal.hxx>
#include <erebus/rtl/type_id.hxx>

#include <any>
#include <array>
#include <functional>

#include <boost/functional/hash.hpp>

namespace Er
{

using FieldId = Flag;

template <class _SelfType, std::size_t _FieldCount>
struct Reflectable
{
    using SelfType = _SelfType;

    static constexpr std::size_t FieldCount = _FieldCount;
    using FieldSet = BitSet<FieldCount>;
    
    using HashType = std::size_t;

    using FieldGetter = std::function<void const*(const SelfType& o)>;
    using FieldSetter = std::function<void*(SelfType& o)>;
    using FieldComparator = std::function<bool(const SelfType& l, const SelfType& r)>;
    using FieldHasher = std::function<void(HashType& seed, const SelfType& o)>;
    using FieldCopier = std::function<void(SelfType& me, const SelfType& other)>;
    using FieldMover = std::function<void(SelfType& me, SelfType&& other)>;
    using FieldWrapper = std::function<std::any(const SelfType& o)>;
    using FieldRefWrapper = std::function<std::any(const SelfType& o)>;

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

        void copy(FieldPtr<_Type> field, SelfType& me, const SelfType& other)
        {
            me.*field = other.*field;
        }

        void move(FieldPtr<_Type> field, SelfType& me, SelfType&& other) noexcept
        {
            me.*field = std::move(other.*field);
        }

        std::any wrap(FieldPtr<_Type> field, const SelfType& o)
        {
            return std::make_any<_Type>(o.*field);
        }

        std::any wrap_ref(FieldPtr<_Type> field, const SelfType& o)
        {
            return std::make_any<std::reference_wrapper<const _Type>>(o.*field);
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
        FieldCopier copier;
        FieldMover mover;
        FieldWrapper wrapper;
        FieldRefWrapper ref_wrapper;

        constexpr FieldInfo(
            unsigned id, 
            TypeIndex type, 
            std::string_view name, 
            SemanticCode semantic, 
            auto&& getter, 
            auto&& setter, 
            auto&& comparator, 
            auto&& hasher,
            auto&& copier,
            auto&& mover,
            auto&& wrapper,
            auto&& ref_wrapper
        ) noexcept
            : id(id)
            , type(type)
            , name(name)
            , semantic(semantic)
            , getter(std::forward<decltype(getter)>(getter))
            , setter(std::forward<decltype(setter)>(setter))
            , comparator(std::forward<decltype(comparator)>(comparator))
            , hasher(std::forward<decltype(hasher)>(hasher))
            , copier(std::forward<decltype(copier)>(copier))
            , mover(std::forward<decltype(mover)>(mover))
            , wrapper(std::forward<decltype(wrapper)>(wrapper))
            , ref_wrapper(std::forward<decltype(ref_wrapper)>(ref_wrapper))
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
            },
            [field](SelfType& me, const SelfType& other) -> void
            {
                FieldOperators<_Type> ops;
                ops.copy(field, me, other);
            },
            [field](SelfType& me, SelfType&& other) -> void
            {
                FieldOperators<_Type> ops;
                ops.move(field, me, static_cast<SelfType&&>(other));
            },
            [field](const SelfType& o) -> std::any
            {
                FieldOperators<_Type> ops;
                return ops.wrap(field, o);
            },
            [field](const SelfType& o) -> std::any
            {
                FieldOperators<_Type> ops;
                return ops.wrap_ref(field, o);
            }
        };
    }

    Reflectable() = default;

    Reflectable(const Reflectable&) = default;
    Reflectable& operator=(const Reflectable&) = default;

    void swap(Reflectable& o) noexcept
    {
        using std::swap;
        swap(_valid, o._valid);
        swap(_hash, o._hash);
        swap(_hashValid, o._hashValid);
    }

    Reflectable(Reflectable&& o) noexcept
        : Reflectable()
    {
        swap(o);
    }

    Reflectable& operator=(Reflectable&& o) noexcept
    {
        Reflectable tmp(std::move(o));
        swap(tmp);
        return *this;
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

    FieldSet const& validMask() const noexcept
    {
        return _valid;
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

    struct Diff
    {
        enum class Type : std::uint8_t
        {
            Unchanged = 0,
            Added,
            Removed,
            Changed
        };

        std::size_t differences = 0;
        std::array<Type, FieldCount> map = {};

        constexpr Diff() noexcept = default;
    };

    Diff diff(const Reflectable& o) const noexcept
    {
        Diff difference;

        auto& flds = fields();
        
        auto this_ = static_cast<SelfType const*>(this);
        auto that_ = static_cast<SelfType const*>(&o);

        for (auto& f : flds)
        {
            if (_valid[f.id])
            {
                if (o._valid[f.id])
                {
                    if (!f.comparator(*this_, *that_)) // field value differs
                    {
                        difference.map[f.id] = Diff::Type::Changed;
                        ++difference.differences;
                    }
                }
                else
                {
                    difference.map[f.id] = Diff::Type::Removed; // our field is valid, but their's is not
                    ++difference.differences;
                }
            }
            else if (o._valid[f.id])
            {
                difference.map[f.id] = Diff::Type::Added; // our field is invalid, but their's is
                ++difference.differences;
            }
        }

        return difference;
    }

    Diff update(const Reflectable& o)
    {
        Diff difference;
        if (&o == this)
            return difference;

        auto& flds = fields();

        auto this_ = static_cast<SelfType*>(this);
        auto that_ = static_cast<SelfType const*>(&o);

        for (auto& f : flds)
        {
            if (_valid[f.id])
            {
                if (o._valid[f.id])
                {
                    if (!f.comparator(*this_, *that_)) // field value differs
                    {
                        difference.map[f.id] = Diff::Type::Changed;
                        ++difference.differences;

                        f.copier(*this_, *that_);
                        _hashValid = false;
                    }
                }
                else
                {
                    difference.map[f.id] = Diff::Type::Removed; // our field is valid, but their's is not
                    ++difference.differences;

                    _valid.reset(f.id);
                    _hashValid = false;
                }
            }
            else if (o._valid[f.id])
            {
                difference.map[f.id] = Diff::Type::Added; // our field is invalid, but their's is not
                ++difference.differences;

                f.copier(*this_, *that_);
                _valid.set(f.id);
                _hashValid = false;
            }
        }

        return difference;
    }

    Diff update(Reflectable&& o)
    {
        Diff difference;
        if (&o == this)
            return difference;

        auto& flds = fields();

        auto this_ = static_cast<SelfType*>(this);
        auto that_ = static_cast<SelfType*>(&o);

        for (auto& f : flds)
        {
            if (_valid[f.id])
            {
                if (o._valid[f.id])
                {
                    if (!f.comparator(*this_, *that_)) // field value differs
                    {
                        difference.map[f.id] = Diff::Type::Changed;
                        ++difference.differences;

                        f.mover(*this_, static_cast<SelfType&&>(*that_));
                        
                        _hashValid = false;
                    }
                }
                else
                {
                    difference.map[f.id] = Diff::Type::Removed; // our field is valid, but their's is not
                    ++difference.differences;

                    _valid.reset(f.id);
                    _hashValid = false;
                }
            }
            else if (o._valid[f.id])
            {
                difference.map[f.id] = Diff::Type::Added; // our field is invalid, but their's is
                ++difference.differences;

                f.mover(*this_, static_cast<SelfType&&>(*that_));

                _valid.set(f.id);
                _hashValid = false;
            }
        }

        // since we're 'moving' from 'that'...
        that_->_valid = FieldSet{};
        that_->_hashValid = false;

        return difference;
    }

    std::string_view name(unsigned id) const noexcept
    {
        ErAssert(id < FieldCount);
        auto& flds = fields();
        return flds[id].name;
    }

    TypeIndex type(unsigned id) const noexcept
    {
        ErAssert(id < FieldCount);
        auto& flds = fields();
        return flds[id].type;
    }

    SemanticCode semantics(unsigned id) const noexcept
    {
        ErAssert(id < FieldCount);
        if (!_valid[id])
            return Semantics::Default;

        auto& flds = fields();
        return flds[id].semantic;
    }

    std::any wrapValue(unsigned id) const // NOTE: makes a copy
    {
        ErAssert(id < FieldCount);
        if (!_valid[id])
            return {};

        auto& flds = fields();
        auto this_ = static_cast<SelfType const*>(this);

        return flds[id].wrapper(*this_);
    }

    std::any wrapRef(unsigned id) const // yields a const reference
    {
        ErAssert(id < FieldCount);
        if (!_valid[id])
            return {};

        auto& flds = fields();
        auto this_ = static_cast<SelfType const*>(this);

        return flds[id].ref_wrapper(*this_);
    }

    std::string format(unsigned id) const
    {
        auto any = wrapRef(id);
        auto sema = semantics(id);

        auto formatter = findAnyFormatter(sema);
        return formatter(any);
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
    void set(unsigned id, const _Type& value)
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

        *p = value;

        _valid.set(id, true);
        _hashValid = false;
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



#define ER_REFLECTABLE_FILEDS_BEGIN(Class) \
static Fields const& fields() noexcept \
{ \
    static const Fields fields = \
    {


#define ER_REFLECTABLE_FIELD(Class, Id, SemanticCode, field) \
        reflectableField<Class::Id, decltype(Class::field), #field, SemanticCode>(&Class::field)


#define ER_REFLECTABLE_FILEDS_END() \
    }; \
    return fields; \
}


#define ErSet(Class, Id, obj, field, val) \
    obj.set<decltype(Class::field)>(Class::Id, val)

#define ErGetp(Class, Id, obj, field) \
    obj.get<decltype(Class::field)>(Class::Id)

#define ErGet(Class, Id, obj, field) \
    *obj.get<decltype(Class::field)>(Class::Id)
