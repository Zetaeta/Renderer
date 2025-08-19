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
			{ {-1,-1.f, Z}, {0,1} }, // bottom left
			{ {-1,3.f, Z}, {0,-1} }, // above top left 
			{ {3,-1.f, Z}, {2,1} }, // right of bottom right
		};
		FullScreenTriangle = mDevice->CreateDirectMesh(verts);
		FullScreenTriangle->Topology = EPrimitiveTopology::Triangles;
	}
	return FullScreenTriangle;
}

rnd::DeviceMeshRef BasicMeshFactory::GetFullscreenSquare()
{
	constexpr float Z = 0.5f;
	if (!mSquare.IsValid())
	{
		FlatVert const verts[] =
		{
			{ {-1,1.f, Z}, {0,0} },
			{ {1,1.f, Z}, {1,0} },
			{ {-1,-1.f, Z}, {0,1} },
			{ {1,-1.f, Z}, {1,1} },
		};
		mSquare = mDevice->CreateDirectMesh(verts);
		mSquare->Topology = EPrimitiveTopology::TriangleStrip;
	}
	return mSquare;
}

DeviceTextureRef BasicTextureMgr::GetBlackTexture(IRenderDevice* device)
{
	if (mBlack == nullptr)
	{
		u32 const empty = 0x00000000;
		DeviceTextureDesc emptyDesc;
		emptyDesc.Width = 1;
		emptyDesc.Height = 1;
		emptyDesc.DebugName = "PlainBlack";
		mBlack = device->CreateTexture2D(emptyDesc, &empty);
	}
	return mBlack;
}

void BasicTextureMgr::Teardown()
{
	mBlack = nullptr;
}

}
