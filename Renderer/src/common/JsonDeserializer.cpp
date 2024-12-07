#include "JsonDeserializer.h"
#include "core/TypeInfoUtils.h"
#include <fstream>



bool JsonDeserializer::Read(ReflectedValue value, std::string const& file)
{
	std::ifstream i(file);
	if (!i) return false;
	JsonDeserializer jd;
	json j;
	i >> j;
	jd.Deserialize(value, j);
	return true;
}

void JsonDeserializer::DeserializeContainer(ReflectedValue value, json const& data)
{
	ContainerTypeInfo const& typ = static_cast<ContainerTypeInfo const&>(value.GetType());
	auto acc = typ.CreateAccessor(value);
	if (typ.HasFlag(ContainerTypeInfo::RESIZABLE))
	{
		acc->Resize(data.size());
	}
	else
	{
		ZE_ASSERT(acc->GetSize() == data.size());
	}
	for (int i = 0; i < acc->GetSize(); ++i)
	{
		Deserialize(acc->GetAt(i), data[i]);
		
	}
	
}

void JsonDeserializer::DeserializeClass(ReflectedValue value, json const& data)
{
	ClassTypeInfo const& type = static_cast<ClassTypeInfo const&>(value.GetRuntimeType());
	type.ForEachProperty([this, value, &data] (PropertyInfo const& prop)
	{
		auto it = data.find(prop.GetName());
		if (it != data.end())
		{
			Deserialize(prop.Access(value), *it);
		}
	});
}

void JsonDeserializer::DeserializeBasic(ReflectedValue value, json const& data)
{
	auto const& typ = value.GetType();
#define X(basic) if (typ == GetTypeInfo<basic>())\
	{                                \
		value.GetAs<basic>() = data.get<basic>(); \
	}
	BASIC_TYPES
#undef X
}

void JsonDeserializer::DeserializePointer(ReflectedValue value, json const& data)
{
	if (data == json{})
	{
		return;
	}
	TypeInfo const* type = FindTypeInfo(data["type"]);
	if (type == nullptr)
	{
		return;
	}
	PointerTypeInfo const& typ = static_cast<PointerTypeInfo const&>(value.GetType());
	typ.New(value, *type);
	Deserialize(typ.Get(value).Downcast(), data["value"]);
}
