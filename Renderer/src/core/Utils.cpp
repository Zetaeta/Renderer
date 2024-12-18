#include "core/Utils.h"
#include <sstream>
#include <algorithm>

unsigned char const ZerosArray[1024] = {0};

std::vector<std::string> Split(const std::string& s, char delim)
{
	std::vector<std::string> result;
	std::stringstream		 ss(s);
	std::string				 item;

	while (getline(ss, item, delim))
	{
		result.push_back(item);
	}

	return result;
}

bool FindIgnoreCase(const std::string_view& haystack, const std::string_view& needle)
{
	auto it = std::search(
		haystack.begin(), haystack.end(),
		needle.begin(), needle.end(),
		[](unsigned char ch1, unsigned char ch2) { return std::toupper(ch1) == std::toupper(ch2); });
	return (it != haystack.end());
}
