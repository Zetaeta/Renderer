#pragma once
#include "core/Types.h"

namespace Platform
{
	String GetPlatformError();

	String GetCmdLine();
	char const** ParseCmdLine(const char* CmdLine);

}
