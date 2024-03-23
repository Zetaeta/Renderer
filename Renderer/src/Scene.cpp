#include "Scene.h"
#include <format>
#include "SceneComponent.h"

/* Mesh Mesh::MakeCube(vec3 origin, mat3 rotation, MaterialID mat)
{
	vec3 xax = rotation[0];
	vec3 yax = rotation[1];
	vec3 zax = rotation[2];
	std::vector<vec3> vertices = {origin, origin + xax, origin + yax, origin + xax + yax,
		origin + zax, origin + zax + xax, origin + zax + yax, origin + zax + xax + yax};
	std::vector<IndexedTri> tris;

	return Mesh(vertices, tris,mat);
}*/

using std::vector;


void ComputeNormals(MeshPart& mesh)
{
	vector<Vertex>& vertices = mesh.vertices;
	for (u32 i = 0; i < mesh.vertices.size(); ++i)
	{
		if (mesh.vertices[i].normal != vec3(0))
		{
			continue;
		}
		u32	 count = 0;
		vec3 normal{ 0 };
		for (IndexedTri const& tri : mesh.triangles)
		{
			int ind = -1;
			if (tri[0] == i)
			{
				ind = 0;
			}
			else if (tri[1] == i)
			{
				ind = 1;
			}
			else if (tri[2] == i)
			{
				ind = 2;
			}
			if (ind == -1)
			{
				continue;
			}
			vec3 myContrib =																						   // glm::normalize(
				-glm::cross(vertices[tri[2]].pos - vertices[tri[1]].pos, vertices[tri[1]].pos - vertices[tri[0]].pos); //);
			normal += myContrib;																					   //= (float(count) * normal + myContrib) / float(count + 1);
			count++;
		}
		mesh.vertices[i].normal = (count == 0) ? normal : glm::normalize(normal);
	}
}

Name Scene::MakeName(String base)
{
	if (!IsObjNameTaken(base))
	{
		return base;
	}
	int	   i = 1;
	String outName;
	while (IsObjNameTaken(outName = std::format("{}{}", base.c_str(), i)))
	{
		++i;
	}
	return outName;
}

bool Scene::IsObjNameTaken(Name name)
{
	for (auto const& so : m_Objects)
	{
		if (name == so->m_Name)
		{
			return true;
		}
	}
	return false;
}

void Scene::InsertCompoundMesh(CompoundMesh::Ref cmesh)
{
	auto& obj = m_Objects.emplace_back(make_unique<SceneObject>(this,cmesh->name));
	auto& smc = obj->SetRoot<StaticMeshComponent>();
	smc.SetMesh(cmesh);
	//obj.root
	//for (auto& mesh : cmesh.components)
	//{
	//	//m_MeshInstances.emplace_back(mesh.instance, Transform{});
	//	auto& mc = obj->root->AddChild<StaticMeshComponent>("Child") ;
	//	//mc.SetMesh(mesh.instance);
	//	//mc.SetTransform(mesh.trans);
	//}
}

void Scene::AddScenelet(Scenelet const& scenelet)
{
	auto& obj = m_Objects.emplace_back(make_unique<SceneObject>(this,scenelet.m_Path));
	auto& root = obj->SetRoot<StaticMeshComponent>();
	AddSceneletPart(root, scenelet.m_Root);
}

void Scene::AddSceneletPart(StaticMeshComponent& component, SceneletPart const& part)
{
	component.SetMesh(part.mesh);
	component.SetName(part.mesh->name);
	component.SetTransform(part.trans);
	for (auto const& child : part.children)
	{
		auto& childComp = component.AddChild<StaticMeshComponent>(child.mesh->name);
		AddSceneletPart(childComp, child);
	}
}


MeshInstanceRef Scene::AddMesh(MeshRef mesh, Transform trans)
{
	 auto& mi = m_MeshInstances.emplace_back();
	 mi.mesh = mesh;
	 mi.trans = trans;
	 return m_MeshInstances.size() - 1;
}

void Scene::Initialize()
{
	 for (auto const& obj : m_Objects)
	 {
		obj->SetScene(this);
		obj->Initialize();
	 }
}


SceneObject* Scene::CreateObject(ClassTypeInfo const& type)
{
	auto& ptr = m_Objects.emplace_back();
	auto const& ptrType = static_cast<PointerTypeInfo const&>(::GetTypeInfo(ptr));
	ptrType.New(ValuePtr::From(ptr), type);
	ptr->SetName(MakeName(type.GetName()));
	ptr->SetScene(this);
	ptr->Initialize();
	return ptr.get();
}

template<typename TFunc>
void ForEachCompRecursive(SceneComponent& comp, TFunc&& func)
{
	func(comp);
	for (auto& child : comp.GetChildren())
	{
		if (IsValid(child))
		{
			ForEachCompRecursive(*child, func);
		}
	}
}
void Scene::ForEachComponent(std::function<void(SceneComponent&)> const& func)
{
	for (auto& obj : m_Objects)
	{
		if (IsValid(obj->GetRoot()))
		{
			ForEachCompRecursive(*obj->GetRoot(), func);
		}
	}
}

void Scene::ForEachComponent(std::function<void(SceneComponent const&)> const& func) const
{
	for (auto& obj : m_Objects)
	{
		if (IsValid(obj->GetRoot()))
		{
			ForEachCompRecursive(*obj->GetRoot(), func);
		}
	}
}

DEFINE_CLASS_TYPEINFO(Scene)
BEGIN_REFL_PROPS()
//REFL_PROP(m_DirLights)
//REFL_PROP(m_PointLights)
//REFL_PROP(m_SpotLights)
REFL_PROP(m_Objects)
END_REFL_PROPS()
END_CLASS_TYPEINFO()
