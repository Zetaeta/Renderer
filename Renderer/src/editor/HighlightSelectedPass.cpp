#include "HighlightSelectedPass.h"
#include "Editor.h"
#include <scene/SceneComponent.h>
#include <render/RenderDeviceCtx.h>
#include <render/RenderContext.h>
#include <render/dx11/DX11Renderer.h>

namespace rnd
{

void HighlightSelectedPass::RenderFrame(RenderContext& renderCtx)
{
	auto* selectedPrimComp = Cast<PrimitiveComponent>(Editor::Get()->GetSelectedComponent());

	auto& PSPF = static_cast<dx11::DX11Renderer*>(DeviceCtx())->GetPerFramePSCB();

	{
		dx11::PerFramePSData perFrameData;
		Zero(perFrameData);
		perFrameData.ambient = vec3(1);
		perFrameData.debugMode = Denum(mRCtx->ShadingDebugMode);
		perFrameData.debugGrayscaleExp = mRCtx->mDebugGrayscaleExp;
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

	static col4 const HighlightColour(.5f, .5f, 0.f, 0.f);
	ScopedCBOverride colourOverride(*mRCtx, CB::colour, HighlightColour);
	auto material = mRCtx->GetMaterialManager()->GetDefaultMaterial(MAT_PLAIN);
	material->Bind(*mRCtx, EShadingLayer::BASE);
	DeviceCtx()->SetBlendMode(EBlendState::COL_ADD | EBlendState::ALPHA_MAX);
	DeviceCtx()->SetDepthMode(EDepthMode::LESS_EQUAL);
	mRCtx->DrawPrimComp(primComp, material);
}

}
