#include "Viewport.h"
#include <render/RenderContext.h>
#include "render/RendererScene.h"
#include "render/RenderController.h"
#include "render/RenderScreenIds.h"
#include "render/GPUSyncPoint.h"
#include "Editor.h"


Viewport::Viewport(rnd::IRenderDeviceCtx* rdc, Scene* scene, Camera::Ref camera, rnd::IDeviceSurface* parentSurface)
	: mScene(scene), mRScene(rnd::RendererScene::Get(*scene, rdc)), mCamera(camera), mDeviceCtx(rdc), mSurface(parentSurface)
{
	rdc->Device->AddViewport(this);
	if (auto ed = Editor::Get())
	{
		ed->mViewports.push_back(this);
	}
	Editor::RegisterViewpoert(this);
}

Viewport::~Viewport()
{
	Editor::UnegisterViewpoert(this);
	if (auto ed = Editor::Get())
	{
		Remove(ed->mViewports, this);
	}
	mDeviceCtx->Device->RemoveViewport(this);
}

ScreenObject* Viewport::GetObjectAt(ivec2 pos)
{
	return nullptr;
}

void Viewport::Resize(u32 width, u32 height, rnd::IDeviceTexture::Ref backbuffer)
{
	float scale = float(std::min(width, height));
	mCamera->SetViewExtent(.5f * width / scale, .5f * height / scale);
	rnd::RenderSettings settings;
	if (mRCtx)
	{
		settings = mRCtx->Settings;
	}
	mRCtx = MakeOwning<rnd::RenderContext>(mDeviceCtx, mRScene, mCamera, backbuffer, settings);
	mWidth = width;
	mHeight = height;
}

void Viewport::SetBackbuffer(rnd::IDeviceTexture::Ref backbuffer)
{
	mRCtx->SetTarget(backbuffer);
}

void Viewport::Reset()
{
	mWidth = mHeight = 0;
	mRCtx = nullptr;
//	mScreenIdTex = nullptr;
}

void Viewport::SetScene(Scene* scene)
{
	mRScene = rnd::RendererScene::Get(*scene, mDeviceCtx);
}

void Viewport::Draw(rnd::IRenderDeviceCtx& ctx)
{
	mRCtx->RenderFrame(ctx);
}

void Viewport::OnClick(ivec2 position)
{
#if HIT_TESTING	
	using namespace rnd;
	MappedResource mapped;
	RefPtr<GPUSyncPoint> syncPt;
	RenderCommand([&mapped, this, &syncPt](IRenderDeviceCtx& ctx)
		{
			DeviceTextureDesc desc;
			desc.DebugName = "ScreenIds";
			desc.Flags = TF_RenderTarget | TF_CpuReadback;
			desc.Format = ETextureFormat::R32_Uint;
			desc.Width = mWidth;
			desc.Height = mHeight;
			auto screenIdTex = mRCtx->GetDevice()->CreateTexture2D(desc);
			mRCtx->RunSinglePass<rnd::RenderScreenIds>(ctx, "ScreenIds", mCamera, screenIdTex->GetRT());
			mapped = ctx.Readback(screenIdTex, 0,&syncPt);
		},
		mRCtx->GetDevice(), "ScreenIds readback");
	FlushRenderCommands(mRCtx->GetDevice());

	syncPt->Wait();

	const u32* textureData = reinterpret_cast<const u32*>(mapped.Data);
	ZE_ASSERT(position.x < int(mWidth));
	ZE_ASSERT(position.y < int(mHeight));
	const u32 id = textureData[position.x + position.y * (mapped.RowPitch / 4)];
//	RenderCommand([toRelease = std::move(mapped)](IRenderDeviceCtx& ctx) {
		mapped.Release();
//	});
	FlushRenderCommands(mRCtx->GetDevice());

	printf("Id: %u\n", id);
	if (ScreenObject* so = Editor::Get()->GetScreenObjManager().GetScreenObj(id))
	{
		so->OnClicked();
	}
#endif
}

void Viewport::DebugPixel(uint2 pixCoord)
{
	mRCtx->DebugPixel(pixCoord);
}
