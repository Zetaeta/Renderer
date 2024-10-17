#include "DX11Device.h"

namespace rnd
{
namespace dx11
{

 DX11Device::DX11Device(ID3D11Device* pDevice)
		: IRenderDevice(&mShaderMgr), mDevice(pDevice), mCompiler(pDevice), mShaderMgr(&mCompiler)
{
	mSemanticsToMask["Position"] = VA_Position;
	mSemanticsToMask["Normal"] = VA_Normal;
	mSemanticsToMask["Tangent"] = VA_Tangent;
	mSemanticsToMask["TexCoord"] = VA_TexCoord;
}

DeviceMeshRef DX11Device::CreateDirectMesh(VertexAttributeDesc::Handle vertAtts, u32 numVerts, u32 vertSize, void const* data)
{
	ComPtr<ID3D11Buffer> buffer;
	D3D11_BUFFER_DESC bd = {};
	Zero(bd);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = vertSize * numVerts;
	bd.StructureByteStride = vertSize;
	D3D11_SUBRESOURCE_DATA sd;
	Zero(sd);
	sd.pSysMem = data;

	HR_ERR_CHECK(mDevice->CreateBuffer(&bd, &sd, &buffer));
	auto* mesh = new DX11DirectMesh;
	mesh->vBuff = std::move(buffer);
	mesh->VertexCount = numVerts;
	mesh->VertexAttributes = vertAtts;
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
			if ((mSemanticsToMask[entry.mName] & requiredAtts) == 0)
			{
				continue;
			}
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

		mDevice->CreateInputLayout(&inputElements[0], usedAtts, testShader->GetBufferPointer(),
									testShader->GetBufferSize(), &inputLayout);

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
	RASSERT(false);
	return DXGI_FORMAT_R32_FLOAT;
}

}
}
