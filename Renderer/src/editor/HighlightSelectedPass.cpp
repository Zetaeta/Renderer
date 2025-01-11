#include "HighlightSelectedPass.h"
#include "Editor.h"
#include <scene/SceneComponent.h>
#include <render/RenderDeviceCtx.h>
#include <render/RenderContext.h>
#include <render/dx11/DX11Renderer.h>
#include "render/ShadingCommon.h"
#include "scene/StaticMeshComponent.h"

namespace rnd
{

void HighlightSelectedPass::Execute(RenderContext& renderCtx)
{
	auto* selectedPrimComp = Cast<PrimitiveComponent>(Editor::Get()->GetSelectedComponent());

	auto& PSPF = *DeviceCtx()->GetConstantBuffer(ECBFrequency::PS_PerFrame);

	{
		PerFramePSData perFrameData;
		Zero(perFrameData);
		perFrameData.ambient = vec3(1);
		perFrameData.debugMode = Denum(mRCtx->Settings.ShadingDebugMode);
		perFrameData.debugGrayscaleExp = mRCtx->Settings.DebugGrayscaleExp;
		perFrameData.brdf = mRCtx->mBrdf;
		PSPF.WriteData(perFrameData);
	}
	if (selectedPrimComp)
	{
		DrawPrimitive(selectedPrimComp);
	}
	else if (SceneObject* selectedObject = Editor::Get()->GetSelectedObject())
	{
		ForEachCompRecursive(*selectedObject->GetRoot(), [this](const SceneComponent& sceneComp)
		{
			if (const PrimitiveComponent* primComp = Cast<const PrimitiveComponent>(&sceneComp); IsValid(primComp))
			{
				DrawPrimitive(primComp);
			}
		});
	}
}

void HighlightSelectedPass::DrawPrimitive(const PrimitiveComponent* primComp)
{
	if (!IsValid(primComp))
	{
		return;
	}

	static col4 const HighlightColour(.1f, .1f, 0.f, 0.f);
	ScopedCBOverride colourOverride(*mRCtx, CB::colour, HighlightColour);
	auto material = mRCtx->GetMaterialManager()->GetDefaultMaterial(MAT_PLAIN);
	material->Bind(*mRCtx, EShadingLayer::BASE);
	DeviceCtx()->SetBlendMode(EBlendState::COL_ADD | EBlendState::ALPHA_MAX);
	DeviceCtx()->SetDepthStencilMode(EDepthMode::LessEqual | EDepthMode::NoWrite);
	mRCtx->DrawPrimComp(primComp, material);
}

}
