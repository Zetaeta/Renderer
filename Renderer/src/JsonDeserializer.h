#pragma once
#include "TypeInfo.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class JsonDeserializer
{
public:
	static bool Read(ValuePtr value, std::string file);

	void Deserialize(ValuePtr value, json const& data)
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
	void DeserializeContainer(ValuePtr value, json const& data);

	void DeserializeClass(ValuePtr value, json const& data);
	
	void DeserializeBasic(ValuePtr value, json const& data);

	void DeserializePointer(ValuePtr value, json const& data);
};
