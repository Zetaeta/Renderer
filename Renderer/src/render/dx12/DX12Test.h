#pragma once

#include "SharedD3D12.h"
#include <d3dx12.h>
#include "render/dx11/DX11ShaderCompiler.h"
#include "d3dcompiler.h"
#include "DX12Window.h"
#include "core/Random.h"
#include "render/VertexTypes.h"
#include "asset/Texture.h"
#include "DX12Context.h"

extern TextureRef gDirt;

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
	RefPtr<IDeviceMesh> mMesh;
	IDeviceTexture::Ref mTexture;
};

DX12Test::DX12Test(ID3D12Device_* device)
{
	D3D12_ROOT_PARAMETER params[2];
	Zero(params);
	params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	params[0].Descriptor = {0, 0};
	params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	params[1].DescriptorTable.NumDescriptorRanges = 1;
	D3D12_DESCRIPTOR_RANGE srvTable{};
	srvTable.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvTable.NumDescriptors = -1;
	params[1].DescriptorTable.pDescriptorRanges = &srvTable;
	CD3DX12_STATIC_SAMPLER_DESC samplerDesc(0);
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
	rootSigDesc.Init(ARRAY_SIZE(params), params, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	HR_ERR_CHECK(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	HR_ERR_CHECK(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRootSig)));

	ComPtr<ID3DBlob> vertexShader = DXCompileFile(L"src/shaders/dx12/Test.hlsl", "TestVS2", "vs_5_0", nullptr, D3DCOMPILE_ENABLE_STRICTNESS);;
	ComPtr<ID3DBlob> pixelShader = DXCompileFile(L"src/shaders/dx12/Test.hlsl", "TestPS", "ps_5_0", nullptr, D3DCOMPILE_ENABLE_STRICTNESS);;

	ZE_ASSERT(vertexShader && pixelShader);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc {};
	psoDesc.pRootSignature = mRootSig.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
	psoDesc.InputLayout.NumElements = 1;
	D3D12_INPUT_ELEMENT_DESC ied[] = 
	{
		{ "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TexCoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FlatVert, uv), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	psoDesc.InputLayout.pInputElementDescs = ied;
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
	DeviceTextureDesc desc = gDirt->MakeDesc();
	mTexture = GetRHI().CreateTexture2D(desc, gDirt->GetData());

	ZE_ASSERT(gDirt.IsValid());

	mMesh = BasicMeshFactory(&GetRHI()).GetFullScreenTri();
}

void DX12Test::Render(ID3D12GraphicsCommandList_& cmdList)
{
	float4 cbData {Random::Range(0.f, 1.f),Random::Range(0.f, 1.f),Random::Range(0.f, 1.f),1};
	auto cbHandle = GetRHI().GetCBPool().AcquireConstantBuffer(ECBLifetime::Dynamic, cbData);
	cmdList.SetGraphicsRootSignature(mRootSig.Get());
	cmdList.SetGraphicsRootConstantBufferView(0, cbHandle.UserData.As<D3D12_GPU_VIRTUAL_ADDRESS>());
	(DX12Context{&cmdList}).SetShaderResources(EShaderType::Pixel, Single<const ResourceView>(ResourceView(mTexture)));
	cmdList.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList.IASetVertexBuffers(0, 1, &static_cast<DX12DirectMesh*>(mMesh.Get())->view);
	cmdList.DrawInstanced(3, 1, 0, 0);

//	commandList->RSSet
}

}
}
