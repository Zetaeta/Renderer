#pragma once
#include <string>
#include <vector>
#include "Scene.h"

using std::string;

struct MtlMat
{
	string name;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float specExp = 10;
	string ambientMap;
	string diffuseMap;
	string specularMap;
	string specularExpMap;
	MaterialID loadedMat = -1;
};

using Mtlib = std::vector<MtlMat>;
using MaterialDB = std::vector<Material>;

class ObjReader
{
public:
	ObjReader(MaterialDB& matDB)
	: m_MatDB(matDB) {}

	std::vector<Mesh> Parse(string file);

	Mtlib ParseMtl(std::string file);


private:

	MaterialID LoadMat(MtlMat& mat);

	MaterialDB& m_MatDB;
};
