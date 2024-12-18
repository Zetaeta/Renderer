#pragma once
#include <tuple>

namespace rnd
{

struct PermutationElement
{
	String Key;
	String Value;
};

template<typename T>
struct TPermutationElement
{
	String Value;
	void Set(T&& value)
	{
		Value = ToString(value);
	}
};

struct PermutationBool
{
	bool Value = false;
	void Set(bool value)
	{
		Value = value;
	}

	const char* GetValue() const
	{
		return Value ? "1" : "0";
	}

	constexpr static u32 NumBits()
	{
		return 1;
	}

	u64 EncodeBits() const
	{
		return Value;
	}
};

template<typename T>
struct TSimpleEnumElement
{
	String Value;
	String Key;
	void Set(T&& value)
	{
		Value = ToString(Denum(value));
	}
};

//template<typename Enum, size_t NumElements = Enum::Count>
//struct TEnumPermutationElement
//{
//	std::array<Enum, NumElements> EnumVals;
//	std::array<char const *, NumElements> intvals;
//	TEnumPermutationElement(std::initializer_list<Enum> enumVals, std::initializer_list<char const*> stringVals)
//	:EnumVals(enumVals), StringVals(stringVals)
//	{
//	}
//	String Value;
//	void Set(Enum value)
//	{
//		for (u32 i = 0; i < NumElements; ++i)
//		{
//			if (EnumVals[i] == value)
//			{
//				Value = StringVals[i];
//				return;
//			}
//		}
//		ZE_ASSERT(false, "Invalid enum permutation");
//	}
//};

#define PERM_WITH_KEY(key, type) public type {\
	static constexpr char const* Key = key;\
	void ModifyEnv(ShaderCompileEnv& env) const\
	{\
		env.SetEnv(Key, GetValue());\
	}\
}

#define SHADER_PERM_BOOL(key) PERM_WITH_KEY(key, PermutationBool)
#define SHADER_PERM_INT(key) PERM_WITH_KEY(key, TPermutationElement<int>)
#define SHADER_PERM_ENUM(key, enumtype ) PERM_WITH_KEY(key, TSimpleEnumElement<enumtype>)

#define ENUM_PERM_SELECT(key, enumtype, ... ) public TSimpleEnumElement<enumtype> {\
	String Key = key;\
}

template<typename... Elements>
struct PermutationDomain : public IShaderPermutation, public std::tuple<Elements...>
{
	PermutationDomain() {}

	template<typename ElementType, typename ValueType>
	void Set(ValueType&& value)
	{
		std::get<ElementType>(*this).Set(std::forward<ValueType>(value));
	}

	template<typename ElementType>
	ElementType const& Get() const
	{
		return std::get<ElementType>(*this);
	}

	void ModifyCompileEnv(ShaderCompileEnv& env) const override
	{
		std::apply([&](auto&&... elements)
		{
			(elements.ModifyEnv(env), ...);
//			env.SetEnv(elements.Key, elements.Value);
		}, static_cast<const std::tuple<Elements...>&>(*this));
	}

	u64 GetUniqueId() const override
	{
		u64 result = 0;
		std::apply([&](auto&&... elements)
		{
			u32 currentBit = 0;
			auto loop = [&](auto&& element)
			{
				ZE_ASSERT(currentBit + element.NumBits() < 64);
				result |= (element.EncodeBits() << currentBit);
				currentBit += element.NumBits();
			};
			(loop(elements), ...);
		}, static_cast<const std::tuple<Elements...>&>(*this));

		return result;
	}
};

}
