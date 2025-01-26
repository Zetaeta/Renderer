#pragma once
#include "common/SceneDataInterface.h"
#include "asset/Mesh.h"
#include "RenderMaterial.h"
#include "MaterialManager.h"

namespace rnd
{
class IRenderDevice;
class IDeviceIndexedMesh;

struct RenderObject
{
	SmallVector<RefPtr<IDeviceIndexedMesh>, 1> MeshData;
	SmallVector<RefPtr<RenderMaterial>, 1> Materials;
	#if HIT_TESTING
	ScreenObjectId ScreenId;
	#endif
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

	static void OnShutdownDevice(IRenderDevice* device);

	RendererScene();
	RendererScene(IRenderDevice* device, Scene const& scene);
	RCOPY_PROTECT(RendererScene);
	RMOVE_DEFAULT(RendererScene);

	~RendererScene();
	void Destroy();

	RenderObject* AddPrimitive(SceneDataInterface::SMCreationData const& data);


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
	}

	const Scene& GetScene() const
	{
		return *mScene;
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

	ScreenObjectId GetScreenObjId(PrimitiveId prim) const
	{
		return mPrimitives.at(prim).ScreenId;
	}

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

	std::unordered_map<PrimitiveId, RenderObject> mPrimitives{}; // TODO optimize
	Scene const* mScene = nullptr;
//	Vector<RenderObject*> mPrimitives;
//	std::unordered_map<MaterialID, RenderMaterial> mMaterials;
};

}
