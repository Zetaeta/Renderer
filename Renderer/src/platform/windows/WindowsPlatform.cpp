#include "platform/Platform.h"
#include "platform/windows/WinApi.h"
namespace Platform
{

String GetPlatformError()
{
	DWORD errorId = ::GetLastError();
	if (errorId == 0)
	{
		return {};
	}

	LPSTR outMessage;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, errorId, LANG_SYSTEM_DEFAULT, reinterpret_cast<LPSTR>(&outMessage), 0, nullptr);
	String result(outMessage, size);
	LocalFree(outMessage);
	return result;
}

String GetCmdLine()
{
	return ::GetCommandLineA();
}

}