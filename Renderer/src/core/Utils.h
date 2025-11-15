#pragma once
#include "CoreTypes.h"
#include "core/Types.h"
#include "core/Core.h"
#include "core/Concepts.h"
#include <string>
#include <vector>


template<typename TInt>
class range
{
public:
	class iterator
	{
		friend class range;

	public:
		TInt		operator*() const { return i_; }
		const iterator& operator++()
		{
			++i_;
			return *this;
		}
		iterator operator++(int)
		{
			iterator copy(*this);
			++i_;
			return copy;
		}

		bool operator==(const iterator& other) const { return i_ == other.i_; }
		bool operator!=(const iterator& other) const { return i_ != other.i_; }

	protected:
		iterator(TInt start)
			: i_(start) {}

	private:
		TInt i_;
	};

	iterator begin() const { return begin_; }
	iterator end() const { return end_; }
	range(TInt begin, TInt end)
		: begin_(begin), end_(end) {}

private:
	iterator begin_;
	iterator end_;
};

template <typename TCont>
class BracketAccessed
{
public:
	using size_type = u32;

	template <bool IsConst>
	class Row
	{
	public:
		auto operator[](size_type y) -> decltype(this->m_Cont.at(this->m_x,y)) {
			return m_Cont->at(m_x,y);
		}
	private:
		ApplyConst<TCont, IsConst>* m_Cont;
		size_type m_x;
	};

	template <typename TAccessor, bool IsConst>
	class TemplateRow
	{
	public:
		auto operator[](TAccessor y) -> decltype(this->m_Cont.at(this->m_x,y)) {
			return m_Cont->at(m_x,y);
		}
	private:
		ApplyConst<TCont, IsConst>* m_Cont;
		TAccessor m_x;
	};

	Row<false> operator[](size_type x) {
		return Row<false>{this, x};
	}
	Row<true> operator[](size_type x) const {
		return Row<false>{this, x};
	}

	template<typename TAccessor>
	TemplateRow<TAccessor, false> operator[](TAccessor x) {
		return TemplateRow<TAccessor, false>{this, x};
	}
	template<typename TAccessor>
	TemplateRow<TAccessor, true> operator[](TAccessor x) const {
		return TemplateRow<TAccessor, false>{this, x};
	}
};


template<typename T, typename TContainer = std::vector<std::vector<T>>>
std::vector<T> Flatten(TContainer const& vecs)
{
	std::vector<T> ret;
	for (const auto& v : vecs)
	{
		ret.insert(ret.end(), v.begin(), v.end());
	}
	return ret;
}

std::vector<std::string> Split(const std::string& s, char delim);

template<typename T, size_t N>
struct ArrayHasher
{
	std::size_t operator()(const std::array<T, N>& a) const
	{
		std::size_t h = 0;

		for (auto e : a)
		{
			h ^= std::hash<T>{}(e) + 0x9e3779b9 + (h << 6) + (h >> 2);
		}
		return h;
	}
};

template<typename T>
constexpr auto Addr(T const& t)
{
	return &t;
}

template<typename T>
constexpr auto Addr(T const* t)
{
	return t;
}

template<typename T>
constexpr auto Addr(std::vector<T> const& v)
{
	return v.empty() ? nullptr : &v[0];
}

template<typename T>
constexpr auto Addr(std::vector<T>& v)
{
	return v.empty() ? nullptr : &v[0];
}

template<typename T>
constexpr u32 Size(T const& t)
{
	return static_cast<u32>(std::size(t));
}

template<typename T>
constexpr u32 Sizeof(T const& t)
{
	return static_cast<u32>(sizeof(t));
}

template<typename T>
constexpr u32 Sizeof(std::span<T> s)
{
	return static_cast<u32>(s.size() * sizeof(T));
}

template<typename T>
constexpr u32 Sizeof(std::vector<T> const& v)
{
	return static_cast<u32>(v.size() * sizeof(T));
}

template<typename TStream>
auto& operator<<(TStream& stream, vec3& v)
{
	return stream << '(' << v.x << ',' << v.y << ',' << v.z << ')';
}

#define CHECK(Type, member)   \
	requires(const Type& t) { \
		t.member;\
	}

template<typename TTo, typename TFrom>
TTo& Convert(TFrom const& in, TTo& out)
{
#define TRANSFERVAL(valname)                                    \
{                        \
		constexpr bool a = CHECK(TTo, valname);\
		constexpr bool b = CHECK(TFrom, valname);\
		if constexpr (a && b) \
		{                                                           \
			out.valname = in.valname;\
		}\
	}

	TRANSFERVAL(r);
	TRANSFERVAL(g);
	TRANSFERVAL(b);
	TRANSFERVAL(a);
	TRANSFERVAL(x);
	TRANSFERVAL(y);
	TRANSFERVAL(z);
	TRANSFERVAL(w);
	return out;
}

template<typename TTo, typename TFrom>
TTo Convert(TFrom const& in)
{
	TTo out;
	return Convert(in, out);
}


// https://stackoverflow.com/questions/23999573/convert-a-number-to-a-string-literal-with-constexpr
namespace detail
{
	template <unsigned... digits>
	struct to_chars
	{
		constexpr static const char value[] = { ('0' + digits)..., 0 };
	};

	template <unsigned rem, unsigned... digits>
	struct explode : explode<rem / 10, rem % 10, digits...>
	{
	};

	template <unsigned... digits>
	struct explode<0, digits...> : to_chars<digits...>
	{
	};
} // namespace detail

template <unsigned num>
struct U32ToStr : ::detail::explode<num>
{
};


template <unsigned num>
using U32ToStr_v = U32ToStr<num>::value;

inline bool IsValid(s32 ref)
{
	return ref >= 0;
}

template<typename T>
bool IsValid(const std::weak_ptr<T>& weakPtr)
{
	return !weakPtr.expired();
}

template<typename T>
T* Get(const std::weak_ptr<T>& weakPtr)
{
	if (auto strong = weakPtr.lock())
	{
		return strong.get();
	}

	return nullptr;
}

//template<typename TPtr>
//	requires requires(TPtr p) { *p; }
template<typename T>
inline bool IsValid(T* const ptr)
{
	return ptr != nullptr;
}

template<typename T>
inline bool IsValid(std::shared_ptr<T> const& ptr)
{
	return ptr != nullptr;
}

template<typename T>
inline bool IsValid(std::unique_ptr<T> const& ptr)
{
	return ptr != nullptr;
}



template<typename T>
using RefWrap = std::reference_wrapper<T>;

bool FindIgnoreCase(const std::string_view& haystack, const std::string_view& needle);

#define ARRAY_SIZE(arr) sizeof(arr) / sizeof(arr[0])
#define STR_LEN(str) (ARRAY_SIZE(str) - 1)

#define AT_START(block) static uint8_t sStaticInitializer##__LINE__ = [] { block }();

extern unsigned char const ZerosArray[1024];

template<typename T>
T const* GetZeroData(size_t requiredSize)
{
	ZE_ASSERT(requiredSize * sizeof(T) < sizeof(ZerosArray));
	return reinterpret_cast<T const*>(ZerosArray);
}

template<typename InContainer, typename OutContainer>
void Append(OutContainer& to, InContainer const& from)
{
	to.insert(to.end(), from.begin(), from.end());
}


template<typename InContainer, typename OutContainer>
void AppendMove(OutContainer& to, InContainer&& from)
{
	to.insert(to.end(), std::move_iterator(from.begin()), std::move_iterator(from.end()));
}

template<typename... Args>
struct TMaxSize
{
};
template<typename T, typename... Args>
struct TMaxSize<T, Args...>
{
#ifdef max
#undef max
#endif
	constexpr static u32 size = std::max<u32>(sizeof(T), TMaxSize<Args...>::size);
};

template<>
struct TMaxSize<>
{
	constexpr static u32 size = 0;
};

template<typename... Args>
constexpr u32 MaxSize()
{
	return TMaxSize<Args...>::size;
}

template<typename T>
bool SetWasChanged(T& out, T const& value)
{
	bool wasChanged = out != value;
	out = value;
	return wasChanged;
}

//template<typename... Args>
//using Variant = std::Variant<Args...>;
