#pragma once

#include "core/Maths.h"
#include "VertexAttributes.h"
#include "core/RefCounted.h"

namespace rnd
{

enum class EPrimitiveTopology : u8
{
	TRIANGLES
};

//struct DevicePrimitiveDesc
//{
//	u32 VertexCount = 0;
//	u32 TriCount = 0;
//};


enum class EDeviceMeshType : u8
{
	DIRECT,
	INDEXED
};

class IDeviceMesh : public RefCountedObject
{
public:
	DeviceVertAttsRef VertexAttributes;

	IDeviceMesh(DeviceVertAttsArg vertAttrs = InvalidVertAttsHandle, u32 vertCount = 0, EPrimitiveTopology topology = EPrimitiveTopology::TRIANGLES)
		: VertexAttributes(vertAttrs) {}


	DeviceVertAttsArg GetVertexAttributes()
	{
		return VertexAttributes;
	}

	u32 VertexCount;
	EPrimitiveTopology Topology;
	const EDeviceMeshType MeshType = EDeviceMeshType::DIRECT;

protected:
	IDeviceMesh(EDeviceMeshType MeshType, DeviceVertAttsArg vertAttrs = InvalidVertAttsHandle, u32 vertCount = 0, EPrimitiveTopology topology = EPrimitiveTopology::TRIANGLES)
		: VertexAttributes(vertAttrs) {}
};

class IDeviceIndexedMesh : public IDeviceMesh
{
public:
	IDeviceIndexedMesh(DeviceVertAttsArg vertAttrs = InvalidVertAttsHandle, u32 vertCount = 0, u32 idxCount = 0, EPrimitiveTopology topology = EPrimitiveTopology::TRIANGLES)
		: IDeviceMesh(EDeviceMeshType::INDEXED, vertAttrs, vertCount, topology), IndexCount(idxCount) {}

	u32 IndexCount = 0;
};

using DeviceMeshRef = RefPtr<IDeviceMesh>;

}

