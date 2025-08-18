#pragma once
#include "render/DeviceMesh.h"
#include "render/RenderDevice.h"

namespace ProceduralMeshes
{
// Positions and uvs
RefPtr<rnd::IDeviceMesh> CreateTiledPlane(rnd::IRenderDevice* device, float tileSize, vec2 startPos, uvec2 numTiles);


}
