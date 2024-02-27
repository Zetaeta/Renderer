#include "JsonSerializer.h"
#include "ContainerTypeInfo.h"
#include "PointerTypeInfo.h"
#include <fstream>



void JsonSerializer::Dump(ConstValuePtr value, std::string file)
{
	JsonSerializer ser;
	json			  j = ser.Serialize(value);
	std::ofstream o(file);
	o << j << std::endl;
	o.close();
}

json JsonSerializer::SerializeContainer(ConstValuePtr value)
{
	json					 j = json::array();
	ContainerTypeInfo const& typ = static_cast<ContainerTypeInfo const&>(value.GetType());
	auto accessor = typ.CreateAccessor(value);
	size_t size = accessor->GetSize();
	for (int i = 0; i < size; ++i)
	{
		j.push_back(Serialize(accessor->GetAt(i)));
	}
	return j;
}

json JsonSerializer::SerializeClass(ConstClassValuePtr value)
{
	json j = json::object();
	ConstClassValuePtr actual = value.Downcast();
	actual.GetType().ForEachProperty([this, actual, &j](PropertyInfo const* prop)
	{
		j[prop->GetName()] = Serialize(prop->Access(actual));
	});
	return j;
}

json JsonSerializer::SerializeBasic(ConstValuePtr value)
{
	TypeInfo const& typ = value.GetType();
#define X(Type)                        \
	if (typ == GetTypeInfo<Type>())       \
	{                                     \
		return json(value.GetAs<Type>()); \
	}
	BASIC_TYPES
#undef X
	//CASE(int);
	//CASE(bool);
	//CASE(double);
	//CASE(float);
	//CASE(bool);
	//CASE(u32);
	//CASE(u64);
	//CASE(s64);
	//CASE(s8);
	//CASE(u8);
	//CASE(u16);
	//CASE(s16);
	return json{};
	// if (typ == GetTypeInfo<int>())
	//{
	//	return json (value.GetAs<int>());
	// }
}

json JsonSerializer::SerializePointer(ConstValuePtr value)
{
	json j;
	PointerTypeInfo const& typ = static_cast<PointerTypeInfo const&>(value.GetType());

	if (typ.IsNull(value))
	{
		j["type"] = "nullptr";
		j["value"] = {};
	}
	else
	{
		auto target = typ.GetConst(value);
		j["type"] = target.GetRuntimeType().GetName();
		j["value"] = Serialize(target);
	}

	return j;
}
