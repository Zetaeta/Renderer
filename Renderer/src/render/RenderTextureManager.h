#pragma once
#include "container/EnumArray.h"
#include "DeviceResource.h"
#include "DeviceTexture.h"

namespace rnd { class IRenderDeviceCtx; }

namespace rnd
{

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

	TextureSlotFlags& Flags() { return m_Flags; }
	void SetFlags(TextureSlotFlags flags) { m_Flags |= flags; }
	void UnsetFlags(TextureSlotFlags flags) { m_Flags &= ~flags; }
	void ClearFlags(TextureSlotFlags mask = 0) { m_Flags &= mask; }

	void SetTexture(ETextureSlot slot, ResourceView tex)
	{
		m_Textures[slot] = tex;
	}

	void SetTexture(ETextureSlot slot, DeviceTextureRef texture)
	{
		SetTexture(slot, ResourceView(texture));
	}

	void Bind(IRenderDeviceCtx* ctx);
	void UnBind(IRenderDeviceCtx* ctx);


protected:

	u64 m_Flags = 0;
	EnumArray<ETextureSlot, ResourceView, E_TS_COUNT> m_Textures; 

	Vector<ResourceView> m_Active;
	//Vector<ID3D11ShaderResourceView*> m_Null;
};

}
