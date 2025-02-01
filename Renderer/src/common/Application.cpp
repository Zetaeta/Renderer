#include "Application.h"

static bool sExit = false;

bool App::IsExitRequested()
{
	return sExit;
}

void App::RequestExit()
{
	sExit = true;
}
