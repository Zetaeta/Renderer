#pragma once
#include "core/TypeInfoUtils.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class JsonSerializer
{
public:
	static void Dump(ConstReflectedValue value, std::string file);

	json Serialize(ConstReflectedValue value)
	{
		switch (value.GetType().GetTypeCategory())
		{
			case ETypeCategory::BASIC:
				return SerializeBasic(value);
			case ETypeCategory::CONTAINER:
				return SerializeContainer(value);
			case ETypeCategory::CLASS:
				return SerializeClass(value);
			case ETypeCategory::POINTER:
				return SerializePointer(value);
			default:
				break;
		}
		return json {};
	}

private:
	json SerializeContainer(ConstReflectedValue value);

	json SerializeClass(ConstClassValuePtr value);
	
	json SerializeBasic(ConstReflectedValue value);

	json SerializePointer(ConstReflectedValue value);
};

