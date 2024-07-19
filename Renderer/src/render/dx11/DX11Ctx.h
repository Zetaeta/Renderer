#pragma once

#include <d3d11.h>
#include <vector>
#include <assert.h>
#include <array>
#include "core/Maths.h"
#include "core/Types.h"
#include "render/RenderContext.h"
#include <render/RenderDeviceCtx.h>

namespace rnd
{
namespace dx11
{

struct DX11Ctx;
class DX11Renderer;


class DX11RenderTextureManager final : public IRenderTextureManager
{
public:
	DX11RenderTextureManager() { memset(&m_Textures, 0, sizeof(m_Textures)); }
	void SetTexture(ETextureSlot slot, ID3D11ShaderResourceView* tex)
	{
		m_Textures[slot] = tex;
	}

	void SetTexture(ETextureSlot slot, IDeviceTexture::Ref texture) override
	{
		SetTexture(slot, texture->GetTextureHandle<ID3D11ShaderResourceView>());
	}

	void Bind(DX11Ctx const& ctx);
	void UnBind(DX11Ctx const& ctx);


private:

	std::array<ID3D11ShaderResourceView*, E_TS_COUNT> m_Textures; 

	Vector<ID3D11ShaderResourceView*> m_Active;
	Vector<ID3D11ShaderResourceView*> m_Null;
};

//class SRVList : public std::vector<ID3D11ShaderResourceView*>
//{
//public:
//	template<typename TCont>
//	void Push(TCont const& cont)
//	{
//		
//		insert(end(), std::begin(cont), std::end(cont));
//	}
//
//	void Pop(size_type n)
//	{
//		assert(n <= size());
//		resize(size() - n);
//	}
//
//	void Bind(DX11Ctx const& ctx);
//};


struct DX11Ctx
{
	ID3D11Device*		 pDevice;
	ID3D11DeviceContext* pContext;
	DX11RenderTextureManager psTextures;
	DX11Renderer* m_Renderer;		
	rnd::RenderContext* mRCtx;
};

}
}
