#include "WindowsConfigBackend.h"
#include "Windows.h"


WindowsConfigBackend::WindowsConfigBackend(StringView fileName, StringView appName)
: mFileName(fileName), mAppName(appName)
{
}

constexpr static int sBufferSize = 2048;
static char sBuffer[sBufferSize];

bool WindowsConfigBackend::GetValueString(StringView key, String& outValue)
{
	int chars = GetPrivateProfileStringA(mAppName.c_str(), key.data(), nullptr, sBuffer, sBufferSize, mFileName.c_str());
	if (chars == 0)
	{
		return false;
	}

	outValue = sBuffer;
	return true;
}

void WindowsConfigBackend::SetValueString(StringView key, StringView value)
{
	bool result = WritePrivateProfileStringA(mAppName.c_str(), key.data(), value.data(), mFileName.c_str());
	assert(result);
}
