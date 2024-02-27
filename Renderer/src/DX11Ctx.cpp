#include "DX11Ctx.h"
#include "Utils.h"

//void SRVList::Bind(DX11Ctx const& ctx)
//{
//	ctx.pContext->PSSetShaderResources(0, u32(size()), &front());
//}


void RenderTextureManager::Bind(DX11Ctx const& ctx)
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
	ctx.pContext->PSSetShaderResources(0,m_Active.size(), Addr(m_Active));
	m_Null.resize(m_Active.capacity() - m_Active.size());
	ctx.pContext->PSSetShaderResources(m_Active.size(), m_Null.size(), Addr(m_Null));
}

void RenderTextureManager::UnBind(DX11Ctx const& ctx)
{
	m_Null.resize(m_Active.capacity());
	ctx.pContext->PSSetShaderResources(0, m_Null.size(), Addr(m_Null));
}
