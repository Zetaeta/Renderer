#pragma once
#include "core/TypeInfo.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class JsonDeserializer : public Serializer
{
public:
	JsonDeserializer(const json& jsonVal);
	static bool Read(ReflectedValue value, std::string const& file);

	void Deserialize(ReflectedValue value, json const& data);

	void SerializeString(String& str) override;
	void EnterArray(int& ArrayNum) override;
	void LeaveArray() override;
	void EnterStringMap() override;
	bool ReadNextMapKey(std::string_view& key) override;
	void EnterPointer(const TypeInfo*& innerType) override;
	void LeavePointer() override;
	void LeaveStringMap() override;

#define X(type)\
	virtual JsonDeserializer& operator<<(type& val) override;
	PRIMITIVE_TYPES
#undef X

 private:
	struct StackItem
	{
		json const* Item = nullptr;
		json::const_iterator It = {};
	};
	Vector<StackItem> mStack;

	json const& Next();
	StackItem& Top()
	{
		ZE_ASSERT(!mStack.empty());
		return mStack[mStack.size() - 1];
	}

};
