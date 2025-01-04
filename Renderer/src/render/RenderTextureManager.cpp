#include "RenderTextureManager.h"
#include "RenderDeviceCtx.h"

namespace rnd
{

void RenderTextureManager::Bind(IRenderDeviceCtx* ctx)
{

	m_Active.clear();
	int slot = 0;
	int flag = 1;
	while (slot < E_TS_COUNT)
	{
		if (m_Flags & flag)
		{
			m_Active.push_back(m_Textures[slot]);
		}
		++slot;
		flag <<= 1;
	}
	ctx->SetShaderResources(EShaderType::Pixel, m_Active);
}

void RenderTextureManager::UnBind(IRenderDeviceCtx* ctx)
{
	ctx->SetShaderResources(EShaderType::Pixel, { GetZeroData<ResourceView>(m_Active.capacity()), m_Active.capacity() });
	m_Active.clear();
}

}
