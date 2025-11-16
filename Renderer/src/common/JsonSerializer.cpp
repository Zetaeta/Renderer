#include "JsonSerializer.h"
#include "core/TypeInfoUtils.h"
#include <fstream>
#include "core/PointerTypeInfo.h"

static bool ShouldSerializeType(TypeInfo const& type)
{
	if (type.IsClass())
	{
		return !static_cast<ClassTypeInfo const&>(type).HasAnyClassFlags(EClassFlags::Transient);
	}
	else if (type.GetTypeCategory() == ETypeCategory::POINTER)
	{
		return ShouldSerializeType(static_cast<PointerTypeInfo const&>(type).GetTargetType());
	}
	else
	{
		return true;
	}
}

bool ShouldSerialize(ConstReflectedValue value)
{
	return ShouldSerializeType(value.GetRuntimeType());
}

#define X(PrimType	)\
	JsonSerializer& JsonSerializer::operator<<(PrimType& num)\
	{                                                        \
		Next() = json(num);\
		return *this;\
	}

PRIMITIVE_TYPES

#undef X

JsonSerializer::JsonSerializer()
{
	Stack.push_back({&Root, EStackItemType::Unset});
}

JsonSerializer::~JsonSerializer()
{
	ZE_ASSERT(Stack.empty());
}

void JsonSerializer::Dump(ConstReflectedValue value, std::string file)
{
	JsonSerializer ser;
	json			  j = ser.Serialize(value);
	std::ofstream o(file);
#ifdef _DEBUG
	o << j.dump(2) << std::endl;
#else
	#error hi
	o << j << std::endl;
#endif
	o.close();
}

json const& JsonSerializer::Serialize(ConstReflectedValue value)
{
	value.GetRuntimeType().Serialize(*this, const_cast<void*>(value.GetPtr()));
	return Root;
}

void JsonSerializer::SerializeString(String& str)
{
	Next() = json(str);
}

void JsonSerializer::EnterArray(int& ArrayNum)
{
	auto& entry = Top();
	SetType(entry.Type, EStackItemType::Array);
	*entry.Item = json::array();
}

void JsonSerializer::LeaveArray()
{
	Stack.pop_back();
}

void JsonSerializer::EnterStringMap()
{
	auto& entry = Top();
	SetType(entry.Type, EStackItemType::Map);
	*entry.Item = json::object();
}

void JsonSerializer::SerializeMapKey(std::string_view str)
{
	auto& entry = Top();
	ZE_ASSERT(entry.Type == EStackItemType::Map);
	json& newEntry = (*entry.Item)[str];
	
	Stack.push_back({&newEntry, EStackItemType::Unset});
}

bool JsonSerializer::ReadNextMapKey(std::string_view& key)
{
	ZE_ASSERT(false);
}

void JsonSerializer::EnterPointer(const TypeInfo* innerType)
{
	json& jsonWrapper = Next();
	jsonWrapper["type"] = innerType->GetNameStr();
	json& inner = jsonWrapper["value"];
	Stack.push_back({&inner, EStackItemType::Unset});
}

void JsonSerializer::LeavePointer()
{
}

void JsonSerializer::LeaveStringMap()
{
	ZE_ASSERT(Top().Type == EStackItemType::Map);
	Stack.pop_back();
}

void JsonSerializer::SetType(EStackItemType& inOutType, EStackItemType setType)
{
	ZE_ASSERT(inOutType == EStackItemType::Unset);
	inOutType = setType;
}

JsonSerializer::JsonStackItem& JsonSerializer::Top()
{
	ZE_ASSERT(Stack.size() > 0);

	return Stack[Stack.size() - 1];
}

json& JsonSerializer::Next()
{
	auto& entry = Top();
	if (entry.Type == EStackItemType::Array)
	{
		json& arrayEntry = entry.Item->emplace_back();
		return arrayEntry;
	}
	else
	{
		ZE_ASSERT(entry.Type == EStackItemType::Unset);
		json& value = *entry.Item;
		Stack.pop_back();
		return value;
	}
}

