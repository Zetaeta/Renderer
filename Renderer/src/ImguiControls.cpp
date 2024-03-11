#include "ImguiControls.h"
#include <format>
#include "misc/cpp/imgui_stdlib.h"


namespace rnd
{

bool rnd::ImGuiControls(TTransform<quat>& trans)
{
	bool edited = false;
	edited |= ImGui::DragFloat3("translation", &trans.translation[0], 0.1f);
	edited |= ImGui::DragFloat3("scale", &trans.scale[0], 0.1f);
	if (ImGui::DragFloat4("rotation", &trans.rotation[0], 0.05f))
	{
		edited = true;
		trans.rotation = normalize(trans.rotation);
	}
	return edited;
}

bool ImGuiControlBasic(char const* name, ValuePtr val, bool isConst)
{
	if (val.GetType() == GetTypeInfo<int>())
	{
		if (isConst)
		{
			ImGui::Text("%s: %d", name, val.GetAs<int>());
		}
		else
		{
			return ImGui::DragInt(name, &val.GetAs<int>());
		}
	}
	else if (val.GetType() == GetTypeInfo<float>())
	{
		if (isConst)
		{
			ImGui::Text("%s: %f", name, val.GetAs<float>());
			return false;
		}
		else
		{
			return ImGui::DragFloat(name, &val.GetAs<float>());
		}
	}
	else if (val.GetType() == GetTypeInfo<String>())
	{
		if (isConst)
		{
			ImGui::Text("%s: %s", name, val.GetAs<String>().c_str());
		}
		else
		{
			ImGui::InputText(name, &val.GetAs<String>());
		}
	}
	return false;
}

ImGuiDataType GetImGuiType(TypeInfo const& typ)
{
	if (typ == GetTypeInfo<int>())
	{
		return ImGuiDataType_S32;
	}
	if (typ == GetTypeInfo<float>())
	{
		return ImGuiDataType_Float;
	}
	if (typ == GetTypeInfo<double>())
	{
		return ImGuiDataType_Double;
	}
	if (typ == GetTypeInfo<u32>())
	{
		return ImGuiDataType_U32;
	}
	if (typ == GetTypeInfo<u32>())
	{
		return ImGuiDataType_U32;
	}
	return -1;
}

float GetSpeed(TypeInfo const& typ)
{
	if (typ == GetTypeInfo<float>() || typ == GetTypeInfo<double>())
	{
		return 0.01f;
	}
	return 1.f;
}


bool ImGuiControls(char const* name, ValuePtr val, bool isConst)
{
	if (isConst)
	{
		ImGui::BeginDisabled();
	}
	switch (val.GetType().GetTypeCategory())
	{
		case ETypeCategory::CLASS:
			if (ImGui::TreeNode(name))
			{
				bool result = ImGuiControls(ClassValuePtr { val }, isConst);
				ImGui::TreePop();
				return result;
			}
			break;
		case ETypeCategory::BASIC:
			return ImGuiControlBasic(name, val, isConst);
			break;
		case ETypeCategory::CONTAINER:
		{
			ContainerTypeInfo const& ctr = static_cast<ContainerTypeInfo const&>(val.GetType());
			TypeInfo const&			 contained = ctr.GetContainedType();
			auto					 acc = ctr.CreateAccessor(val);
			if (!ctr.HasFlag(ContainerTypeInfo::RESIZABLE) && contained.GetTypeCategory() == ETypeCategory::BASIC && contained != GetTypeInfo<String>())
			{
				ImGuiDataType imTyp = GetImGuiType(contained);
				float		  speed = GetSpeed(contained);
				return ImGui::DragScalarN(name, imTyp, acc->GetAt(0).GetPtr(), acc->GetSize(), speed);
			}
			bool edited = false;
			if (ImGui::TreeNode(name))
			{
				bool areConst = isConst || ctr.HasFlag(ContainerTypeInfo::CONST_CONTENT);
				for (u32 i = 0; i < acc->GetSize(); ++i)
				{
					auto idx = std::format("[{}]", i);
					ImGui::PushID(i);
					edited |= ImGuiControls(idx.c_str(), acc->GetAt(i), areConst);
					ImGui::PopID();
				}
				ImGui::TreePop();
			}
			return edited;
		}
		case ETypeCategory::POINTER:
		{
			PointerTypeInfo const& ptr = static_cast<PointerTypeInfo const&>(val.GetType());
			if (ptr.IsNull(val))
			{
				ImGui::Text("nullptr");
				return false;
			}
			return ImGuiControls(name, ptr.Get(val), isConst || ptr.IsConst());
		}
		
		default:
			break;
	}
	if (isConst)
	{
		ImGui::EndDisabled();
	}
	return false;
}

bool ImGuiControls(ClassValuePtr obj, bool isConst)
{
	bool changed = false;
	obj = obj.Downcast();
	obj.GetRuntimeType().ForEachProperty([&obj, &changed, isConst] (PropertyInfo const* prop)
	{
		changed |= ImGuiControls(prop->GetName().c_str(), prop->Access(obj), isConst || prop->IsConst());
	});
	return changed;
}
}
