#include "core/Utils.h"
#include <sstream>


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
