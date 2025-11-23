#include "DX11Device.h"
#include "container/Vector.h"
#include "asset/Texture.h"
#include "DX11Swapchain.h"
#include "common/CommandLine.h"

#include "dxgidebug.h"
#include <dxgi1_3.h>

namespace rnd
{
namespace dx11
{

DX11Device::DX11Device()
	: IRenderDevice(&mCompiler)
{
	mSemanticsToMask["Position"] = VA_Position;
	mSemanticsToMask["Normal"] = VA_Normal;
	mSemanticsToMask["Tangent"] = VA_Tangent;
	mSemanticsToMask["TexCoord"] = VA_TexCoord;

	CreateDevice();
	mCompiler.Init(mDevice.Get());
}

void DX11Device::CreateDevice()
{
	UINT createDeviceFlags = 0;
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL		featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0,
	};
	HRESULT res = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
					featureLevelArray, 2, D3D11_SDK_VERSION, &mDevice, &featureLevel, &mDeviceCtx);
	DXCALL(res);
	if (res != S_OK)
		ZE_ASSERT(false);

	ComPtr<ID3D11Debug> debug;
	if (SUCCEEDED(mDevice.As(&debug)))
	{
		ComPtr<ID3D11InfoQueue> d3dInfoQueue;
		if (SUCCEEDED(debug.As(&d3dInfoQueue)))
		{
			D3D11_MESSAGE_ID hide[] = {
				D3D11_MESSAGE_ID_DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET
			};
			D3D11_INFO_QUEUE_FILTER filter;
			Zero(filter);
			filter.DenyList.NumIDs = NumCast<u32>(std::size(hide));
			filter.DenyList.pIDList = hide;
			d3dInfoQueue->AddStorageFilterEntries(&filter);
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
			if (CommandLine::Get().HasArg("-breakwarn"))
			{
				d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);
			}
		}
	}
}

DX11Device::~DX11Device()
{
	MatMgr.Release();
	ResourceMgr.OnDevicesShutdown();

	mDeviceCtx = nullptr;
	mDevice = nullptr;
	{
		ComPtr<IDXGIDebug1> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
		{
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
		}
	}
}

void DX11Device::ProcessTextureCreations()
{
	ResourceMgr.ProcessTextureCreations();
}

ComPtr<ID3D11Buffer> DX11Device::CreateVertexBuffer(VertexBufferData data)
{
	ComPtr<ID3D11Buffer> buffer;
	D3D11_BUFFER_DESC bd = {};
	Zero(bd);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = data.VertSize * data.NumVerts;
	bd.StructureByteStride = data.VertSize;
	D3D11_SUBRESOURCE_DATA sd;
	Zero(sd);
	sd.pSysMem = data.Data;

	HR_ERR_CHECK(mDevice->CreateBuffer(&bd, &sd, &buffer));
	return buffer;
}

DeviceMeshRef DX11Device::CreateDirectMesh(EPrimitiveTopology topology, VertexBufferData data, BatchedUploadHandle uploadHandle)
{
	
	auto* mesh = new DX11DirectMesh; //TODO
	mesh->vBuff = CreateVertexBuffer(data);
	mesh->VertexCount = data.NumVerts;
	mesh->VertexAttributes = data.VertAtts;
	return mesh;
}

RefPtr<IDeviceIndexedMesh> DX11Device::CreateIndexedMesh(EPrimitiveTopology topology, VertexBufferData vertexBuffer, Span<u16> indexBuffer, BatchedUploadHandle uploadHandle)
{
	auto* mesh = new DX11IndexedMesh(vertexBuffer.VertAtts, vertexBuffer.NumVerts, NumCast<u32>(indexBuffer.size()), topology);
	mesh->vBuff = CreateVertexBuffer(vertexBuffer);

	{
		D3D11_BUFFER_DESC bd = {};
		Zero(bd);
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = Sizeof(indexBuffer);
		bd.StructureByteStride = sizeof(u16);
		D3D11_SUBRESOURCE_DATA sd;
		Zero(sd);
		sd.pSysMem = indexBuffer.data();

		HR_ERR_CHECK(mDevice->CreateBuffer(&bd, &sd, &mesh->iBuff));
	}
	return mesh;
}

ID3D11InputLayout* DX11Device::GetOrCreateInputLayout(VertexAttributeDesc::Handle vaHandle, VertexAttributeMask requiredAtts)
{
	//if (mInputLayouts.size() < vaHandle)
	//{
	//	mInputLayouts.resize(vaHandle);
	//}
	VertAttDrawInfo info {vaHandle, requiredAtts};
	auto& inputLayout = mInputLayouts[info];

	if (inputLayout == nullptr)
	{
		VertexAttributeDesc const& vertAtts = VertexAttributeDesc::GetRegistry().Get(vaHandle);

		u32 const totalAtts = NumCast<u32>(vertAtts.Layout.Entries.size());
		SmallVector<D3D11_INPUT_ELEMENT_DESC, 16> inputElements;
		u32 usedAtts = 0;
		for (u32 i=0; i<totalAtts; ++i)
		{
			DataLayoutEntry const& entry = vertAtts.Layout.Entries[i];
			//if ((mSemanticsToMask[entry.mName] & requiredAtts) == 0)
			//{
			//	continue;
			//}
			++usedAtts;
			D3D11_INPUT_ELEMENT_DESC& desc = inputElements.emplace_back();
			Zero(desc);
			desc.Format = GetFormatForType(entry.mType);
			desc.SemanticName = GetNameData(entry.mName);
			desc.AlignedByteOffset = NumCast<u32>(entry.mOffset);
			desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		}

		ID3DBlob* testShader = mCompiler.mShadersForCIL[requiredAtts].Get();
		if (!ZE_ENSURE(testShader))
		{
			return nullptr;
		}

		DXCALL(mDevice->CreateInputLayout(&inputElements[0], usedAtts, testShader->GetBufferPointer(),
									testShader->GetBufferSize(), &inputLayout));

		//mInputLayouts;
	}

	return inputLayout.Get();
}

DXGI_FORMAT DX11Device::GetFormatForType(TypeInfo const* type)
{
	if (type == &GetTypeInfo<vec2>())
	{
		return DXGI_FORMAT_R32G32_FLOAT;
	}
	if (type == &GetTypeInfo<vec3>())
	{
		return DXGI_FORMAT_R32G32B32_FLOAT;
	}
	ZE_ASSERT(false);
	return DXGI_FORMAT_R32_FLOAT;
}

SamplerHandle DX11Device::GetSampler(SamplerDesc const& desc)
{
	auto& value = mSamplers[desc];
	if (value == nullptr)
	{
		D3D11_SAMPLER_DESC desc11;
		Zero(desc11);
		desc11.ComparisonFunc = EnumCast<D3D11_COMPARISON_FUNC>(desc.Comparison);
		desc11.MipLODBias = desc.MipBias;
		desc11.MinLOD = desc.MinMip;
		desc11.MaxLOD = desc.MaxMip;
		desc11.AddressU = EnumCast<D3D11_TEXTURE_ADDRESS_MODE>(desc.AddressModes[0]);
		desc11.AddressV = EnumCast<D3D11_TEXTURE_ADDRESS_MODE>(desc.AddressModes[1]);
		desc11.AddressW = EnumCast<D3D11_TEXTURE_ADDRESS_MODE>(desc.AddressModes[2]);
		for (int i = 0; i < 4; ++i)
		{
			desc11.BorderColor[i] = desc.BorderColour[i];
		}
		switch (desc.Filter & 0x0f)
		{
		case ETextureFilter::Point:
			desc11.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			break;
		case ETextureFilter::Bilinear:
			desc11.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case ETextureFilter::Trilinear:
			desc11.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			break;
		default:
			ZE_ASSERT(false);
			break;
		}
		// D3D11_FILTER has similar bitwise breakup
		if (!!(desc.Filter & ETextureFilter::Comparison))
		{
			desc11.Filter = EnumCast<D3D11_FILTER>(desc11.Filter | D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT);
		}
		else if (!!(desc.Filter & ETextureFilter::Minimum))
		{
			desc11.Filter = EnumCast<D3D11_FILTER>(desc11.Filter | D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT);
		}
		else if (!!(desc.Filter & ETextureFilter::Maximum))
		{
			desc11.Filter = EnumCast<D3D11_FILTER>(desc11.Filter | D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT);
		}
		mDevice->CreateSamplerState(&desc11, &value);
	}
	return SamplerHandle(value.Get());
}

IDXGIFactory2* DX11Device::GetFactory()
{
	if (mFactory == nullptr)
	{

		ComPtr<IDXGIDevice2> dxgiDevice;
		DXCALL(mDevice->QueryInterface(IID_PPV_ARGS(&dxgiDevice)));
		ComPtr<IDXGIAdapter> adapter;
		DXCALL(dxgiDevice->GetParent(IID_PPV_ARGS(&adapter)));
		DXCALL(adapter->GetParent(IID_PPV_ARGS(&mFactory)));
	}
	return mFactory.Get();
}

IDeviceSurface* DX11Device::CreateSurface(wnd::Window* window)
{
	return mSwapChains.emplace_back(MakeOwning<DX11SwapChain>(this, window)).get();
}

}
}
