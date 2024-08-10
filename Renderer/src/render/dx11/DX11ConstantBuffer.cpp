#include "DX11ConstantBuffer.h"

namespace rnd
{
namespace dx11
{

void DX11ConstantBuffer::CreateDeviceResource()
{
	D3D11_BUFFER_DESC cbDesc;
	Zero(cbDesc);
	cbDesc.ByteWidth = NumCast<u32>(mData.GetSize());
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA srData;
	Zero(srData);
	srData.pSysMem = mData.GetData();
	HR_ERR_CHECK(mCtx->pDevice->CreateBuffer(&cbDesc, &srData, &mDeviceBuffer))
}

DX11ConstantBuffer::DX11ConstantBuffer(DX11Ctx* ctx, u32 size, CBLayout::Ref layout /*= nullptr*/)
	:mCtx(ctx), IConstantBuffer(size, layout)
{
	CreateDeviceResource();
}

void DX11ConstantBuffer::Update()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	Zero(mappedResource);
	
	HR_ERR_CHECK(mCtx->pContext->Map(mDeviceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
	memcpy(mappedResource.pData, mData.GetData(), mData.GetSize());
	mCtx->pContext->Unmap(mDeviceBuffer.Get(), 0);
}

//void DX11ConstantBuffer::Bind(u32 idx)
//{
//}

}
}
