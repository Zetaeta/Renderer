#include "BufferedRenderInterface.h"



void BufferedRenderInterface::FlipFrameBuffers(u32 from, u32 to)
{
}

std::array<std::binary_semaphore, BufferedRenderInterface::NumSceneDataBuffers> BufferedRenderInterface::sFrameGuards
	{std::binary_semaphore(0), std::binary_semaphore(1)};

u32 BufferedRenderInterface::sMainThreadIdx = 0;

u32 BufferedRenderInterface::sRenderThreadIdx = 0;
