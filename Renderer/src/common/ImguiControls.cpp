#include "ImguiControls.h"
#include <core/TypeInfoUtils.h>
#include <format>
#include "misc/cpp/imgui_stdlib.h"

namespace rg = std::ranges;


bool ImGuiControls(TTransform<quat>& trans)
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

struct NumericControlMeta
{
	std::optional<double> dragSpeed;
	std::optional<double> min;
	std::optional<double> max;

	template<typename T>
	T DragSpeed(double def = 0.1) const
	{
		return static_cast<T>(dragSpeed.value_or(def));
	}

	template<typename T>
	T Min() const
	{
		return static_cast<T>(min.value_or(0.));
	}

	template<typename T>
	T Max() const
	{
		return static_cast<T>(max.value_or(0.));
	}

	void Apply(MemberMetadata const& memberMeta)
	{
		auto memDrag = memberMeta.GetDragSpeed();
		if (memDrag.has_value())
		{
			dragSpeed = memDrag.value();
		}

		auto memMin = memberMeta.GetMinValue();
		if (memMin.has_value())
		{
			min = memMin.value();
		}

		auto memMax = memberMeta.GetMaxValue();
		if (memMax.has_value())
		{
			max = memMax.value();
		}
	}
};

bool ImGuiControlBasic(char const* name, ReflectedValue val, bool isConst, NumericControlMeta const& meta = {})
{
	if (val.GetType() == GetTypeInfo<int>())
	{
		if (isConst)
		{
			ImGui::Text("%s: %d", name, val.GetAs<int>());
		}
		else
		{
			return ImGui::DragInt(name, &val.GetAs<int>(), meta.DragSpeed<float>(), meta.Min<int>(), meta.Max<int>());
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
			return ImGui::DragFloat(name, &val.GetAs<float>(), meta.DragSpeed<float>(), meta.Min<float>(), meta.Max<float>());
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


bool ImGuiControls(char const* name, ReflectedValue val, bool isConst, bool expand = false, NumericControlMeta controlMeta = {})
{
	if (isConst)
	{
		ImGui::BeginDisabled();
	}
	switch (val.GetType().GetTypeCategory())
	{
		case ETypeCategory::CLASS:
			if (expand)
			{
				return ImGuiControls(ClassValuePtr { val }, isConst);
			}

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
			controlMeta.Apply(ctr.GetContainedMeta());
			if (!ctr.HasFlag(ContainerTypeInfo::RESIZABLE) && contained.GetTypeCategory() == ETypeCategory::BASIC && contained != GetTypeInfo<String>())
			{
				ImGuiDataType imTyp = GetImGuiType(contained);
				float		  defaultSpeed = GetSpeed(contained);
				return ImGui::DragScalarN(name, imTyp, acc->GetAt(0).GetPtr(), NumCast<u32>(acc->GetSize()), controlMeta.DragSpeed<float>(defaultSpeed));
			}

			bool edited = false;
			auto displayContainer = [&] {
				bool areConst = isConst || ctr.HasFlag(ContainerTypeInfo::CONST_CONTENT);
				for (u32 i = 0; i < acc->GetSize(); ++i)
				{
					auto idx = std::format("[{}]", i);
					ImGui::PushID(i);
					edited |= ImGuiControls(idx.c_str(), acc->GetAt(i), areConst);
					ImGui::PopID();
				}

				};
			if (expand)
			{
				ImGui::Text(name);
				displayContainer();
			}
			else if (ImGui::TreeNode(name))
			{
				displayContainer();
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

bool ImGuiControls(ClassValuePtr obj, bool isConst, const PropertyFilter& filter)
{
	bool changed = false;
	obj = obj.Downcast();
	obj.GetRuntimeType().ForEachProperty([&obj, &changed, isConst, &filter] (PropertyInfo const& prop)
	{
		if (rg::find(filter.ExcludedProperties, &prop) != filter.ExcludedProperties.end()
			|| rg::find_if(filter.ExcludedTypes, [&prop](const TypeInfo* type)
				{
				return type->Contains(prop.GetType());
			}) != filter.ExcludedTypes.end())
		{
			return;
		}
		auto const& meta = prop.GetMetadata();
		bool expand = meta.GetAutoExpand().value_or(false);	
		NumericControlMeta controlMeta;
		controlMeta.Apply(meta);

		if (ImGuiControls(prop.GetName().c_str(), prop.Access(obj), isConst || prop.IsConst(), expand, controlMeta))
		{
			if (prop.HasSetter())
			{
				ReflectedValue val[1] = { prop.Access(obj) };
				prop.GetSetter()->Invoke(obj, NoValue, val);
			}
			changed = true;
		}
		
	});
	return changed;
}
