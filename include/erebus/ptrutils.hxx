#pragma once

#include <erebus/erebus.hxx>


namespace Er
{

template <typename T>
struct IsUniquePtrType
    : std::false_type
{
};

template <typename T>
struct IsUniquePtrType<std::unique_ptr<T>>
    : std::true_type
{
};

template <typename T>
struct IsUniquePtrType<std::unique_ptr<T, void (*)(T*)>>
    : std::true_type
{
};

template <typename T>
struct IsSharedPtrType
    : std::false_type
{
};

template <typename T>
struct IsSharedPtrType<std::shared_ptr<T>>
    : std::true_type
{
};

template <typename T>
struct IsWeakPtrType
    : std::false_type
{
};

template <typename T>
struct IsWeakPtrType<std::weak_ptr<T>>
    : std::true_type
{
};

template <typename T>
struct IsPlainPtrType
    : std::is_pointer<T>
{
};

template <typename T>
struct IsMemberPtrType
    : std::is_member_pointer<T>
{
};

namespace __
{

template <typename T>
struct IsSimpleOwningPtrType
    : std::bool_constant<IsUniquePtrType<T>::value || IsSharedPtrType<T>::value>
{
};

template <typename T, typename EnableTag = void>
struct IsOwningPtrTypeImpl
    : std::false_type
{
};

template <typename T>
struct IsOwningPtrTypeImpl<T, std::enable_if_t<IsSimpleOwningPtrType<T>::value, void>>
    : std::true_type
{
};

} // namespace __ {}

template <typename T>
struct IsOwningPtrType
    : __::IsOwningPtrTypeImpl<T>
{
};

template <typename T>
struct IsNormalPtrType
    : std::bool_constant<IsOwningPtrType<T>::value || IsPlainPtrType<T>::value>
{
};

template <typename T>
struct IsSpecialPtrType
    : std::bool_constant<IsWeakPtrType<T>::value || IsMemberPtrType<T>::value>
{
};

template <typename T>
struct IsPtrType
    : std::bool_constant<IsNormalPtrType<T>::value || IsSpecialPtrType<T>::value>
{
};

template <typename T, std::enable_if_t<IsOwningPtrType<std::decay_t<T>>::value>* = nullptr>
auto getPlainPtr(T& t) -> decltype(t.get())
{
    return t.get();
}

template <typename T, std::enable_if_t<IsPlainPtrType<std::decay_t<T>>::value>* = nullptr>
T getPlainPtr(T t)
{
    return t;
}

template <typename T, std::enable_if_t<!IsPtrType<std::decay_t<T>>::value>* = nullptr>
auto getPlainPtr(T& t) -> decltype(&t)
{
    return &t;
}

} // namespace Er{}