#pragma once
#include <d3d11.h>
#include "maths.h"
#include "DX11Backend.h"
#include <wrl.h>
#include <memory>
#include "Utils.h"
#include "Texture.h"

using Microsoft::WRL::ComPtr;

class ImageDX11
{
public:
	ImageDX11(ID3D11Device* device)
		: m_Device(device) {}

	RCOPY_MOVE_PROTECT(ImageDX11);

	using Ref = std::shared_ptr<ImageDX11>;

	//template<template<typename T> typename TPtr>
	static Ref Create(ID3D11Device* device, u32 width, u32 height, u32 const* data, bool sRGB = false) {
		auto result = std::make_shared<ImageDX11>(device);
		result->Init(width, height, data, sRGB);
		return result;
	}
	static Ref CreateFrom(TextureRef tex, ID3D11Device* device, bool sRGB = false)
	{
		return Create(device, u32(tex->width), u32(tex->height), tex->GetData(), sRGB);
	}

	void* GetHandle()
	{
		return m_SRV.Get();
	}

	ID3D11ShaderResourceView* GetSRV()
	{
		return m_SRV.Get();
	}
	
	void Init(u32 width, u32 height, u32 const* data, bool sRGB);

private:
	ComPtr<ID3D11ShaderResourceView> m_SRV = nullptr;
	ID3D11Device* m_Device;
};
