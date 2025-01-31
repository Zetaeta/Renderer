#include "Editor.h"
#include <common/Input.h>
#include <render/RenderManager.h>
#include <render/RenderScreenIds.h>
#include <common/ImguiControls.h>
#include <scene/SceneObject.h>
#include "core/Logging.h"

DEFINE_LOG_CATEGORY_STATIC(LogEditor);


Vector<Viewport*> Editor::mViewports;
Editor* Editor::sSingleton;

Editor::Editor(Input* input, RenderManager* rmg)
	: mInput(input), mRmgr(rmg)
{
	mScene = &rmg->mScene;
//	mViewports.push_back(static_cast<rnd::dx11::DX11Renderer*>(rmg->Renderer())->GetViewport());
	mLastSaved = std::chrono::system_clock::now();
}

Editor* Editor::Create(Input* input, class RenderManager* rmg)
{
	ZE_ASSERT(sSingleton == nullptr);
	return (sSingleton = new Editor(input, rmg));
}

void Editor::Destroy()
{
	delete sSingleton;
	sSingleton = nullptr;
}

void Editor::RegisterViewpoert(Viewport* vp)
{
	mViewports.push_back(vp);
}

void Editor::UnegisterViewpoert(Viewport* vp)
{
	Remove(mViewports, vp);
}

void Editor::OnClickPoint(ivec2 position)
{
	Viewport* viewport = GetViewportAt(position);
	if (!viewport || position.x >= (int) viewport->GetWidth() || position.y >= (int) viewport->GetHeight()
		|| position.x < 0 || position.y < 0)
	{
		return;
	}
	viewport->OnClick(position);

	printf("Click(%d,%d)\n", position.x, position.y);
	CreateScreenIdTex(viewport->GetWidth(), viewport->GetHeight());
	//viewport->GetRenderContext()->RunSinglePass<rnd::RenderScreenIds>("ScreenIds", viewport->mCamera,
	//																	mScreenIdTex->GetRT());
	using namespace rnd;
	MappedResource mapped = mScreenIdTex->Map(0, ECpuAccessFlags::Read);
	const u32* textureData = reinterpret_cast<const u32*>(mapped.Data);
	const int width = mScreenIdTex->Desc.Width;
	const int height = mScreenIdTex->Desc.Height;
	ZE_ASSERT(position.x < width);
	ZE_ASSERT(position.y < height);
	const u32 id = textureData[position.x + position.y * (mapped.RowPitch / 4)];
	mScreenIdTex->Unmap(0);
	printf("Id: %u\n", id);
	if (ScreenObject* so = GetScreenObjManager().GetScreenObj(id))
	{
		so->OnClicked();
	}
}

void Editor::OnLeftClick()
{
	ivec2 mousePos = mInput->GetAbsoluteMousePos();
	for (Viewport* vp : mViewports)
	{
		ivec2 vpPos = vp->GetPos();
		ivec2 delta = mousePos - vpPos;
		if (delta.x >= 0 && delta.y >= 0 && delta.x < vp->GetSize().x && delta.y < vp->GetSize().y)
		{
			vp->OnClick(delta);
		}
	}
	//OnClickPoint({NumCast<int>(mousePos.x), NumCast<int>(mousePos.y)});
}

void Editor::OnEndLeftClick()
{
	RLOG(LogGlobal, Info, "End left click");
}

void Editor::OnRightClick()
{
	return;
	vec2 mousePos = mInput->GetWindowMousePos();
	Viewport* viewport = GetViewportAt(mousePos);
	viewport->DebugPixel({NumCast<int>(mousePos.x), NumCast<int>(mousePos.y)});
}

void Editor::OnWindowResize(u32 width, u32 height)
{
}

void Editor::DrawControls()
{
	DrawObjectsWindow();
	DrawComponentsWindow();
}

void Editor::DrawObjectsWindow()
{
	if (!ImGui::Begin("Scene objects"))
	{
		ImGui::End();
		return;	
	}
	if (ImGui::BeginListBox("Scene objects"))
	{
		for (int o = 0; o < mScene->m_Objects.size(); ++o)
		{
			auto& so = *mScene->m_Objects[o];
			ImGui::PushID(&so);
			bool isSelected = o == mSelectedSceneObj;
			if (ImGui::Selectable(so.GetName().c_str(), isSelected))
			{
				mSelectedSceneObj = o;
			}
			ImGui::PopID();
		}
		ImGui::EndListBox();
	}
	ImGui::End();
}

void Editor::DrawComponentsWindow()
{
	if (!ImGui::Begin("Components"))
	{
		ImGui::End();
		return;	
	}
	if (SceneObject* obj = GetSelectedObject())
	{
		if (mSelectedComponent == nullptr || mSelectedComponent->GetOwner() != obj)
		{
			mSelectedComponent = obj->GetRoot();
		}
		DrawComponentsRecursive(obj->GetRoot());
		PropertyFilter propertyFilter;
		propertyFilter.ExcludedTypes.push_back(&SceneComponent::StaticClass());
		if (ImGui::TreeNodeEx("SceneObject properties", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGuiControls(ClassValuePtr::From(obj), false, propertyFilter);
			ImGui::TreePop();
		}
		ImGui::Text("Screen id %d", mSelectedComponent->GetScreenId());
		if (ImGui::TreeNodeEx("Component properties", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGuiControls(ClassValuePtr::From(mSelectedComponent.Get()), false, propertyFilter);
			ImGui::TreePop();
		}
	}
	ImGui::End();
}

void Editor::DrawComponentsRecursive(SceneComponent* comp)
{
	if (comp == nullptr)
	{
		return;
	}
	static ImGuiTreeNodeFlags const baseFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick
		| ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
	ImGuiTreeNodeFlags flags = baseFlags;
	if (mSelectedComponent == comp)
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	if (ImGui::TreeNodeEx(comp, flags, comp->GetTypeInfo().GetName().c_str()))
	{
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		{
			SelectComponent(comp);
		}
		for (auto const& child : comp->GetChildren())
		{
			DrawComponentsRecursive(child.get());
		}
		ImGui::TreePop();
	}
}

void Editor::Tick(float dt)
{
	for (auto& vp : mViewports)
	{
		//CreateScreenIdTex(vp->GetWidth(), vp->GetHeight());
	}
	DrawControls();
	bool isMouseDown = mInput->IsMouseDown(Input::MouseButton::LEFT);
	//&& !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)
	;
	if (isMouseDown != mWasMouseDown)
	{
		if (!ImGui::GetIO().WantCaptureMouse)
		{
			if (isMouseDown)
			{
				OnLeftClick();
			}
			else
			{
				OnEndLeftClick();
			}
		
		}
		mWasMouseDown = isMouseDown;
	}

	bool isRightMouseDown = mInput->IsMouseDown(Input::MouseButton::RIGHT);
	//&& !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)
	;
	if (isRightMouseDown != mWasRightMouseDown)
	{
		if (!ImGui::GetIO().WantCaptureMouse)
		{
			if (isRightMouseDown)
			{
				OnRightClick();
			}
			else
			{
				OnEndRightClick();
			}
		
		}
		mWasRightMouseDown = isRightMouseDown;
	}

	// Autosave
	if (!isMouseDown && mScene->IsModified())
	{
		using namespace std::chrono;
		auto currTime = system_clock::now();
		auto timeSinceSaved = double(std::chrono::duration_cast<ch::seconds>(currTime - mLastSaved).count());
		if (timeSinceSaved > 10)
		{
			mRmgr->SaveScene("default.json");
			mLastSaved = currTime;
			mScene->ClearModified(true);
		}
	}
	//	mWasMouseDown = isMouseDown;
}

void Editor::SelectComponent(SceneComponent* Component)
{
	
	if (IsValid(mSelectedComponent) && Component != mSelectedComponent)
	{
		mSelectedComponent->SetSelected(false);
	}
	SelectObject(Component->GetOwner());

	ForEachCompRecursive(*Component->GetOwner()->GetRoot(), [](SceneComponent& comp)
	{
		comp.SetSelected(false);
	});
	mSelectedComponent = Component->shared_from_this();
	Component->SetSelected(true);
}

void Editor::ClickComponent(class SceneComponent* Component)
{
	SceneObject* Object = Component->GetOwner();
	if (GetSelectedObject() == Object)
	{
		SelectComponent(Component);
	}
	else
	{
		SelectObject(Object);
	}
}

void Editor::CreateScreenIdTex(u32 width, u32 height)
{
	if (mScreenIdTex && mScreenIdTex->Desc.Width == width && mScreenIdTex->Desc.Height == height)
	{
		return;
	}

	using namespace rnd;
	DeviceTextureDesc desc;
	desc.DebugName = "ScreenIds";
	desc.Flags = TF_RenderTarget | TF_CpuReadback;
	desc.Format = ETextureFormat::R32_Uint;
	desc.Width = width;
	desc.Height = height;
	mScreenIdTex = mRmgr->Device()->CreateTexture2D(desc);
}

SceneObject* Editor::GetSelectedObject()
{
	if (mSelectedSceneObj >= mScene->m_Objects.size())
	{
		return nullptr;
	}

	return mScene->m_Objects[mSelectedSceneObj].get();
}

void Editor::SelectObject(const SceneObject* obj)
{
	auto it = std::ranges::find_if(mScene->m_Objects, [obj](const auto& so)
	{
		return so.get() == obj;
	}) ;

	if (SceneObject* old = GetSelectedObject(); old && old != obj)
	{
		ForEachCompRecursive(*old->GetRoot(), [](SceneComponent& comp)
		{
			comp.SetSelected(false);
		});
	}
	
	if (it != mScene->m_Objects.end())
	{
		mSelectedSceneObj = NumCast<int>(it - mScene->m_Objects.begin());
		mSelectedComponent = (*it)->GetRoot();
		ForEachCompRecursive(*obj->GetRoot(), [](SceneComponent& comp)
		{
			comp.SetSelected(true);
		});
	}
}
