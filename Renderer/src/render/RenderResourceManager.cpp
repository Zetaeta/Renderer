#include "RenderResourceManager.h"
#include <algorithm>
#include "container/Vector.h"

namespace rg = std::ranges;

namespace rnd
{

namespace rrm
{
static Vector<IRenderResourceManager*> ResourceManagers;
static Vector<Texture*> CreateCommands;
static bool AreDevicesReady = false;
}

IRenderResourceManager::IRenderResourceManager()
{
	rrm::ResourceManagers.push_back(this);
}

IRenderResourceManager::~IRenderResourceManager()
{
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

rnd::DeviceTextureRef IRenderResourceManager::GetMainDeviceTexture(const Texture* texture)
{
	return rrm::ResourceManagers[0]->GetDeviceTexture(texture);
}

}
