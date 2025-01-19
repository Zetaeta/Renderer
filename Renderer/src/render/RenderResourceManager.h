#pragma once
#include "DeviceTexture.h"
#include "unordered_map"
#include "mutex"
#include "RndFwd.h"
#include "unordered_set"

class Texture;
using TextureId = u64;

template<typename T>
using SmallSet = std::unordered_set<T>;

namespace rnd
{
class IRenderDevice;

// Responsible for sending 
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

	static void UpdateMaterials(MaterialID matId);
	virtual void UpdateMaterial(MaterialID matId) = 0;
};

// Standard implementation
class RenderResourceMgr : public IRenderResourceManager
{
public:
	RenderResourceMgr(IRenderDevice* device)
	:mDevice(device) { }


	void Teardown();
	void CreateRenderTexture(Texture* texture) override;
	virtual void DestroyRenderTexture(Texture* texture) override;
	DeviceTextureRef GetDeviceTexture(const Texture* texture) override
#if MULTI_RENDER_BACKEND
	;
#else
	{
		return texture->GetDeviceTexture();
	}
#endif

	void ProcessTextureCreations();
	void UpdateMaterial(MaterialID matId);
private:
	struct TexDestroyCommand
	{
		TextureId TexId;
		DeviceTextureRef DevTex;
	};
	IRenderDevice* mDevice;
	std::unordered_map<TextureId, DeviceTextureRef> mTextures;
	Vector<TexDestroyCommand> mDestroyCommands;
	Vector<class Texture*> mCreateCommands;
	std::mutex mCreateDestroyMtx;
	SmallSet<MaterialID> mMaterialUpdates;
	std::mutex mMaterialUpdateMtx;
};

}

