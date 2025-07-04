#pragma once
#include "core/CoreTypes.h"
#include "core/Types.h"
#include "container/Map.h"
#include "ConfigTypes.h"
#include "core/StringView.h"
#include "core/StringConversion.h"

class IConfigBackend
{
public:
	virtual bool GetValueString(StringView key, String& outValue) = 0;
	virtual bool GetValueInt(StringView key, int& outValue);
	virtual bool GetValueFloat(StringView key, float& outValue);
	virtual bool GetValueBool(StringView key, bool& outValue);
	virtual void SetValueString(StringView key, StringView value) = 0;
	virtual void SetValueInt(StringView key, int value);
	virtual void SetValueFloat(StringView key, float value);
	virtual void SetValueBool(StringView key, bool value);
};

class ConfigRepository
{
public:
	void Register(StringView name, IConfigVariable* variable);
	void Unregister(StringView name);

	static ConfigRepository& Get();
private:
	ConfigRepository();
	void CreateBackend();

	Map<String, IConfigVariable*> mRegistry;
	OwningPtr<IConfigBackend> mBackend;
	Map<String, String> mCommandLineSpecified;
};

template<typename T>
class ConfigVariableAuto : public IConfigVariable
{
public:
	ConfigVariableAuto(CompileTimeString name, T&& defaultVal, CompileTimeString description = nullptr,
		EConfigVarFlags flags = EConfigVarFlags::Default)
		: mName(name), mValue(std::move(defaultVal))//, mRenderThreadValue(mValue)
		, mFlags(flags)
	{
		ConfigRepository::Get().Register(name, this);
	}

	~ConfigVariableAuto()
	{
		ConfigRepository::Get().Unregister(mName);
	}

public:
	bool SetValueFromString(StringView stringValue) override
	{
		return FromString(stringValue.data(), mValue);
	}

	String GetValueAsString() const override
	{
		return std::to_string(mValue);
	}

	EConfigVarType GetType() const override
	{
		return GetCVType<T>();
	}

	void Set(T&& value)
	{
		mValue = std::move(value);
	}
	void Set(const T& value)
	{
		mValue = value;
	}

	const T& GetValueOnMainThread() const
	{
		return mValue;
	}

	T GetValueOnRenderThread() const
	{
		return mValue;
	}

private:
	CompileTimeString mName;
	T mValue;
//	T mRenderThreadValue;
	EConfigVarFlags mFlags;
};

