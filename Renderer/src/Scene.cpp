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


void ComputeNormals(Mesh& mesh)
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

void Scene::InsertCompoundMesh(CompoundMesh const& cmesh)
{
	auto& obj = m_Objects.emplace_back(make_unique<SceneObject>(this,cmesh.name));
	//obj.root
	for (auto& mesh : cmesh.components)
	{
		u32 ind = m_MeshInstances.size();
		m_MeshInstances.emplace_back(mesh.instance, Transform{});
		obj->root->AddChild<MeshComponent>("Child", ind).SetMesh(mesh.instance) ;
	}
}

MeshInstanceRef Scene::AddMesh(MeshRef mesh, Transform trans)
{
	 auto& mi = m_MeshInstances.emplace_back();
	 mi.mesh = mesh;
	 mi.trans = trans;
	 return m_MeshInstances.size() - 1;
}