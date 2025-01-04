#pragma once
#include "common/Material.h"

namespace rnd
{

class RenderMaterial : public IDeviceMaterial
{
public:
	RenderMaterial(RefPtr<MaterialArchetype> matType, EMatType opacity)
		: IDeviceMaterial(matType), mMatType(opacity) {}



	virtual void UnBind(rnd::RenderContext& rctx)
	{
	}

	void Bind(rnd::RenderContext& rctx, EShadingLayer layer) override;
protected:
	EMatType mMatType;
};

class TexturedRenderMaterial : public RenderMaterial
{
public:

	TexturedRenderMaterial(RefPtr<MaterialArchetype> matType, EMatType opacity, DeviceTextureRef const& albedo = nullptr)
		: RenderMaterial(matType, opacity), m_Albedo(albedo) {}

	
	void Bind(rnd::RenderContext& rctx, EShadingLayer layer) override;
	void		 UnBind(rnd::RenderContext& rctx) override {}

	DeviceTextureRef m_Albedo;
	DeviceTextureRef m_Normal;
	DeviceTextureRef m_Emissive;
	DeviceTextureRef m_Roughness;
};


}
