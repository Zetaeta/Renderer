#include "Landscape.h"

#include "asset/Texture.h"
#include "procedural/PerlinNoise.h"
#include <random>
#include "common/Material.h"
#include "render/VertexAttributes.h"
#include "render/RenderContext.h"
#include "render/RendererScene.h"
#include "procedural/ProceduralMeshes.h"

namespace rnd
{
DECLARE_MATERIAL_SHADER(LandscapeMat)

DEFINE_MATERIAL_SHADER(LandscapeMat, "PlainPixelShader", "main", "GPULandscape", "VSMain", VA_TexCoord);

static PerRenderDevice<rnd::MaterialArchetype> sLandscapeMatArch;

class LandscapeMaterial : public RenderMaterial
{
	using Super = RenderMaterial;
public:
	LandscapeMaterial(RefPtr<MaterialArchetype> matType, EMatType opacity, StandardMatProperties const& props, const DeviceTextureRef& heightmap)
		: RenderMaterial(matType, opacity, props), mHeightmap(heightmap)
	{
	}

	void Bind(rnd::RenderContext& rctx, EShadingLayer layer) override
	{
		Super::Bind(rctx, layer);
		rctx.DeviceCtx()->SetShaderResources(EShaderType::Vertex, Single<ResourceView const>({mHeightmap}));
	}

	DeviceTextureRef mHeightmap;
};

class LandscapeDrawable : public rnd::IDrawable
{
	
public:
	LandscapeDrawable(TextureRef heightmap)
		: mHeightmapTextureRef(heightmap)
	{
	}

	void InitResources(IRenderDevice& device)
	{
		auto& matArch = sLandscapeMatArch[device.GetRenderDeviceIndex()];
		if (!matArch.IsValid())
		{
			matArch = MaterialArchetype(LandscapeMat, device);
		}
		mMaterial = MakeOwning<LandscapeMaterial>(&matArch, EMatType::E_MT_OPAQUE, StandardMatProperties{}, device.ResourceMgr.GetDeviceTexture(mHeightmapTextureRef));
		mMesh = ProceduralMeshes::CreateTiledPlane(&device, 0.5f, float2(0), { 10, 10 });
	}
	void DrawDynamic(IPrimitiveDrawer& drawInterface) override
	{
		drawInterface.DrawMesh(mMesh, mMaterial.get());
	}

	TextureRef mHeightmapTextureRef;
	RefPtr<IDeviceMesh> mMesh;
	OwningPtr<LandscapeMaterial> mMaterial;
};

}

Landscape::Landscape(Scene* scene, uvec2 extents /*= {1, 1}*/)
	: SceneObject(scene, "Landscape")
{

	constexpr u32 textureSize = 1024;
	mHeightmap = std::make_shared<Texture>(textureSize, textureSize, "LandscapeHeight", nullptr, ETextureFormat::R32_Float, false);
	float* data = reinterpret_cast<float*>(mHeightmap->GetMutableData());
	std::mt19937 random(0);
//	PerlinNoise2 noise(1.f, 1024, random);
	auto noise = 0.5f * PerlinNoise2(1.f, 256, random)
		+ 0.25f * PerlinNoise2(2.f, 256, random) + 0.125f * PerlinNoise2(4.f, 256, random);
		+ 0.0625f * PerlinNoise2(8.f, 256, random);
	for (int y = 0; y < textureSize; ++y)
	{
		for (int x = 0; x < textureSize; ++x)
		{
			data[y * textureSize + x] = //(
			noise.Evaluate(vec2{x, y} * 4.f/float(textureSize));// + 1)/2;
		}
	}
	mHeightmap->Upload();

	SetRoot<LandscapeComponent>("LandscapeComponent");

//	mMaterial = std::make_shared<LandscapeMaterial>(&mMatArch, EMatType::E_MT_OPAQUE, {}, );
}

bool LandscapeComponent::ShouldAddToScene()
{
	return true;
}

RefPtr<rnd::IDrawable> LandscapeComponent::CreateDrawable()
{
	Landscape* landscape = GetOwner<Landscape>();
	return new rnd::LandscapeDrawable(landscape->mHeightmap);
}

DEFINE_CLASS_TYPEINFO(Landscape)
BEGIN_REFL_PROPS()
END_REFL_PROPS()
END_CLASS_TYPEINFO()

DEFINE_CLASS_TYPEINFO(LandscapeComponent)
BEGIN_REFL_PROPS()
END_REFL_PROPS()
END_CLASS_TYPEINFO()
