#pragma once
#include <tuple>

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
	String Value;
	void Set(bool value)
	{
		Value = value ? "1" : "0";
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

template<typename Enum, size_t NumElements = Enum::Count>
struct TEnumPermutationElement
{
	std::array<Enum, NumElements> EnumVals;
	std::array<char const *> intvals;
	TEnumPermutationElement(std::initializer_list<Enum> enumVals, std::initializer_list<char const*> stringVals)
	:EnumVals(enumVals), StringVals(stringVals)
	{
	}
	String Value;
	void Set(Enum value)
	{
		for (u32 i = 0; i < NumElements; ++i)
		{
			if (EnumVals[i] == value)
			{
				Value = StringVals[i];
				return;
			}
		}
		ZE_ASSERT(false, "Invalid enum permutation");
	}
};

#define PERM_WITH_KEY(key, type) public type {\
	static constexpr char const* Key = key;\
	void ModifyEnv(ShaderCompileEnv& env)\
	{\
		env.SetEnv(Key, Value);\
	}\
}

#define SHADER_PERM_BOOL(key) PERM_WITH_KEY(key, PermutationBool)
#define SHADER_PERM_INT(key) PERM_WITH_KEY(key, TPermutationElement<int>)
#define SHADER_PERM_ENUM(key, enumtype ) PERM_WITH_KEY(key, TSimpleEnumElement<enumtype>)

#define ENUM_PERM_SELECT(key, enumtype, ... ) public TSimpleEnumElement<enumtype> {\
	String Key = key;\
}

template<typename... Elements>
class PermutationDomain :  public IShaderPermutation, public std::tuple<Elements...>
{
	template<typename ElementType, typename ValueType>
	void Set(ValueType&& value)
	{
		std::get<ElementType>(*this).Set(std::forward<ValueType>(value));
	}

	void ModifyCompileEnv(ShaderCompileEnv& env) override
	{
		std::apply([](auto&&... elements)
		{
			(elements.ModifyEnv(env), ...);
//			env.SetEnv(elements.Key, elements.Value);
		}, *this);
	}
};
