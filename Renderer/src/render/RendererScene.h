#pragma once
#include "common/SceneDataInterface.h"
#include "asset/Mesh.h"
#include "RenderMaterial.h"
#include "MaterialManager.h"

namespace rnd
{
class IRenderDevice;
class IDeviceIndexedMesh;

struct StaticMeshDrawable
{
	SmallVector<RefPtr<IDeviceIndexedMesh>, 1> MeshData;
	SmallVector<RefPtr<RenderMaterial>, 1> Materials;
#if HIT_TESTING
	ScreenObjectId ScreenId;
#endif
};

class IPrimitiveDrawer
{
public:
	virtual void DrawMesh(IDeviceMesh* mesh, RenderMaterial* material) = 0;
};

class IDrawable;

class SimplePrimitiveRecorder : public IPrimitiveDrawer
{
public:
	void DrawMesh(IDeviceMesh* mesh, RenderMaterial* material) override
	{
		Meshes.push_back(mesh);
		Materials.push_back(material);
	}

	Vector<IDeviceMesh*> Meshes;
	Vector<RenderMaterial*> Materials;
};

/**
 * Responsible for scene data used by the render thread(s).
 * In case of multiple render backends, each has its own scene.
 */
class RendererScene
{
public:
	static RendererScene* Get(Scene const& scene, IRenderDeviceCtx* deviceCtx);

	static void DestroyScene(Scene const& scene);
	static void InitializeScene(Scene const& scene);

	static void OnShutdownDevice(IRenderDevice* device);

	RendererScene();
	RendererScene(IRenderDevice* device, Scene const& scene);
	ZE_COPY_PROTECT(RendererScene);
	RMOVE_DEFAULT(RendererScene);

	~RendererScene();
	void Destroy();

	StaticMeshDrawable* AddPrimitive(SceneDataInterface::SMCreationData const& data);

	void AddCustomDrawable(SceneDataInterface::CustomDrawableCreationData const& data);
	void RemoveCustomPrimitive(PrimitiveId primId);

	template<typename Func>
	void ForEachPrimitive(Func&& func) const
	{
		for (auto const& [id, obj] : mPrimitives)
		{
			func(id, obj);
		}
	}

	template<typename Func>
	void ForEachMeshPart(Func&& func) const
	{
		for (auto const& [id, obj] : mPrimitives)
		{
			for (u32 i = 0; i < obj.MeshData.size(); ++i)
			{
				func(id, obj.MeshData[i], obj.Materials[i]);
			}
		}

		for (const auto& [id, customPrim] : mCustomPrimitives)
		{
			SimplePrimitiveRecorder customPrimRecorder;
			customPrim->DrawDynamic(customPrimRecorder);

			for (u32 i = 0; i < customPrimRecorder.Meshes.size(); ++i)
			{
				func(id, customPrimRecorder.Meshes[i], customPrimRecorder.Materials[i]);
			}
		}
	}

	const Scene& GetScene() const
	{
		return *mScene;
	}

	const PrimitiveInfo& GetPrimInfo(PrimitiveId id) const
	{
		return mPrimInfos[id];
	}

	void UpdatePrimitives();

	// Light functions redirect to Scene for now
	std::variant<DirLight const*, PointLight const*, SpotLight const*> GetLight(ELightType lightType, u32 lightIdx) const
	{
		return mScene->GetLight(lightType, lightIdx);
	}
	std::vector<DirLight> const& GetDirLights() const
	{
		return mScene->m_DirLights;
	}
	std::vector<PointLight> const& GetPointLights() const
	{
		return mScene->m_PointLights;
	}
	std::vector<SpotLight> const& GetSpotLights() const
	{
		return mScene->m_SpotLights;
	}

	col3 GetAmbientLight() const
	{
		return mScene->m_AmbientLight;
	}

	template<typename T>
	const std::vector<T>& GetLights() const
	{
		return mScene->GetLights<T>();
	}

	
	template<typename T>
	T const& GetLight(T::Ref ref)
	{
		 return GetLights<T>()[ref];
	}

	template<typename T>
	const T& GetLight(T::Ref ref) const
	{
		 return GetLights<T>()[ref];
	}

	const LightData* GetLightData(ELightType lightType, u32 lightIdx) const
	{
		return mScene->GetLightData(lightType, lightIdx);
	}

	const WorldTransform& GetLightTransform(ELightType lightType, u32 lightIdx) const
	{
		return GetLightData(lightType, lightIdx)->GetSceneComponent()->GetWorldTransform();
	}
	
	SceneDataInterface& DataInterface() const
	{
		return *mDataInterface;
	}

	ScreenObjectId GetScreenObjId(PrimitiveId prim) const;

	bool IsSelected(PrimitiveId prim) const
	{
		return mDataInterface->GetRenderThreadData().Selected[prim];
	}

	WorldTransform const& GetPrimTransform(PrimitiveId prim) const
	{
		return mDataInterface->GetRenderThreadData().Transforms[prim];
	}

	bool IsInitialized() const
	{
		return mInitialized;
	}

	void BeginFrame();
	void EndFrame();
private:

	IRenderDevice* mDevice = nullptr;
	SceneDataInterface* mDataInterface = nullptr;
	bool mInitialized = false;

	Vector<PrimitiveInfo> mPrimInfos;

	std::unordered_map<PrimitiveId, StaticMeshDrawable> mPrimitives{}; // TODO optimize
	std::unordered_map<PrimitiveId, RefPtr<IDrawable>> mCustomPrimitives; // For non-trivial static meshes.

	Scene const* mScene = nullptr;
//	Vector<RenderObject*> mPrimitives;
//	std::unordered_map<MaterialID, RenderMaterial> mMaterials;
};

}
