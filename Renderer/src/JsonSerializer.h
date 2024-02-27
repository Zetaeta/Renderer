#pragma once
#include "TypeInfo.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class JsonSerializer
{
public:
	static void Dump(ConstValuePtr value, std::string file);

	json Serialize(ConstValuePtr value)
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
	json SerializeContainer(ConstValuePtr value);

	json SerializeClass(ConstClassValuePtr value);
	
	json SerializeBasic(ConstValuePtr value);

	json SerializePointer(ConstValuePtr value);
};

