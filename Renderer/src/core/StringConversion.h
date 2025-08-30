#pragma once
#include <cstdio>
#include <cstring>

bool FromString(char const* string, int& value);
bool FromString(char const* string, float& value);
bool FromString(char const* string, double& value);
bool FromString(char const* string, bool& value);

inline bool FromString(char const* string, int& value)
{
	return sscanf_s(string, "%d", &value) == 1;
}

inline bool FromString(char const* string, float& value)
{
	return sscanf_s(string, "%f", &value) == 1;
}

inline bool FromString(char const* string, double& value)
{
	return sscanf_s(string, "%lf", &value) == 1;
}

inline bool FromString(char const* string, bool& value)
{
	int intVal;
	if (sscanf_s(string, "%d", &intVal) == 1)
	{
		if (intVal == 0 || intVal == 1)
		{
			value = static_cast<bool>(intVal);
			return true;
		}
		return false;
	}

	if (EqualsIgnoreCase(string, "true") == 0)
	{
		value = true;
		return true;
	}

	if (EqualsIgnoreCase(string, "false") == 0)
	{
		value = false;
		return true;
	}

	return false;
}
