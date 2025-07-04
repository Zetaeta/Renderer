#include "Config.h"
#include "core/StringConversion.h"
#include "CommandLine.h"
#ifdef _WIN32
#include "windows/WindowsConfigBackend.h"
#endif


void ConfigRepository::Register(StringView name, IConfigVariable* variable)
{
	auto& entry = mRegistry[String(name)];
	assert(entry == nullptr);
	entry = variable;
	String value;
	bool isSet = mBackend->GetValueString(name, value);
	if (auto it = mCommandLineSpecified.find(String(name)); it != mCommandLineSpecified.end())
	{
		value = it->second;
		isSet = true;
	}

	if (isSet)
	{
		variable->SetValueFromString(value);
	}
}

void ConfigRepository::Unregister(StringView name)
{
	mRegistry.erase(String(name));
}

ConfigRepository& ConfigRepository::Get()
{
	static ConfigRepository singleton;
	return singleton;
}

ConfigRepository::ConfigRepository()
{
	CreateBackend();

	Vector<String> entries = CommandLine::Get().GetValues("config");
	for (const String& entry : entries)
	{
		Vector<StringView> items = Split(StringView(entry), ',');
		for (const StringView& item : items)
		{
			size_t splitOn = item.find('=');
			if (splitOn < item.size())
			{
				String key(item.substr(0, splitOn));
				String value(item.substr(splitOn + 1));
				mCommandLineSpecified[std::move(key)] = std::move(value);
			}
		}
	}
}

void ConfigRepository::CreateBackend()
{
#ifdef _WIN32
	mBackend = MakeOwning<WindowsConfigBackend>("renderer.ini", "renderer");
#endif
}

bool IConfigBackend::GetValueInt(StringView key, int& outValue)
{
	String strVal;
	return GetValueString(key, strVal) && FromString(strVal.c_str(), outValue);
}

bool IConfigBackend::GetValueFloat(StringView key, float& outValue)
{
	String strVal;
	return GetValueString(key, strVal) && FromString(strVal.c_str(), outValue);
}

bool IConfigBackend::GetValueBool(StringView key, bool& outValue)
{
	String strVal;
	return GetValueString(key, strVal) && FromString(strVal.c_str(), outValue);
}

void IConfigBackend::SetValueInt(StringView key, int value)
{
	SetValueString(key, std::to_string(value));
}

void IConfigBackend::SetValueFloat(StringView key, float value)
{
	SetValueString(key, std::to_string(value));
}

void IConfigBackend::SetValueBool(StringView key, bool value)
{
	SetValueString(key, std::to_string(static_cast<int>(value)));
}
