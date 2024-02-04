#pragma once
#include "Texture.h"
#include <d3d11.h>
#include "WinUtils.h"

class CubemapDX11
{

public:
	static CubemapDX11 FoldUp(ID3D11Device* device, TextureRef tex);
	ComPtr<ID3D11ShaderResourceView> srv;
	
};
