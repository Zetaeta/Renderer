
#include "render/dx11/DX11Backend.h"

int main(int argc, char** argv)
{
#ifdef _WIN32
	//if (argc > 1 && strcmp(argv[1], "dx11") == 0)
	{
		return MainDX11(argc, argv);
	}
#endif
}