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
		case EPrimitiveTopology::TRIANGLES:
			return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case EPrimitiveTopology::TRIANGLE_STRIP:
			return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		default:
			assert(false);
			return {};
	}
}

}
}
