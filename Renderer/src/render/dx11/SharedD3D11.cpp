#include "SharedD3D11.h"
#include "render/DeviceTexture.h"


namespace rnd
{
namespace dx11
{

D3D11_PRIMITIVE_TOPOLOGY GetD3D11Topology(EPrimitiveTopology topology)
{
	switch (topology)
	{
		case EPrimitiveTopology::Triangles:
			return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case EPrimitiveTopology::TriangleStrip:
			return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		default:
			assert(false);
			return {};
	}
}

}
}
