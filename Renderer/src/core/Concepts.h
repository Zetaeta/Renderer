#pragma once

template <typename T, bool IsConst>
struct TApplyConst
{
	using Type = T;
};
template <typename T >
struct TApplyConst<T, true>
{
	using Type = const T;
};

template <typename T, bool IsConst>
using ApplyConst = typename TApplyConst<T,IsConst>::Type;


template<typename Derived, typename Base>
concept DerivedStrip = std::derived_from<std::remove_cvref_t<Derived>, Base>;
