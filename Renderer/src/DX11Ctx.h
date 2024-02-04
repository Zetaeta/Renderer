#pragma once

#include <d3d11.h>
#include <vector>
#include <assert.h>
#include "maths.h"

struct DX11Ctx;

class SRVList : public std::vector<ID3D11ShaderResourceView*>
{
public:
	template<typename TCont>
	void Push(TCont const& cont)
	{
		
		insert(end(), std::begin(cont), std::end(cont));
	}

	void Pop(size_type n)
	{
		assert(n <= size());
		resize(size() - n);
	}

	void Bind(DX11Ctx const& ctx);
};


struct DX11Ctx
{
	ID3D11Device*		 pDevice;
	ID3D11DeviceContext* pContext;
	SRVList				 pixelSRVs;
};
