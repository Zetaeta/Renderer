#pragma once
#include "DX11ShaderCompiler.h"
#include "render/ShaderManager.h"
#include "render/DeviceMesh.h"
#include "render/RenderDevice.h"
#include "asset/Mesh.h"

template<typename T, size_t InlineSize>
class SmallVector : public std::vector<T>
{
public:
	SmallVector(size_t initialSize = 0)
	{
		this->reserve(InlineSize);
		this->resize(initialSize);
	}
	// TODO: Actual implementation
};

namespace rnd
{
namespace dx11
{

struct DX11DirectMesh : public IDeviceMesh
{
public:
	ComPtr<ID3D11Buffer> vBuff;
};

struct DX11IndexedMesh : public IDeviceIndexedMesh
{
	ComPtr<ID3D11Buffer> vBuff;
	ComPtr<ID3D11Buffer> iBuff;
};

class DX11Device : public IRenderDevice
{
public:
	DX11Device(ID3D11Device* pDevice);
	ID3D11Device* mDevice;
	DX11ShaderCompiler mCompiler;
	ShaderManager mShaderMgr;

	DeviceMeshRef CreateDirectMesh(VertexAttributeDesc::Handle vertAtts, u32 numVerts, u32 vertSize, void const* data) override;

	ID3D11InputLayout* GetOrCreateInputLayout(VertexAttributeDesc::Handle vertAtts, VertexAttributeMask requiredAtts);

protected:

	SmallMap<VertAttDrawInfo, ComPtr<ID3D11InputLayout>> mInputLayouts;

	DXGI_FORMAT GetFormatForType(TypeInfo const* type);

//	std::vector<ComPtr<ID3D11InputLayout>> mInputLayouts;

	std::unordered_map<MeshPart const*, DX11IndexedMesh> m_MeshData;

	SmallMap<Name, VertexAttributeMask> mSemanticsToMask;
};
}
}
