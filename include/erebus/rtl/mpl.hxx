#pragma once

#include <erebus/rtl/rtl.hxx>


namespace Er::Mpl
{

template <typename T, T v>
struct Const
{
    static constexpr T value = v;

    using value_type = T;
    using type = Const;

    constexpr operator value_type() const noexcept { return value; }
    constexpr value_type operator()() const noexcept { return value; }
};

template <bool Value>
using Bool = Const<bool, Value>;

template <typename T>
using Not = Bool<!T::value>;

using True = Bool<true>;
using False = Bool<false>;


template <std::size_t N>
using Size = Const<std::size_t, N>;


template <typename _Ty>
struct Identity
{
    using Type = _Ty;
};


template <typename... _Base>
struct Inherit
    : _Base...
{
};


template <std::size_t... I>
struct IndexSequence
{
};


template <typename... _Ty>
struct List 
{
};


namespace __
{

template <typename Seq1, typename Seq2>
struct AppendIndexSequence;

template <size_t... I, size_t... J>
struct AppendIndexSequence<IndexSequence<I...>, IndexSequence<J...>>
{
    using type = IndexSequence<I..., (sizeof...(I) + J)...>;
};

constexpr size_t cpp11_nearest_power_of_2(size_t N)
{
    return (N & (N - 1)) ? cpp11_nearest_power_of_2(N & (N - 1)) : N;
}

static_assert(cpp11_nearest_power_of_2(15) == 8, "");
static_assert(cpp11_nearest_power_of_2(16) == 16, "");

template <size_t N>
struct MakeIndexSequencePowerOf2
{
    static_assert(N != 0);

    using half = typename MakeIndexSequencePowerOf2<(N / 2)>::type;
    using type = typename AppendIndexSequence<half, half>::type;
};

template <>
struct MakeIndexSequencePowerOf2<1>
{
    using type = IndexSequence<0>;
};

template <>
struct MakeIndexSequencePowerOf2<2>
{
    using type = IndexSequence<0, 1>;
};

template <>
struct MakeIndexSequencePowerOf2<4>
{
    using type = IndexSequence<0, 1, 2, 3>;
};

template <>
struct MakeIndexSequencePowerOf2<8>
{
    using type = IndexSequence<0, 1, 2, 3, 4, 5, 6, 7>;
};

template <size_t N>
struct MakeIndexSequence
{
private:
    static constexpr size_t nearest_power_of_2 = cpp11_nearest_power_of_2(N);

    using base = typename MakeIndexSequencePowerOf2<nearest_power_of_2>::type;
    using remaider = typename MakeIndexSequence<(N - nearest_power_of_2)>::type;

public:
    using type = typename AppendIndexSequence<base, remaider>::type;
};

template <>
struct MakeIndexSequence<0>
{
    using type = IndexSequence<>;
};

template <>
struct MakeIndexSequence<1>
{
    using type = IndexSequence<0>;
};

template <>
struct MakeIndexSequence<2>
{
    using type = IndexSequence<0, 1>;
};

template <>
struct MakeIndexSequence<3>
{
    using type = IndexSequence<0, 1, 2>;
};


template <
    typename L1 = List<>,
    typename L2 = List<>,
    typename L3 = List<>,
    typename L4 = List<>,
    typename L5 = List<>,
    typename L6 = List<>,
    typename L7 = List<>,
    typename L8 = List<>,
    typename L9 = List<>,
    typename L10 = List<>
>
struct Append10;


template <
    template <typename...> class L1, typename... T1,
    template <typename...> class L2, typename... T2,
    template <typename...> class L3, typename... T3,
    template <typename...> class L4, typename... T4,
    template <typename...> class L5, typename... T5,
    template <typename...> class L6, typename... T6,
    template <typename...> class L7, typename... T7,
    template <typename...> class L8, typename... T8,
    template <typename...> class L9, typename... T9,
    template <typename...> class L10, typename... T10
>
struct Append10<L1<T1...>, L2<T2...>, L3<T3...>, L4<T4...>, L5<T5...>, L6<T6...>, L7<T7...>, L8<T8...>, L9<T9...>, L10<T10...>>
{
    using type = L1<T1..., T2..., T3..., T4..., T5..., T6..., T7..., T8..., T9..., T10...>;
};


template <
    typename L1 = List<>,
    typename L2 = List<>,
    typename L3 = List<>,
    typename L4 = List<>,
    typename L5 = List<>,
    typename L6 = List<>,
    typename L7 = List<>,
    typename L8 = List<>,
    typename L9 = List<>,
    typename L10 = List<>,
    typename... L
>
struct Append_impl
{
    using type = typename Append_impl<typename Append10<L1, L2, L3, L4, L5, L6, L7, L8, L9, L10>::type, L...>::type;
};

template <
    typename L1,
    typename L2,
    typename L3,
    typename L4,
    typename L5,
    typename L6,
    typename L7,
    typename L8,
    typename L9,
    typename L10
>
struct Append_impl<L1, L2, L3, L4, L5, L6, L7, L8, L9, L10>
{
    using type = typename Append10<L1, L2, L3, L4, L5, L6, L7, L8, L9, L10>::type;
};


template <bool Condition>
struct IfC_impl
{
    template <typename True, typename Else>
    struct apply
    {
        using type = Else;
    };
};

template <>
struct IfC_impl<true>
{
    template <typename True, typename... Else>
    struct apply
    {
        using type = True;
    };
};

} // namespace __{}


template <bool Condition, typename True, typename... Else>
using IfC = typename __::IfC_impl<Condition>::template apply<True, Else...>::type;

template <typename Condition, typename True, typename... Else>
using If = typename __::IfC_impl<static_cast<bool>(Condition::value)>::template apply<True, Else...>::type;


namespace __
{

template <typename L, template <typename...> class P>
struct RemoveIf_impl
{
};

template <template <typename...> class L, typename... T, template <typename...> class P>
struct RemoveIf_impl<L<T...>, P>
{
    template <typename U>
    struct apply_predicate
    {
        using type = If<P<U>, List<>, List<U>>;
    };

    using type = typename Append_impl<L<>, typename apply_predicate<T>::type...>::type;
};


template <typename L, template <typename...> class P>
struct CopyIf_impl
{
};

template <template <typename...> class L, typename... T, template <typename...> class P>
struct CopyIf_impl<L<T...>, P>
{
    template <typename U>
    struct apply_predicate
    {
        using type = If<P<U>, List<U>, List<>>;
    };

    using type = typename Append_impl<L<>, typename apply_predicate<T>::type...>::type;
};


template <template <typename...> class F, typename L>
struct Apply_impl
{
};

template <template <typename...> class F, template <typename...> class L, typename... T>
struct Apply_impl<F, L<T...>>
{
    using type = F<T...>;
};


template <typename T, typename U>
struct IsSame_impl
{
    using type = False;
};

template <typename T>
struct IsSame_impl<T, T>
{
    using type = True;
};


constexpr std::size_t cpp11_constexpr_find_index()
{
    return 0;
}

template <typename... T>
constexpr std::size_t cpp11_constexpr_find_index(bool v1, T... v)
{
    return v1 ? 0 : 1 + cpp11_constexpr_find_index(v...);
}

template <typename L, typename U>
struct Find_impl
{
};

template <template <typename...> class L, typename... T, typename U>
struct Find_impl<L<T...>, U>
{
    static constexpr std::size_t index = cpp11_constexpr_find_index(IsSame_impl<T, U>::type::value...);
    using type = Size<index>;
};


template <typename L, template <typename...> class P>
struct FindIf_impl
{
};

template <template <typename...> class L, typename... T, template <typename...> class P>
struct FindIf_impl<L<T...>, P>
{
    static constexpr size_t index = cpp11_constexpr_find_index(static_cast<bool>(P<T>::value)...);
    using type = Size<index>;
};


template <typename L>
struct Second_impl
{
};

template <template <typename...> class L, typename T1, typename T2, typename... T>
struct Second_impl<L<T1, T2, T...>>
{
    using type = T2;
};


template <template <typename...> class F, typename... L>
struct Transform_impl
{
};

template <template <typename...> class F, template <typename...> class L1, typename... T1>
struct Transform_impl<F, L1<T1...>>
{
#if !defined(_MSC_VER) || defined(__clang__)
    using type = L1<F<T1>...>;
#else
    template <typename... U> struct apply_F { using type = F<U...>; };
    using type = L1<typename apply_F<T1>::type...>;
#endif
};

template <
    template <typename...> class F,
    template <typename...> class L1, typename... T1,
    template <typename...> class L2, typename... T2
>
struct Transform_impl<F, L1<T1...>, L2<T2...>>
{
#if !defined(_MSC_VER) || defined(__clang__)
    using type = L1<F<T1, T2>...>;
#else
    template <typename... U> struct apply_F { using type = F<U...>; };
    using type = L1<typename apply_F<T1, T2>::type...>;
#endif
};

template <
    template <typename...> class F,
    template <typename...> class L1, typename... T1,
    template <typename...> class L2, typename... T2,
    template <typename...> class L3, typename... T3
>
struct Transform_impl<F, L1<T1...>, L2<T2...>, L3<T3...>>
{
#if !defined(_MSC_VER) || defined(__clang__)
    using type = L1<F<T1, T2, T3>...>;
#else
    template <typename... U> struct apply_F { using type = F<U...>; };
    using type = L1<typename apply_F<T1, T2, T3>::type...>;
#endif
};

template <
    template <typename...> class F,
    template <typename...> class L1, typename... T1,
    template <typename...> class L2, typename... T2,
    template <typename...> class L3, typename... T3,
    template <typename...> class L4, typename... T4
>
struct Transform_impl<F, L1<T1...>, L2<T2...>, L3<T3...>, L4<T4...>>
{
#if !defined(_MSC_VER) || defined(__clang__)
    using type = L1<F<T1, T2, T3, T4>...>;
#else
    template <typename... U> struct apply_F { using type = F<U...>; };
    using type = L1<typename apply_F<T1, T2, T3, T4>::type...>;
#endif
};

template <
    template <typename...> class F,
    template <typename...> class L1, typename... T1,
    template <typename...> class L2, typename... T2,
    template <typename...> class L3, typename... T3,
    template <typename...> class L4, typename... T4,
    template <typename...> class L5, typename... T5
>
struct Transform_impl<F, L1<T1...>, L2<T2...>, L3<T3...>, L4<T4...>, L5<T5...>>
{
#if !defined(_MSC_VER) || defined(__clang__)
    using type = L1<F<T1, T2, T3, T4, T5>...>;
#else
    template <typename... U> struct apply_F { using type = F<U...>; };
    using type = L1<typename apply_F<T1, T2, T3, T4, T5>::type...>;
#endif
};


template <typename M, typename K, typename Else = void>
struct MapFind_impl
{
};

template <template <typename...> class M, typename... T, typename K, typename Else>
struct MapFind_impl<M<T...>, K, Else>
{
    template <template <typename...> class L, typename... U>
    static Identity<L<K, U...>> test(Identity<L<K, U...>> *);
    static Identity<Else> test(...);

    using _result = decltype(test(static_cast<Inherit<Identity<T>...> *>(nullptr)));
    using type = typename _result::type;
};


template <typename S>
struct SequenceToList_impl
{
};

template <size_t... I>
struct SequenceToList_impl<IndexSequence<I...>>
{
    using type = List<Size<I>...>;
};


template <typename L>
struct Size_impl
{
};

template <template <typename...> class L, typename... T>
struct Size_impl<L<T...>> : Size<sizeof...(T)>
{
};


template <typename I, typename L, typename... Else>
struct Get_impl
{
    using _seq = typename MakeIndexSequence<Size_impl<L>::value>::type;
    using _index = typename SequenceToList_impl<_seq>::type;
    using _map = typename Transform_impl<List, _index, L>::type;
    using _pair = typename MapFind_impl<_map, I, List<void, Else...>>::type;
    using type = typename Second_impl<_pair>::type;
};


template <typename L, typename U, template <typename...> class F>
struct LeftFold_impl
{
};

template <template <typename...> class L, typename U, template <typename...> class F>
struct LeftFold_impl<L<>, U, F>
{
    using type = U;
};

template <template <typename...> class L, typename T1, typename... T, typename U, template <typename...> class F>
struct LeftFold_impl<L<T1, T...>, U, F>
{
    using type = typename LeftFold_impl<L<T...>, F<U, T1>, F>::type;
};

template <template <typename...> class L, typename T1, typename T2, typename... T, typename U, template <typename...> class F>
struct LeftFold_impl<L<T1, T2, T...>, U, F>
{
    using type = typename LeftFold_impl<L<T...>, F<F<U, T1>, T2>, F>::type;
};

template <template <typename...> class L, typename T1, typename T2, typename T3, typename T4, typename T5, typename... T, typename U, template <typename...> class F>
struct LeftFold_impl<L<T1, T2, T3, T4, T5, T...>, U, F>
{
    using type = typename LeftFold_impl<L<T...>, F<F<F<F<F<U, T1>, T2>, T3>, T4>, T5>, F>::type;
};


template <typename L, typename U, template <typename...> class F>
struct RightFold_impl
{
};

template <template <typename...> class L, typename U, template <typename...> class F>
struct RightFold_impl<L<>, U, F>
{
    using type = U;
};

template <template <typename...> class L, typename T1, typename... T, typename U, template <typename...> class F>
struct RightFold_impl<L<T1, T...>, U, F>
{
    using type = F<T1, typename RightFold_impl<L<T...>, U, F>::type>;
};

template <template <typename...> class L, typename T1, typename T2, typename... T, typename U, template <typename...> class F>
struct RightFold_impl<L<T1, T2, T...>, U, F>
{
    using type = F<T1, F<T2, typename RightFold_impl<L<T...>, U, F>::type>>;
};

template <template <typename...> class L, typename T1, typename T2, typename T3, typename T4, typename T5, typename... T, typename U, template <typename...> class F>
struct RightFold_impl<L<T1, T2, T3, T4, T5, T...>, U, F>
{
    using type = F<T1, F<T2, F<T3, F<T4, F<T5, typename RightFold_impl<L<T...>, U, F>::type>>>>>;
};


} // namespace __{}


template <typename... L>
using Append = typename __::Append_impl<L...>::type;


template <typename L, template <typename...> class P>
using RemoveIf = typename __::RemoveIf_impl<L, P>::type;

template <typename L, typename Q>
using RemoveIfQ = typename __::RemoveIf_impl<L, Q::template apply>::type;


template <typename L, template <typename...> class P>
using CopyIf = typename __::CopyIf_impl<L, P>::type;

template <typename L, typename Q>
using CopyIfQ = typename __::CopyIf_impl<L, Q::template apply>::type;


template <template <typename...> class F, typename L>
using Apply = typename __::Apply_impl<F, L>::type;


template <typename L, template <typename...> class P>
using FindIf = typename __::FindIf_impl<L, P>::type;

template <typename L, typename Q>
using FindIfQ = typename __::FindIf_impl<L, Q::template apply>::type;


template <typename I, typename L, typename... Else>
using Get = typename __::Get_impl<I, L, Else...>::type;

template <std::size_t I, typename L, typename... Else>
using GetC = typename __::Get_impl<Size<I>, L, Else...>::type;


template <template <typename...> class F, typename... L>
using Transform = typename __::Transform_impl<F, L...>::type;

template <typename Q, typename... L>
using TransformQ = typename __::Transform_impl<Q::template apply, L...>::type;


// LeftFold<List<>, U, F>           => U
// LeftFold<List<T1, T2, T3>, U, F> => F<F<F<U, T1>, T2>, T3>
template <typename L, typename U, template <typename...> class F>
using LeftFold = typename __::LeftFold_impl<L, U, F>::type;

// RightFold<List<>, U, F>           => U
// RightFold<List<T1, T2, T3>, U, F> => F<T1, F<T2, F<T3, U>>>
template <typename L, typename U, template <typename...> class F>
using RightFold = typename __::RightFold_impl<L, U, F>::type;



} // namespace Er::Mpl {}