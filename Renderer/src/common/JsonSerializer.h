#pragma once
#include "core/TypeInfoUtils.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;


bool ShouldSerialize(ConstReflectedValue value);

class JsonSerializer
{
public:
	static void Dump(ConstReflectedValue value, std::string file);

	json Serialize(ConstReflectedValue value);

private:
	json SerializeContainer(ConstReflectedValue value);

	json SerializeClass(ConstClassValuePtr value);
	
	json SerializeBasic(ConstReflectedValue value);

	json SerializePointer(ConstReflectedValue value);
};

