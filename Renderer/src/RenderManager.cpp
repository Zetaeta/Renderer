#include "RenderManager.h"
#include "ImguiControls.h"
#include "SceneComponent.h"
#include "misc/cpp/imgui_stdlib.h"
#include "JsonSerializer.h"
#include "JsonDeserializer.h"
#include <nfd.h>

using std::string;
using namespace rnd;

inline vec3 Vec3(aiVector3D const& v)
{
	return {v.x, v.y, v.z};
}

inline vec2 Vec2(aiVector3D const& v)
{
	return {v.x, v.y};
}

 RenderManager::RenderManager(Input* input)
	: m_Camera(input), scene(&m_AssMan)
{
	int			   x, y, comp;
	unsigned char* data = stbi_load("content/Dirt.png", &x, &y, &comp, 4);
	fprintf(stdout, "width: %d, height: %d, comp: %d\n", x, y, comp);
	TextureRef dirt = (x > 0 && y > 0) ? Texture::Create(x, y, data) : Texture::EMPTY;
	stbi_image_free(data);
	scene.Materials() = { { { 1.f, 0.f, 0.f, 1.f }, 1.0, 10 }, { { 0.f, 1.f, 0.f, 1.f }, 1.0, 500, 1.f, dirt }, { vec4{ 1.f, 1.f, 0.f, 1.f } } };
	scene.m_Spheres = { Sphere({ 0, 0, 2 }, .5f, 0), { { 0, 1, 2 }, .3f, 1 } };
	//scene.m_DirLights = { DirLight({ -1.f, -1.f, 1.f }, 0.5) };
	//scene.m_PointLights.emplace_back(vec3(1), vec3(1),1.f);
//	scene.m_SpotLights.emplace_back(vec3(1), vec3{ 0, 0, 1 }, vec3(1), 1.f, 1.f);
	vector<IndexedTri> inds = { IndexedTri({ 0, 1, 2 }) };
	scene.m_Objects.emplace_back(make_unique<LightObject<SpotLight>>(&scene));
	scene.m_Objects.emplace_back(make_unique<LightObject<PointLight>>(&scene));
	scene.m_Objects.emplace_back(make_unique<LightObject<DirLight>>(&scene));
	auto cube = m_AssMan.AddMesh(Mesh::Cube(1));
	MeshComponent& cubeMC = scene.m_Objects.emplace_back(make_unique<SceneObject>(&scene, "cube"))->SetRoot<MeshComponent>();
	cubeMC.SetMesh(cube);
	cubeMC.SetTransform(RotTransform{ { 0, 0, 2 }, { 1, 1, 1 } } );
		//	scene.m_MeshInstances.emplace_back(MeshInstance{ m_AssMan.AddMesh(Mesh::Cube(1)),  );
	ComputeNormals(scene.GetMesh(0));
	m_Renderer = make_unique<RastRenderer>(&m_Camera, 0, 0, nullptr);

	ObjReader objReader{ scene.Materials() };

	scene.Initialize();

	for (const auto& file : fs::directory_iterator("content"))
	{
		//m_AssMan.LoadAssetUntyped(file);
		/*
		Assimp::Importer importer;
		const aiScene*	 aiscene = importer.ReadFile(file.path().string(),
			  aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_ConvertToLeftHanded
				  | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType
				  | aiProcess_PreTransformVertices | aiProcess_FlipWindingOrder);
		cout << file.path() << endl;
		if (aiscene != nullptr)
		{
			AddToScene(scene, aiscene);
		}
		else if (file.path().string().ends_with(".obj"))
		{
			auto cmesh = objReader.Parse(file.path().string());
			u32	 start = scene.m_Meshes.size();
			scene.m_Meshes.insert(scene.m_Meshes.end(), cmesh.meshes.begin(), cmesh.meshes.end());
			auto& object = scene.m_CompoundMeshes.emplace_back(cmesh.name);
			object.components.resize(cmesh.meshes.size());
			for (u32 i = 0; i < cmesh.meshes.size(); ++i)
			{
				object.components[i].name = cmesh.meshes[i].name;
				object.components[i].instance = start + i;
			}
		}*/
	}
}

	string savefile;
void RenderManager::SceneControls()
{
	int			i = 0;
	const float speed = 0.1f;
	ImGui::Text("Camera pos: (%.2f, %.2f, %.2f)", m_Camera.position.x, m_Camera.position.y, m_Camera.position.z);
	if (ImGui::TreeNode("Materials"))
	{
		for (int m = 0; m < scene.Materials().size(); ++m)
		{
			auto& mat = scene.Materials()[m];
			ImGui::PushID(&mat);
			ImGui::Text("Material %d", m);
			if (ImGui::TreeNode("Details"))
			{
				ImGui::DragFloat4("Colour", &mat.colour[0]);
				ImGui::DragFloat("Specularity", &mat.specularity);
				ImGui::DragInt("SpecExp", &mat.specularExp);
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
		ImGui::TreePop();
	}
	ImGui::InputText("Save file", &savefile);
	if (ImGui::Button("Save"))
	{
		std::filesystem::create_directory("saves");
		JsonSerializer::Dump(ConstValuePtr::From(scene), "saves/" + savefile);
	}
	if (ImGui::Button("Load"))
	{
		Scene newScene(&m_AssMan);
		if (JsonDeserializer::Read(ValuePtr::From(newScene), "saves/" + savefile))
		{
			scene = std::move(newScene);
			scene.Initialize();
		}
	}
	
	if (ImGui::TreeNode("Add Object"))
	{
		auto const& soType = GetClassTypeInfo<SceneObject>();
		TypeSelector(soType);
		if (ImGui::Button("Create"))
		{
			scene.CreateObject(*m_SelectedSO);
		}
		ImGui::TreePop();
	}

	ImGui::Text("Meshes");
	if (ImGui::TreeNode("Meshes"))
	{
		auto& meshes = m_AssMan.GetMeshes();
		for (int m = 0; m < meshes.size(); ++m)
		{
			ImGui::PushID(m);
			ImGui::Text("Mesh %d", m);
			auto& mesh = meshes[m];
			ImGui::Text(mesh.name.c_str());
			if (ImGui::Button("Add to scene"))
			{
				scene.m_MeshInstances.emplace_back(m, Transform{ { 0, -2, 3 } });
			}
			ImGui::DragInt("MaterialID", &mesh.material, 1, 0, scene.Materials().size() - 1);
			if (ImGui::TreeNode("Vertices"))
			{
				for (auto& vert : mesh.vertices)
				{
					ImGui::PushID(&vert);
					ImGui::DragFloat3("vertex", &vert.pos.x, speed);
					ImGui::DragFloat3("normal", &vert.normal.x, speed);
					ImGui::PopID();
				}
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Faces"))
			{
				for (auto& tri : mesh.triangles)
				{
					ImGui::PushID(&tri);
					ImGui::DragScalarN("index", ImGuiDataType_U16, &tri[0], 3, 1);
					// ImGui::DragInt3("index", reinterpret_cast<int*>(&tri[0]), 1, 0, 2);
					ImGui::PopID();
				}
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Compound Meshes"))
	{
		for (int m = 0; m < scene.CompoundMeshes().size(); ++m)
		{
			ImGui::PushID(m);
			auto const& cmesh = scene.CompoundMeshes()[m];
			ImGui::Text(cmesh.name.c_str());
			ImGui::Text("%d parts", cmesh.components.size());
			if (ImGui::Button("Add to scene"))
			{
				scene.InsertCompoundMesh(cmesh);
				for (auto mesh : cmesh.components)
				{
					//scene.m_MeshInstances.emplace_back(mesh.instance, Transform{ { 0, -2, 3 } });
				}
			}
			ImGui::PopID();
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Mesh files"))
	{
		if (ImGui::Button("Refresh"))
		{
			m_AssMan.RefreshMeshList();
		}
		for (int m = 0; m < m_AssMan.MeshList().size(); ++m)
		{
			auto const& mesh = m_AssMan.MeshList()[m];
			ImGui::PushID(m);

			ImGui::Text(mesh.path.c_str());
			if (mesh.loaded)
			{
				ImGui::Text("Loaded");
			}
			else
			{
				if (ImGui::Button("Load"))
				{
					m_AssMan.LoadMesh(mesh.path);
				}
			}
			ImGui::PopID();
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Mesh instances"))
	{
		for (int m = 0; m < scene.m_MeshInstances.size(); ++m)
		{
			ImGui::PushID(m);
			ImGui::Text("Mesh inst %d", m);
			if (ImGui::Button("Delete"))
			{
				scene.m_MeshInstances.erase(scene.m_MeshInstances.begin() + m);
			}
			auto& mi = scene.m_MeshInstances[m];
			ImGui::DragInt("index",
				reinterpret_cast<int*>(&mi.mesh), 1.f, 0, m_AssMan.GetMeshes().size());
			ImGuiControls(mi.trans);
			//mi.trans.rotation.Normalize();
			ImGui::PopID();
		}
		ImGui::TreePop();
	}

	ImGui::Text("Scene objects");
	for (int o = 0; o < scene.m_Objects.size(); ++o)
	{
		auto& so = *scene.m_Objects[o];
		ImGui::PushID(&so);
		if (ImGui::TreeNode(so.GetName().c_str()))
		{
			rnd::ImGuiControls(so);
			if (ImGui::Button("Delete"))
			{
				scene.m_Objects.erase(scene.m_Objects.begin() + o);
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	ImGui::Text("Lights");
	ImGui::DragFloat3("Ambient", &scene.m_AmbientLight.x, 0.01f, 0.f, 1.f);
	for (int l = 0; l < scene.m_DirLights.size(); ++l)
	{
		ImGui::PushID(l);
		ImGui::Text("Directional light %d", l);
		vec3& dir = scene.m_DirLights[l].dir;
		if (ImGui::DragFloat3("direction", &dir.x, speed))
		{
			dir = glm::normalize(scene.m_DirLights[l].dir);
		}
		ImGui::DragFloat3("intensity", &scene.m_DirLights[l].colour.x, 0.01f, 0.f, 1.f);
		ImGui::PopID();
	}
	for (int l = 0; l < scene.m_SpotLights.size(); ++l)
	{
		auto& light = scene.m_SpotLights[l];
		ImGui::PushID(&light);
		ImGui::Text("Spotlight %d", l);
		ImGui::DragFloat3("position", &light.pos.x, speed);
		vec3& dir = light.dir;
		if (ImGui::DragFloat3("direction", &dir.x, speed))
		{
			dir = glm::normalize(dir);
		}
		ImGui::DragFloat3("colour", &light.colour.x, 0.01f);
		ImGui::DragFloat("falloff", &light.falloff, 0.02f);
		ImGui::DragFloat("tan(angle)", &light.tangle, 0.01f);
		ImGui::PopID();
	}
	for (int l = 0; l < scene.m_PointLights.size(); ++l)
	{
		auto& light = scene.m_PointLights[l];
		ImGui::PushID(&light);
		ImGui::Text("Point light %d", l);
		vec3& pos = scene.m_PointLights[l].pos;
		ImGui::DragFloat3("position", &pos.x, speed);
		ImGui::DragFloat("radius", &scene.m_PointLights[l].radius, 0.02f);
		ImGui::DragFloat3("intensity", &scene.m_PointLights[l].colour[0], 0.01f);
		ImGui::DragFloat("falloff", &scene.m_PointLights[l].falloff, 0.02f);
		ImGui::PopID();
	}
	ImGui::Text("Spheres");
	for (auto& sphere : scene.m_Spheres)
	{
		ImGui::PushID(i);
		ImGui::BeginGroup();
		ImGui::DragFloat3("origin", &sphere.origin.x, speed);
		ImGui::DragFloat("radius", &sphere.radius, speed);
		ImGui::EndGroup();
		ImGui::PopID();
		++i;
	}
	for (auto& so : scene.m_Objects)
	{
		so->Update();
	}
}

void RenderManager::TypeSelector(ClassTypeInfo const& cls)
{
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (m_SelectedSO == &cls)
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	bool expanded = ImGui::TreeNodeEx(&cls, flags, cls.GetName().c_str());
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		m_SelectedSO = &cls;
	}
	if (expanded)
	{
		for (auto child : cls.GetImmediateChildren())
		{
			TypeSelector(*child);
		}
		ImGui::TreePop();
	}
}
