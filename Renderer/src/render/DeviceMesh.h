#pragma once

#include "core/Maths.h"
#include "VertexAttributes.h"
#include "core/RefCounted.h"
#include "RenderEnums.h"

namespace rnd
{

class IDeviceMesh : public SelfReleasingRefCounted
{
public:
	DeviceVertAttsRef VertexAttributes;

	IDeviceMesh(DeviceVertAttsArg vertAttrs = InvalidVertAttsHandle, u32 vertCount = 0, EPrimitiveTopology topology = EPrimitiveTopology::TRIANGLES)
		: VertexAttributes(vertAttrs), VertexCount(vertCount), Topology(topology) {}


	DeviceVertAttsArg GetVertexAttributes()
	{
		return VertexAttributes;
	}

	u32 VertexCount;
	EPrimitiveTopology Topology;
	const EDeviceMeshType MeshType = EDeviceMeshType::DIRECT;

protected:
	IDeviceMesh(EDeviceMeshType InMeshType, DeviceVertAttsArg vertAttrs = InvalidVertAttsHandle, u32 vertCount = 0, EPrimitiveTopology topology = EPrimitiveTopology::TRIANGLES)
		: VertexAttributes(vertAttrs), VertexCount(vertCount), Topology(topology), MeshType(InMeshType) {}
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

