#pragma once
#include "core/TypeInfo.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class JsonDeserializer
{
public:
	static bool Read(ReflectedValue value, std::string file);

	void Deserialize(ReflectedValue value, json const& data)
	{
		switch (value.GetType().GetTypeCategory())
		{
			case ETypeCategory::BASIC:
				DeserializeBasic(value, data);
				break;
			case ETypeCategory::CONTAINER:
				DeserializeContainer(value, data);
				break;
			case ETypeCategory::CLASS:
				DeserializeClass(value, data);
				break;
			case ETypeCategory::POINTER:
				DeserializePointer(value, data);
				break;
			default:
				break;
		}
	}

private:
	void DeserializeContainer(ReflectedValue value, json const& data);

	void DeserializeClass(ReflectedValue value, json const& data);
	
	void DeserializeBasic(ReflectedValue value, json const& data);

	void DeserializePointer(ReflectedValue value, json const& data);
};
