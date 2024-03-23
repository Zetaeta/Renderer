#pragma once
#include <vector>
#include <array>
#include "maths.h"
#include "Transform.h"
#include "Texture.h"
#include "Material.h"
#include "Types.h"
#include "Mesh.h"
#include "BaseObject.h"
#include "AssetManager.h"
#include "Lights.h"
#include "SceneObject.h"
#include "SceneComponent.h"

class StaticMeshComponent;

using namespace glm;


struct Sphere
{

	Sphere(vec3 origin, float rad, MaterialID mat)
		: origin(origin), radius(rad), material(mat) {}

	vec3 origin;
	float radius;
	MaterialID material;
	//vec4 colour;
};

/*struct IndexedTri
{

};*/


void ComputeNormals(MeshPart& mesh);



/*class PointLightComponent : SceneComponent
{
	PointLight::Ref m_Light;
};

class SpotLightComponent : SceneComponent
{
	SpotLight::Ref m_Light;
};

class DirLightComponent : SceneComponent
{
	DirLight::Ref m_Light;
};*/ 


struct Scene// : public BaseObject
{
	DECLARE_STI_NOBASE(Scene);
	Scene(AssetManager* assMan)
		: m_AssetManager(assMan)
	{}

	Name MakeName(String base);

	bool IsObjNameTaken(Name name);

	MeshInstanceRef AddMesh(MeshRef mesh, Transform trans = Transform{});

	RCOPY_PROTECT(Scene);

	Scene& operator=(Scene&& other) = default;

	void InsertCompoundMesh(CompoundMesh::Ref cmesh);
	void AddScenelet(Scenelet const& scenelet);
	void AddSceneletPart(StaticMeshComponent& component, SceneletPart const& part);

	void ForEachComponent(std::function<void(SceneComponent&)> const& func );
	void ForEachComponent(std::function<void(SceneComponent const&)> const& func ) const;

	template<typename TComponent>
	void ForEach(std::function<void(TComponent&)> const& func)
	{
		ForEachComponent([&func](SceneComponent& sc)
		{
			if (sc.GetTypeInfo() == TComponent::GetStaticTypeInfo())
			{
				func(static_cast<TComponent&>(sc));
			}
		});
	}

	template<typename TComponent>
	void ForEach(std::function<void(TComponent const&)> const& func) const
	{
		ForEachComponent([&func](SceneComponent const& sc)
		{
			if (sc.GetTypeInfo() == TComponent::GetStaticTypeInfo())
			{
				func(static_cast<TComponent const&>(sc));
			}
		});
	}

	void Initialize();

	AssetManager* GetAssetManager()
	{
		return m_AssetManager;
	}

	AssetManager const* GetAssetManager() const
	{
		return m_AssetManager;
	}

	inline Mesh& GetMesh(MeshRef mr)
	{
		return Meshes()[mr];
	}
	inline Mesh const& GetMesh(MeshRef mr) const
	{
		return Meshes()[mr];
	}

	SceneObject * CreateObject(ClassTypeInfo const& type);
	
	AssetManager* m_AssetManager;
	Material const&			   GetMaterial(MaterialID matId) const { return m_AssetManager->GetMaterial(matId); }
	Material&			   GetMaterial(MaterialID matId) { return m_AssetManager->GetMaterial(matId); }
	std::vector<Material::Ref>&	   Materials() const { return m_AssetManager->m_Materials; }
	auto& CompoundMeshes() const { return m_AssetManager->m_CompoundMeshes; }
	std::vector<MeshInstance>  m_MeshInstances;

	MeshInstance* GetMeshInstance(MeshInstanceRef ref)
	{
		 if (ref < 0)
		 {
			return nullptr;
		 }
		 return &m_MeshInstances[ref];
	}

	std::vector<std::unique_ptr<SceneObject>> m_Objects;

	std::vector<Sphere> m_Spheres;
	std::vector<DirLight> m_DirLights;
	std::vector<PointLight> m_PointLights;
	std::vector<SpotLight> m_SpotLights;

	template<typename T>
	std::vector<T>& GetLights()
	{
		 if constexpr (std::is_same_v<T, DirLight>)
		 {
			return m_DirLights;
		 }
		 if constexpr (std::is_same_v<T, PointLight>)
		 {
			return m_PointLights;
		 }
		 if constexpr (std::is_same_v<T, SpotLight>)
		 {
			return m_SpotLights;
		 }
	}

	
	template<typename T>
	T& GetLight(T::Ref ref)
	{
		 return GetLights<T>()[ref];
	}

	template<typename T>
	typename T::Ref AddLight()
	{
		 auto& lights = GetLights<T>();
		 lights.emplace_back();
		 return lights.size() - 1;
	}

	col3				  m_AmbientLight = col3(0.2f);
private:
	std::vector<Mesh>& Meshes() const { return m_AssetManager->m_Meshes; }
};

DECLARE_CLASS_TYPEINFO(Scene);
