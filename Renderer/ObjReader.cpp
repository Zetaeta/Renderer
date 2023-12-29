#include "ObjReader.h"
#include <fstream>
#include <sstream>

using std::vector;

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

Mesh ObjReader::Parse(string file)
{
	std::ifstream infile(file);
	vector<Vertex> vertices;
	vector<IndexedTri> faces;
	string line;
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
			vertices.emplace_back(v);
		}
		else if (start == "f")
		{
			IndexedTri face;
			iss >> face[0] >> face[1] >> face[2];
			face[0]--;
			face[1]--;
			face[2]--;
			faces.push_back(face);
		}
	}
	Mesh mesh
	{
		std::move(vertices), std::move(faces)
	};
	ComputeNormals(mesh);
	return mesh;
}
