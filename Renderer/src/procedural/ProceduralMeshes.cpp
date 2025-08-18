#include "ProceduralMeshes.h"
#include "render/VertexTypes.h"

namespace ProceduralMeshes
{

using namespace rnd;

RefPtr<rnd::IDeviceMesh> CreateTiledPlane(rnd::IRenderDevice* device, float tileSize, vec2 startPos, uvec2 numTiles)
{
	u32 numVerts = (numTiles.x + 1) * (numTiles.y + 1);
	Vector<FlatVert> verts;
	verts.resize(numVerts);

	vec2 pos = startPos;
	vec2 uv(0.f);
	const vec2 duv = vec2(1) / vec2(numTiles);
	const u32 yPitch = numTiles.x + 1;
#define INDEX(x, y) (x) + (y) * yPitch
	for (u32 y = 0; y <= numTiles.y; ++y)
	{
		for (u32 x = 0; x <= numTiles.x; ++x)
		{
			verts[x + y * (numTiles.x + 1)] = FlatVert({ float3(pos, 0), uv });
			uv += vec2(duv);
			pos += float2(tileSize, tileSize);
		}
	}
	Vector<u16> indices;
	indices.reserve(numTiles.y * (numTiles.x * 2 + 4) + 2 * (numTiles.y - 1));
	for (u32 y = 0; y < numTiles.y; ++y)
	{
	// Each row a triangle strip:
	// 1--3--5...
	// |\ |\ |\
	// | \| \| \
	// 0--2--4--6...
		indices.push_back(INDEX(0, y));
		indices.push_back(INDEX(0, y + 1));
		indices.push_back(INDEX(1, y));
		indices.push_back(INDEX(1, y + 1));
		for (u32 x = 1; x < numTiles.x; ++x)
		{
			indices.push_back(INDEX(x+1,y));
			indices.push_back(INDEX(x+1,y+1));
		}
		if (y + 1 < numTiles.y)
		{
			// transition to next tri strip
			indices.push_back(indices.back());
			indices.push_back(INDEX(0,y+1));
		}
	}
	auto uploadHdl = device->StartBatchedUpload();
	auto result = device->CreateIndexedMesh(EPrimitiveTopology::TRIANGLE_STRIP, { GetVertAttHdl<FlatVert>(), numVerts, sizeof(FlatVert), verts.data()}, indices, uploadHdl);
	device->FinishBatchedUpload(uploadHdl);
	return result;
#undef INDEX
}

}
