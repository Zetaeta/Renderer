#include "BasicMeshes.h"
#include "VertexTypes.h"
#include "RenderDevice.h"

namespace rnd
{

DeviceMeshRef BasicMeshFactory::GetFullScreenTri()
{
	constexpr float Z = 0.5f;
	if (!FullScreenTriangle.IsValid())
	{
		FlatVert const verts[3] =
		{
			{ {-1,-1.f, Z}, {0,1} },
			{ {-1,3.f, Z}, {0,-1} },
			{ {3,-1.f, Z}, {2,1} },
		};
		FullScreenTriangle = mDevice->CreateDirectMesh(verts);
		FullScreenTriangle->Topology = EPrimitiveTopology::TRIANGLES;
	}
	return FullScreenTriangle;
}

}
