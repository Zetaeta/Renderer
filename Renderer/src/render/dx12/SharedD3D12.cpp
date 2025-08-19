#include "SharedD3D12.h"
#include "DX12RHI.h"

namespace rnd::dx12
{

ID3D12Resource_* GetD3D12Resource(IDeviceResource* resource)
{
	if (!resource)
	{
		return nullptr;
	}
	return resource->GetRHIResource().As<ID3D12Resource_*>();
}

void DX12DeferredRelease(ComPtr<ID3D12Pageable>&& resource)
{
	return GetRHI().DeferredRelease(std::move(resource));
}

D3D_PRIMITIVE_TOPOLOGY GetD3D12Topology(EPrimitiveTopology topology)
{
	switch (topology)
	{
		case rnd::EPrimitiveTopology::Triangles:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case rnd::EPrimitiveTopology::TriangleStrip:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		default:
			assert(false);
			return {};
	}
}

}
