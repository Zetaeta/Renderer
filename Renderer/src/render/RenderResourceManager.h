#pragma once
#include "DeviceTexture.h"

class Texture;

namespace rnd
{
class IRenderResourceManager
{
public:
	IRenderResourceManager();
	virtual ~IRenderResourceManager();
	virtual void CreateRenderTexture(Texture* texture) = 0;
	virtual void DestroyRenderTexture(Texture* texture) = 0;
	virtual DeviceTextureRef GetDeviceTexture(const Texture* texture) = 0;


	static DeviceTextureRef GetMainDeviceTexture(const Texture* texture);
	static void CreateRenderTextures(Texture* texture);
	static void DestroyRenderTextures(Texture* texture);

	static void OnDevicesReady();
	static void OnDevicesShutdown();
};

}

