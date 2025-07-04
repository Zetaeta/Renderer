#include "StringView.h"



Vector<StringView> Split(StringView sv, char delim)
{
	Vector<StringView> result;
	u32				   start = 0;
	for (u32 i = 0; i < sv.size(); ++i)
	{
		if (sv[i] == delim)
		{
			result.push_back(StringView(sv.data() + start, i - start));
			start = i + 1;
		}
	}
	if (start < sv.size())
	{
		result.push_back(StringView(sv.data() + start, sv.size() - start));
	}
	return result;
}
