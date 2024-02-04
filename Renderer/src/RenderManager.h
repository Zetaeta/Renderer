#pragma once
#include "stb_image.h"
#include "Texture.h"
#include <memory>
#include "Renderer.h"
#include "Scene.h"
#include <filesystem>
#include "Camera.h"
#include "RastRenderer.h"
#include "ObjReader.h"
#include <iostream>
#include "imgui.h"
#include "DX11Renderer.h"
#include <ranges>
#include <numeric>
#include "assimp/Importer.hpp"
#include <assimp/scene.h>		// Output data structure
#include <assimp/postprocess.h> // Post processing flags

using namespace std;
using namespace glm;
namespace fs = std::filesystem;

class RenderManager
{
public:
	RenderManager(Input* input);

	void SceneControls();

protected:
	Camera m_Camera;
	unique_ptr<IRenderer> m_Renderer;
	Scene scene;
	AssetManager m_AssMan;
};
