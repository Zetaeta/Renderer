#pragma once

#include <d3d11.h>
#include <vector>
#include <assert.h>
#include <array>
#include "core/Maths.h"
#include "core/Types.h"
#include "render/RenderContext.h"
#include <render/RenderDeviceCtx.h>

namespace rnd
{
namespace dx11
{

struct DX11Ctx;
class DX11Renderer;


struct DX11Ctx
{
	ID3D11Device*		 pDevice;
	ID3D11DeviceContext* pContext;
	DX11Renderer* m_Renderer;		
	rnd::RenderContext* mRCtx = nullptr;
};

}
}
