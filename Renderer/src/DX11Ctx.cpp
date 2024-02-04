#include "DX11Ctx.h"

void SRVList::Bind(DX11Ctx const& ctx)
{
	ctx.pContext->PSSetShaderResources(0, u32(size()), &front());
}


