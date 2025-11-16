#include "JsonDeserializer.h"
#include "core/TypeInfoUtils.h"
#include <fstream>



JsonDeserializer::JsonDeserializer(const json& jsonVal)
	:Serializer(ESerializerFlags::Loading)
{
	mStack.push_back({&jsonVal});
}

bool JsonDeserializer::Read(ReflectedValue value, std::string const& file)
{
	std::ifstream i(file);
	if (!i) return false;
	json j;
	i >> j;
	JsonDeserializer jd(j);
	jd.Deserialize(value, j);
	return true;
}

void JsonDeserializer::Deserialize(ReflectedValue value, json const& data)
{
	value.GetRuntimeType().Serialize(*this, const_cast<void*>(value.GetPtr()));
}

void JsonDeserializer::SerializeString(String& str)
{
	str = Next();
}


#define X(PrimType	)\
	JsonDeserializer& JsonDeserializer::operator<<(PrimType& num)\
	{                                                        \
		num = Next();\
		return *this;\
	}

PRIMITIVE_TYPES

#undef X

void JsonDeserializer::EnterArray(int& ArrayNum)
{
	auto& entry = Top();
	if (ZE_ENSURE(entry.Item->is_array()))
	{
		ArrayNum = NumCast<int>(entry.Item->size());
		entry.It = entry.Item->begin();
	}
	else
	{
		ArrayNum = 0;
	}
}

void JsonDeserializer::LeaveArray()
{
	ZE_ASSERT(Top().Item->is_array());
	mStack.pop_back();
}

void JsonDeserializer::EnterStringMap()
{
	auto& entry = Top();
	ZE_ENSURE(entry.Item->is_object());
	entry.It = entry.Item->begin();
}

bool JsonDeserializer::ReadNextMapKey(std::string_view& key)
{
	auto& entry = Top();
	if (entry.It == entry.Item->end())
	{
		return false;
	}
	else
	{
		key = entry.It.key();
		const json& val = entry.It.value();
		entry.It++;
		mStack.push_back({&val});
		return true;
	}
}

void JsonDeserializer::EnterPointer(TypeInfo const*& innerType)
{
	auto& wrapper = Next();
	auto it = wrapper.find("type");
	auto* type = FindTypeInfo(std::string_view(*it));
	if (type && type != &NullTypeInfo())
	{
		innerType = type;
		ZE_ASSERT(it != wrapper.end());
		mStack.push_back({&*wrapper.find("value")});
	}
	else
	{
		innerType = &NullTypeInfo();
	}
}

void JsonDeserializer::LeavePointer()
{
}

void JsonDeserializer::LeaveStringMap()
{
	ZE_ASSERT(Top().Item->is_object());
	mStack.pop_back();
}

json const& JsonDeserializer::Next()
{
	auto& entry = Top();
	if (entry.Item->is_array())
	{
		ZE_ASSERT(entry.It != entry.Item->end());
		return *(entry.It++);
		//ZE_ASSERT(entry.Item->is_array() && entry.ArrayIndex < entry.Item->size());
		//return (*entry.Item)[entry.ArrayIndex++];
	}
	else
	{
//		ZE_ASSERT(entry.Type == EStackItemType::Unset);
		json const& value = *entry.Item;
		mStack.pop_back();
		return value;
	}
}
