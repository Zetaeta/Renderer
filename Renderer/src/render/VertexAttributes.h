#pragma once

#include "core/Maths.h"
#include "core/Hash.h"
#include "DataLayout.h"
#include "container/SmallMap.h"

namespace rnd
{

enum VertexAttributeMask : u64
{
	VA_Position = 0x01,
	VA_Normal = 0x02,
	VA_Tangent = 0x04,
	VA_TexCoord = 0x08
};

class VertexAttributeDesc
{
public:
	using Handle = s64;

	class Registry
	{
		RCOPY_MOVE_PROTECT(Registry)
	public:
		Registry();
//		Handle currentId = 0;
		Handle Register(VertexAttributeDesc&& vertAtts)
		{
			mRegistry.emplace_back(std::move(vertAtts));
			return mRegistry.size() - 1;
		}
		SmallMap<HashString, VertexAttributeMask> SemanticMap;

		VertexAttributeDesc const& Get(Handle handle)
		{
			return mRegistry[handle];
		}
	private:
		std::vector<VertexAttributeDesc> mRegistry;
	};

	static SmallMap<HashString, VertexAttributeMask> const& GetSemanticMap() { return GetRegistry().SemanticMap; }

	static Registry& GetRegistry()
	{
		static Registry sRegistry;
		return sRegistry;
	}

	VertexAttributeDesc(Vector<DataLayoutEntry>&& Entries)
		: Layout(4, std::move(Entries)) {}

	u32 GetSize() const
	{
		return NumCast<u32>(Layout.GetSize());
	}

	DataLayout Layout;
};

template <typename T>
struct TVertexAttributes
{
	static VertexAttributeDesc::Handle Handle;// = VertexAttributeDesc::GetRegistry().Register(CreateVertexAttributes(T{}));
};
#define DECLARE_VERTEX_ATTRIBUTE_DESC(Type)\
	template<>\
	struct ::rnd::TVertexAttributes<Type>\
	{\
		static ::rnd::VertexAttributeDesc::Handle Handle;\
	};

template <typename T>
VertexAttributeDesc::Handle GetVertAttHdl(const T& val = {})
{
	return TVertexAttributes<T>::Handle;
}

#define CREATE_VERTEX_ATTRIBUTE_DESC(Type, Body)\
	rnd::VertexAttributeDesc::Handle rnd::TVertexAttributes<Type>::Handle = rnd::VertexAttributeDesc::GetRegistry().Register(([] Body)());

//class IDeviceVertAtts
//{
//};

//using DeviceVertAttsArg = IDeviceVertAtts const*;
//using DeviceVertAttsRef = RefPtr<IDeviceVertAtts const>;

using DeviceVertAttsArg = VertexAttributeDesc::Handle;
using DeviceVertAttsRef = VertexAttributeDesc::Handle;

constexpr DeviceVertAttsRef InvalidVertAttsHandle = -1;

struct VertAttDrawInfo
{
	VertexAttributeDesc::Handle BufferHandle = 0;
	VertexAttributeMask ShaderMask = {};

	bool operator==(VertAttDrawInfo const& other) const
	{
		return BufferHandle == other.BufferHandle && ShaderMask == other.ShaderMask;
	}
};


}

START_HASH(rnd::VertAttDrawInfo, info)
{
	return CombineHash(Hash(info.BufferHandle), (u64) info.ShaderMask);
}
END_HASH(rnd::VertAttDrawInfo)
