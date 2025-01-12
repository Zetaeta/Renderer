#pragma once

#include "core/WinUtils.h"
#include <d3d11.h>
#include "render/dx11/DX11Texture.h"
#include "render/dx11/DX11Ctx.h"
#include "render/RenderMaterial.h"
#include <scene/Lights.h>
#include <common/Material.h>
namespace rnd
{

using RenderMaterialType = MaterialArchetype;

//struct RenderMaterialType : public MaterialArchetype
//{
//	RenderMaterialType() = default;
//	RenderMaterialType(MaterialArchetypeDesc const& desc);
//	~RenderMaterialType() {}
//
//	RenderMaterialType(RenderMaterialType&&) noexcept = default;
//	RenderMaterialType(RenderMaterialType const&) = delete;
//	RenderMaterialType& operator=(RenderMaterialType&&) noexcept = default;
//
//	virtual void Bind(rnd::RenderContext& rctx, EShadingLayer layer, EMatType matType) override;
//	
//	using Ref = RefPtr<RenderMaterialType>;
//};

}
