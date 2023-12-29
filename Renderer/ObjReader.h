#pragma once
#include <string>
#include "Scene.h"

using std::string;

class ObjReader
{
public:
	static Mesh Parse(string file);
};
