#pragma once
#include <d3d11.h>
#include <core/WinUtils.h>
#include <core/Types.h>
#include <core/Maths.h>
#include <core/Utils.h>
#include "render/DeviceTexture.h"
#include "render/dxcommon/SharedD3D.h"


enum class ETextureFormat : u8;
namespace rnd
{
namespace dx11
{

using namespace rnd::dx;

class DX11Renderer;
class DX11RenderTarget;
class DX11Texture;
class DX11DepthStencil;
class DX11Cubemap;

using DX11TextureRef = std::shared_ptr<DX11Texture>;

}
}
