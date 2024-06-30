#pragma once

#include <d3d11.h>
#include <vector>
#include <assert.h>
#include <array>
#include "core/Maths.h"
#include "core/Types.h"
#include "render/RenderContext.h"

struct DX11Ctx;
class DX11Renderer;

enum ETextureSlot : u8
{
	E_TS_DIFFUSE = 0,
	E_TS_NORMAL,
	E_TS_ROUGHNESS,
	E_TS_EMISSIVE,
	E_TS_SHADOWMAP,
	E_TS_ENVIRONMENT,
	E_TS_COUNT
};

using TextureSlotFlags = u64;

constexpr TextureSlotFlags 
	F_TS_DIFFUSE = 1 << E_TS_DIFFUSE,
	F_TS_NORMAL = 1 << E_TS_NORMAL,
	F_TS_ROUGHNESS = 1 << E_TS_ROUGHNESS,
	F_TS_EMISSIVE = 1 << E_TS_EMISSIVE,
	F_TS_SHADOWMAP = 1 << E_TS_SHADOWMAP,
	F_TS_ENVIRONMENT = 1 << E_TS_ENVIRONMENT;

class RenderTextureManager
{
public:
	RenderTextureManager() { memset(&m_Textures, 0, sizeof(m_Textures)); }
	void SetTexture(ETextureSlot slot, ID3D11ShaderResourceView* tex)
	{
		m_Textures[slot] = tex;
	}

	void Bind(DX11Ctx const& ctx);
	void UnBind(DX11Ctx const& ctx);


	TextureSlotFlags& Flags() { return m_Flags; }

	void SetFlags(TextureSlotFlags flags) { m_Flags |= flags; }

	void UnsetFlags(TextureSlotFlags flags) { m_Flags &= ~flags; }

	void ClearFlags(TextureSlotFlags mask = 0) { m_Flags &= mask; }
	
private:

	std::array<ID3D11ShaderResourceView*, E_TS_COUNT> m_Textures; 

	Vector<ID3D11ShaderResourceView*> m_Active;
	Vector<ID3D11ShaderResourceView*> m_Null;

	u64 m_Flags = 0;
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
	RenderTextureManager psTextures;
	DX11Renderer* m_Renderer;		
	rnd::RenderContext* mRCtx;
};
