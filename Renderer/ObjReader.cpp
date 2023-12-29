#include "ObjReader.h"
#include <fstream>
#include <sstream>
#include <unordered_map>

using std::vector;
using std::string;
using std::array;


template<typename TStream>
auto& operator>>(TStream& stream, vec3& v)
{
	return stream >> v.x >> v.y >> v.z;
}

template<typename TStream>
TStream& operator>>(TStream& stream, vec2& v)
{
	return stream >> v.x >> v.y;
}


void ComputeNormals(Mesh& mesh) {
	vector<Vertex>& vertices = mesh.vertices;
	for (u32 i=0; i<mesh.vertices.size(); ++i)
	{
		if (mesh.vertices[i].normal != vec3(0))
		{
			continue;
		}
		u32 count = 0;
		vec3 normal{0};
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
			vec3 myContrib = //glm::normalize(
				- glm::cross(vertices[tri[2]].pos - vertices[tri[1]].pos, vertices[tri[1]].pos - vertices[tri[0]].pos);//);
			normal += myContrib;//= (float(count) * normal + myContrib) / float(count + 1);
			count++;
		}
		mesh.vertices[i].normal = (count == 0) ? normal : glm::normalize(normal);
	}
}

vector<Mesh> ObjReader::Parse(string file)
{
	vector<Mesh> meshes;
	//per mesh
	vector<Vertex> vertices;
	vector<vec2> uvs;
	vector<vec3> vPos;
	vector<vec3> normals;
	vector<IndexedTri> faces;
	std::unordered_map<array<u32, 3>, size_t, ArrayHasher<u32, 3>> vertIndsPlus1;
	auto getPos = [&vPos](u32 i) {
		  return i == 0 ? vec3(0) : vPos[i-1];
	};
	auto getNorm = [&normals](u32 i) {
		  return i == 0 ? vec3(0) : normals[i-1];
	};
	auto getUVs = [&uvs](u32 i) {
		  return i == 0 ? vec3(0) : uvs[i-1];
	};
	string objName = file;
	string groupName;
	auto   getName = [&objName, &groupName] {
		  return objName + ((objName.empty() && groupName.empty()) ? "" : ".") + groupName;
	};
	MaterialID  matId;
	
	auto saveMesh = [&] {
		if (faces.empty())
		{
			return;
		}
		Mesh& mesh = meshes.emplace_back(std::move(vertices), std::move(faces), getName() );
		matId = -1;
		ComputeNormals(mesh);
		/* vertices.clear();
		vertIndsPlus1.clear();
		uvs.clear();
		vPos.clear();
		normals.clear();*/
		faces.clear();
	};

	std::ifstream infile(file);
	string line;
	Mtlib mtlib;
	while (std::getline(infile, line))
	{
		if (line[0] == '#')
		{
			continue;
		}
		std::istringstream iss(line);
		string start;
		iss >> start;
		if (start == "v")
		{
			vec3 v;
			iss >> v[0] >> v[1] >> v[2];
			vPos.emplace_back(v);
		}
		else if (start == "vt")
		{
			vec2 uv;
			if (!(iss >> uv.x >> uv.y))
			{
				uv.y = 0;
			}
			uvs.push_back(uv);
		}
		else if (start == "vn")
		{
			vec3 vn;
			iss >> vn[0] >> vn[1] >> vn[2];
			normals.emplace_back(vn);
		}
		else if (start == "f")
		{
			array<string,3> f;
			IndexedTri face;
			iss >> f[0] >> f[1] >> f[2];
			for (int i = 0; i < 3; ++i)
			{
				auto parts = Split(f[i], '/');
				array<u32,3> parsed = {0,0,0}; 
				size_t len = min(size_t(2), parts.size());
				for (int i = 0; i < len; ++i)
				{
					parsed[i] = std::stoi(parts[i]);
				}
				auto& vip1 = vertIndsPlus1[parsed];
				if (vip1 == 0)
				{
					vertices.push_back(Vertex{ getPos(parsed[0]), getNorm(parsed[2]), getUVs(parsed[1]) });
					vip1 = vertices.size();
				}
				assert(vip1 > 0);
				face[i] = vip1 - 1;
			}
			faces.push_back(face);
		}
		else if (start == "o")
		{
			saveMesh();
			iss >> objName;
		}
		else if (start == "g")
		{
			saveMesh();
			iss >> groupName;
		}
		else if (start == "mtllib")
		{
			string filename;
			iss >> filename;
			Mtlib lib = ParseMtl(filename);
			mtlib.insert(mtlib.end(), lib.begin(), lib.end());
		}
		else if (start == "usemtl")
		{
			string matName;
			iss >> matName;
			auto mat = find_if(mtlib.begin(), end(mtlib), [&matName](MtlMat const& mat) {
				return mat.name == matName;
			});
			if (mat != end(mtlib))
			{
				if (mat->loadedMat == -1)
				{
					LoadMat(*mat);
				}
				matId = mat->loadedMat;
			}
		}
	}
	saveMesh();
	return meshes;
}

Mtlib ObjReader::ParseMtl(std::string file)
{
	std::ifstream infile(file);
	string line;
	MtlMat current;
	Mtlib lib;
	while (std::getline(infile, line))
	{
		
		std::istringstream iss(line);
		string start;
		iss >> start;
		if (start == "newmtl")
		{
			if (!current.name.empty())
			{
				lib.push_back(current);
				current = MtlMat {};
			}
			iss >> current.name;
		}
		else if (start == "Ka")
		{
			iss >> current.ambient;
		}
		else if (start == "Kd")
		{
			iss >> current.diffuse;
		}
		else if (start == "Ks")
		{
			iss >> current.specular;
		}
		else if (start == "Ns")
		{
			iss >> current.specExp;
		}
		else if (start == "map_Ka")
		{
			iss >> current.ambientMap;
		}
	}
	if (!current.name.empty())
	{
		lib.push_back(current);
	}
	return lib;
}

MaterialID ObjReader::LoadMat(MtlMat& mat)
{
	
	m_MatDB.emplace_back(vec4(mat.ambient, 1.f), int(mat.specExp),Texture::LoadFrom(mat.ambientMap.c_str()));
	return m_MatDB.size() - 1;
}
