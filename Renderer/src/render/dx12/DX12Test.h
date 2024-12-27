#pragma once

#include "SharedD3D12.h"
#include <d3dx12.h>
#include "render/dx11/DX11ShaderCompiler.h"
#include "d3dcompiler.h"
#include "DX12Window.h"

namespace rnd
{
namespace dx12
{

class DX12Test
{
public:
	DX12Test(ID3D12Device_* device);

	void Render(ID3D12GraphicsCommandList_& cmdList);

	ComPtr<ID3D12RootSignature> mRootSig;
	ComPtr<ID3D12PipelineState> mPSO;
};

DX12Test::DX12Test(ID3D12Device_* device)
{
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
	rootSigDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	HR_ERR_CHECK(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	HR_ERR_CHECK(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRootSig)));

	ComPtr<ID3DBlob> vertexShader = DXCompileFile(L"src/shaders/dx12/Test.hlsl", "TestVS", "vs_5_0", nullptr, D3DCOMPILE_ENABLE_STRICTNESS);;
	ComPtr<ID3DBlob> pixelShader = DXCompileFile(L"src/shaders/dx12/Test.hlsl", "TestPS", "ps_5_0", nullptr, D3DCOMPILE_ENABLE_STRICTNESS);;

	ZE_ASSERT(vertexShader && pixelShader);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc {};
	psoDesc.pRootSignature = mRootSig.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = false;
	psoDesc.DepthStencilState.StencilEnable = false;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	HR_ERR_CHECK(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));

}

void DX12Test::Render(ID3D12GraphicsCommandList_& cmdList)
{
	cmdList.SetGraphicsRootSignature(mRootSig.Get());
	cmdList.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//o	cmdList.IASetVertexBuffers(0,)
	cmdList.DrawInstanced(3, 1, 0, 0);

//	commandList->RSSet
}

}
}
