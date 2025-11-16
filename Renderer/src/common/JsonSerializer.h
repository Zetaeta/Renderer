#pragma once
#include "core/TypeInfoUtils.h"
#include <nlohmann/json.hpp>
#include "Serializer.h"

using json = nlohmann::json;


bool ShouldSerialize(ConstReflectedValue value);

class JsonSerializer : public Serializer
{
public:

	JsonSerializer();
	~JsonSerializer();
	static void Dump(ConstReflectedValue value, std::string file);

	json const& Serialize(ConstReflectedValue value);

	void SerializeString(String& str) override;
	void EnterArray(int& ArrayNum) override;
	void LeaveArray() override;
	void EnterStringMap() override;
	void SerializeMapKey(std::string_view str) override;
	bool ReadNextMapKey(std::string_view& key) override;
	void EnterPointer(const TypeInfo*& innerType) override;
	void LeavePointer() override;
	void LeaveStringMap() override;

#define X(type)\
	virtual JsonSerializer& operator<<(type& val) override;
	PRIMITIVE_TYPES
#undef X


 private:
protected:
	enum class EStackItemType : u8
	{
		Unset,
		Map,
		Array,
		Value
	};
	struct StackItem
	{
		json* Item = nullptr;
		EStackItemType Type = EStackItemType::Unset;
	};

	void SetType(EStackItemType& inOutType, EStackItemType setType);

	StackItem& Top();
	json& Next();

	Vector<StackItem> Stack;
	json Root;
};

