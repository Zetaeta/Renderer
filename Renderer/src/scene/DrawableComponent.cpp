#include "DrawableComponent.h"
#include "render/RendererScene.h"



void DrawableComponent::OnInitialize()
{
	Super::OnInitialize();
	if (ShouldAddToScene())
	{
		mPrimId = GetScene().DataInterface().AddPrimitive(this);
	}
}

void DrawableComponent::OnDeinitialize()
{
	if (mPrimId != INVALID_PRIMITIVE)
	{
		GetScene().DataInterface().RemovePrimitive(mPrimId);
		mPrimId = INVALID_PRIMITIVE;
	}
	Super::OnDeinitialize();
}

RefPtr<rnd::IDrawable> DrawableComponent::CreateDrawable()
{
	return nullptr;
}

DEFINE_CLASS_TYPEINFO(DrawableComponent)
BEGIN_REFL_PROPS()
REFL_PROP(CastShadows)
END_REFL_PROPS()
END_CLASS_TYPEINFO()
