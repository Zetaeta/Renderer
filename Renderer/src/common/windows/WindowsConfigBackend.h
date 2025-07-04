#pragma once
#include "common/Config.h"

class WindowsConfigBackend : public IConfigBackend
{
public:
	WindowsConfigBackend(StringView fileName, StringView appName);
	bool GetValueString(StringView key, String& outValue) override;

	void SetValueString(StringView key, StringView value) override;

	String mFileName;
	String mAppName;
};
