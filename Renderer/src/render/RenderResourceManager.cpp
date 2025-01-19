#include "RenderResourceManager.h"
#include <algorithm>
#include "container/Vector.h"
#include "asset/Texture.h"
#include "RenderDevice.h"
#include "thread/RWLock.h"

namespace rg = std::ranges;

namespace rnd
{

namespace rrm
{
static Vector<IRenderResourceManager*> ResourceManagers;
static Vector<Texture*> CreateCommands;
static bool AreDevicesReady = false;
RWLock RMLock;
}

IRenderResourceManager::IRenderResourceManager()
{
	auto lock = rrm::RMLock.ScopedWriteLock();
	rrm::ResourceManagers.push_back(this);
}

IRenderResourceManager::~IRenderResourceManager()
{
	auto lock = rrm::RMLock.ScopedWriteLock();
	Remove(rrm::ResourceManagers, this);
}

void IRenderResourceManager::CreateRenderTextures(Texture* texture)
{
	if (!rrm::AreDevicesReady)
	{
		rrm::CreateCommands.push_back(texture);
	}
	else
	{
		for (auto* mgr : rrm::ResourceManagers)
		{
			mgr->CreateRenderTexture(texture);
		}
	}
}

void IRenderResourceManager::DestroyRenderTextures(Texture* texture)
{
	if (!rrm::AreDevicesReady)
	{
		Remove(rrm::CreateCommands, texture);
	}
	else
	{
		for (auto* mgr : rrm::ResourceManagers)
		{
			mgr->DestroyRenderTexture(texture);
		}
	}
}

void IRenderResourceManager::OnDevicesReady()
{
	rrm::AreDevicesReady = true;
	for (IRenderResourceManager* mgr : rrm::ResourceManagers)
	{
		for (Texture* tex : rrm::CreateCommands)
		{
			mgr->CreateRenderTexture(tex);
		}
	}
	rrm::CreateCommands.clear();
}

void IRenderResourceManager::OnDevicesShutdown()
{
	rrm::AreDevicesReady = false;
}

void IRenderResourceManager::UpdateMaterials(MaterialID matId)
{
	auto lock = rrm::RMLock.ScopedReadLock();
	for (auto* mgr : rrm::ResourceManagers)
	{
		mgr->UpdateMaterial(matId);
	}
}

rnd::DeviceTextureRef IRenderResourceManager::GetMainDeviceTexture(const Texture* texture)
{
	return rrm::ResourceManagers[0]->GetDeviceTexture(texture);
}

void RenderResourceMgr::Teardown()
{
	mTextures.clear();
	mDestroyCommands.clear();
	mCreateCommands.clear();
}

// Begin RenderResourceMgr
void RenderResourceMgr::CreateRenderTexture(Texture* texture)
{
	std::scoped_lock lock(mCreateDestroyMtx);
	mCreateCommands.push_back(texture);
}

void RenderResourceMgr::DestroyRenderTexture(Texture* texture)
{
	std::scoped_lock lock(mCreateDestroyMtx);
	if (!Remove(mCreateCommands, texture))
	{
		mDestroyCommands.push_back({texture->Id, GetDeviceTexture(texture)});
		#if !MULTI_RENDER_BACKEND
		texture->SetDeviceTexture(nullptr);
		#endif
		return;
	}

}
DeviceTextureRef RenderResourceMgr::GetDeviceTexture(const Texture* texture)
{
	if (auto it = mTextures.find(texture->Id); it != mTextures.end())
	{
		return it->second;
	}
	return nullptr;
}

void RenderResourceMgr::ProcessTextureCreations()
{
	{
		std::scoped_lock lock(mCreateDestroyMtx);
		for (Texture* tex : mCreateCommands)
		{
			DeviceTextureDesc desc;
			desc.Flags = TF_SRV;
			desc.Width = tex->width;
			desc.Height = tex->height;
			desc.NumMips = 0;
			desc.DebugName = tex->GetName();
			desc.Format = tex->Format;
			if (desc.Format == ETextureFormat::RGBA8_Unorm_SRGB)
			{
				desc.Flags |= TF_SRGB;
			}
			auto			result = mDevice->CreateTexture2D(desc, tex->GetData());
	//		std::make_shared<DX11Texture>(*mCtx, desc, );
		#if !MULTI_RENDER_BACKEND
			tex.SetDeviceTexture(result);
		#endif
			mTextures[tex->Id] = result;
		}
		mCreateCommands.clear();
	#if MULTI_RENDER_BACKEND
		for (auto& destroyCmd : mDestroyCommands)
		{
			mTextures.erase(destroyCmd.TexId);
			destroyCmd.DevTex = nullptr;
		}
	#endif
		mDestroyCommands.clear();
	}

	{
		std::scoped_lock lock(mMaterialUpdateMtx);
		for (auto const& matId : mMaterialUpdates)
		{
			mDevice->MatMgr.UpdateMaterial(matId);
		}
		mMaterialUpdates.clear();

	}
}

void RenderResourceMgr::UpdateMaterial(MaterialID matId)
{
	std::scoped_lock lock(mMaterialUpdateMtx);
	mMaterialUpdates.insert(matId);
}

}
