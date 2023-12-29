#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "glm/fwd.hpp"
#include "../Scene.h"
#include "../RTRenderer.h"
#include "Walnut/Timer.h"
#include "Walnut/Input/Input.h"
#include <filesystem>
#include "../ObjReader.h"
#include "../RastRenderer.h"
#include "../stb_image.h"
#include "../Texture.h"

using namespace std;
using namespace glm;
namespace fs = std::filesystem;

class RenderLayer : public Walnut::Layer
{
public:
	RenderLayer() {
		int x, y, comp;
		unsigned char* data = stbi_load("content/Dirt.png", &x, &y, &comp, 4);
		fprintf(stdout, "width: %d, height: %d, comp: %d\n", x, y, comp);
		Texture dirt;
		if (x > 0 && y > 0)
		{
			dirt = Texture(x,y,data);
		}
		stbi_image_free(data);
		scene.m_Materials = {{{ 1.f, 0.f, 0.f, 1.f }, 10}, {{ 0.f, 1.f, 0.f, 1.f }, 500, dirt}, {vec4{ 1.f, 1.f, 0.f, 1.f }}};
		scene.m_Spheres = { Sphere({ 0, 0, 2 }, .5f, 0), { { 0, 1, 2 }, .3f, 1 } };
		scene.m_DirLights = { DirLight({ -1.f, -1.f,  1.f }, 0.5) };
		vector<IndexedTri> inds = { IndexedTri({ 0, 1, 2 }) };
		scene.m_Meshes = { Mesh(vector<Vertex>{ Vertex{vec3{ 2.f, 0.f, 3.f }}, Vertex{vec3{ -1.f, 1.f, 3.f }}, Vertex{vec3{ 1.f, -2.f, 3.f }} }, std::move(inds),
							   2),
			Mesh::Cube(1) };
		scene.m_MeshInstances = { { 0, Transform{} }, {0, Transform {{-1, 0, 0}, {1,2,1}, quat{}}} , {1, Transform { {0,0,2}, {1,1,1}, quat{} }}
		};
		m_Renderer = make_unique<RastRenderer>(&m_Camera, 0,0,nullptr);
		 
		for (const auto& file : fs::directory_iterator("content"))
		{
			cout << file.path() << endl;
			if (file.path().string().ends_with(".obj"))
			{
				scene.m_Meshes.push_back(ObjReader::Parse(file.path().string()));
				scene.m_MeshInstances.push_back({ u32(scene.m_Meshes.size() - 1), Transform{{0,-2,3}} });
			}
		}
	}
	virtual void OnUpdate(float deltaTime) {
		m_Camera.Tick(deltaTime);
	}

	void SceneControls() {
		int i = 0;
		const float speed = 0.1f;
		ImGui::Text("Spheres");
		for (auto& sphere : scene.m_Spheres) {
			ImGui::PushID(i);
			ImGui::BeginGroup();
			ImGui::DragFloat3("origin", &sphere.origin.x, speed);
			ImGui::DragFloat("radius", &sphere.radius, speed);
			ImGui::EndGroup();
			ImGui::PopID();
			++i;
		}
		 ImGui::Text("Meshes");
		if (ImGui::TreeNode("Meshes"))
		{
			for (int m = 0; m < scene.m_Meshes.size(); ++m) {
				ImGui::PushID(m);
				ImGui::Text("Mesh %d", m);
				auto& mesh = scene.m_Meshes[m];
				for (auto& vert : mesh.vertices) {
					ImGui::PushID(&vert);
					ImGui::DragFloat3("vertex", &vert.pos.x, speed);
					ImGui::DragFloat3("normal", &vert.normal.x, speed);
					ImGui::PopID();
				}
				for (auto& tri : mesh.triangles) {
					ImGui::PushID(&tri);
					ImGui::DragInt3("index", reinterpret_cast<int*>(&tri[0]), 1, 0, 2);
					ImGui::PopID();
				}
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
		ImGui::Text("Mesh instances");
		for (int m = 0; m < scene.m_MeshInstances.size(); ++m) {
			ImGui::PushID(m);
			ImGui::Text("Mesh inst %d", m);
			auto& mi = scene.m_MeshInstances[m];
			ImGui::DragInt("index", 
			reinterpret_cast<int*>( & mi.mesh));
			ImGui::DragFloat3("translation", &mi.trans.translation[0], speed);
			ImGui::DragFloat3("scale", &mi.trans.scale[0], speed);
			ImGui::PopID();
		}

		ImGui::Text("Lights");
		ImGui::DragFloat("Ambient", &scene.m_AmbientLight, 0.01, 0.f, 1.f);
		for (int l = 0; l < scene.m_DirLights.size(); ++l) {
			ImGui::PushID(l);
			ImGui::Text("Directional light %d", l);
			vec3& dir = scene.m_DirLights[l].dir;
			if (ImGui::DragFloat3("direction", &dir.x, speed)) {
				dir = glm::normalize(scene.m_DirLights[l].dir);
			}
			ImGui::DragFloat("intensity", &scene.m_DirLights[l].intensity, 0.01, 0.f, 1.f);
			ImGui::PopID();
		}
	}

	virtual void OnUIRender() override
	{

		ImGui::Begin("Controls");
		ImGui::Text("Frame time: %.3fms", m_FrameTime);
		SceneControls();
		ImGui::End();

		ImGui::Begin("Viewport");
		m_ViewWidth = ImGui::GetContentRegionAvail().x;
		m_ViewHeight = ImGui::GetContentRegionAvail().y;
		m_Timer.Reset();
		Render();
		if (m_Img)
		{
			ImGui::Image(m_Img->GetDescriptorSet(), { static_cast<float>(m_ViewWidth), static_cast<float>(m_ViewHeight) }, { 0, 1 }, {1,0});
		}
		ImGui::End();
	}

	void Render() {
		m_Timer.Reset();
		if (m_Img == nullptr || m_Img->GetWidth() != m_ViewWidth || m_Img->GetHeight() != m_ViewHeight) {
			m_Img = make_shared<Walnut::Image>(m_ViewWidth,m_ViewHeight, Walnut::ImageFormat::RGBA);
			//delete[] m_ImgData;
			m_ImgData.resize(m_ViewWidth * m_ViewHeight);
			//m_Renderer = make_unique<RTRenderer>(m_ViewWidth, m_ViewHeight, &m_ImgData[0]);
			m_Renderer->Resize(m_ViewWidth, m_ViewHeight, &m_ImgData[0]);
		}
		/* for (int i = 0; i < m_ViewHeight * m_ViewWidth; ++i)
		{
			m_ImgData[i] = 0xffff00ff;
		}*/
		m_Renderer->Render(scene);
		if (m_ViewHeight * m_ViewHeight != 0) {
			m_Img->SetData(&m_ImgData[0]);
		}
		m_FrameTime = m_Timer.ElapsedMillis();
	}

private:
	shared_ptr<Walnut::Image> m_Img;
	vector<u32> m_ImgData;
	u32 m_ViewWidth;
	u32 m_ViewHeight;
	Scene scene;
	Camera m_Camera;
	unique_ptr<IRenderer> m_Renderer;
	Walnut::Timer m_Timer;
	float m_FrameTime = 0;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Walnut Example";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<RenderLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});
	return app;
}