#include "DX11RenderTarget.h"

namespace rnd
{
namespace dx11
{
DX11DepthStencil::~DX11DepthStencil()
{
	//for (auto& ds : DepthStencils)
	//{
	//	CHECK_COM_REF(ds);
	//}
	DepthStencils.clear();
}

DX11RenderTarget::DX11RenderTarget(RenderTargetDesc const& desc, ComPtr<ID3D11RenderTargetView> rt, ComPtr<ID3D11DepthStencilView> dsv /*= nullptr*/)
	: IRenderTarget(desc), RenderTargets({ rt }), DepthStencil(dsv)
{
	rnd::dx11::SetResourceName(rt, desc.DebugName);
}
}
}
