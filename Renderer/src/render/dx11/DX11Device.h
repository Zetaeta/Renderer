#pragma once
#include "DX11ShaderCompiler.h"
#include "render/ShaderManager.h"
#include "render/DeviceMesh.h"
#include "render/RenderDevice.h"
#include "asset/Mesh.h"
#include "core/BaseDefines.h"
#include "SharedD3D11.h"
#include "DX11Ctx.h"

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

class DX11Device : public IRenderDevice, public IRenderResourceManager
{
public:
	DX11Device(ID3D11Device* pDevice);
	~DX11Device();
	ID3D11Device* mDevice;
	DX11ShaderCompiler mCompiler;
	ShaderManager mShaderMgr;

	void ProcessTextureCreations();

	DeviceMeshRef CreateDirectMesh(VertexAttributeDesc::Handle vertAtts, u32 numVerts, u32 vertSize, void const* data) override;
	ID3D11InputLayout* GetOrCreateInputLayout(VertexAttributeDesc::Handle vertAtts, VertexAttributeMask requiredAtts);

	void CreateRenderTexture(Texture* texture) override;
	void DestroyRenderTexture(Texture* texture) override;
	DeviceTextureRef GetDeviceTexture(const Texture* texture) override;
	DX11TextureRef GetRenderTexture(const Texture* texture)
#if MULTI_RENDER_BACKEND
	;
#else
	{
		return static_pointer_cast<DX11Texture>(texture->GetDeviceTexture());
	}
#endif

	SamplerHandle GetSampler(SamplerDesc const& desc) override;

	std::unordered_map<TextureId, DX11TextureRef> mTextures;



protected:
	std::unordered_map<SamplerDesc, ComPtr<ID3D11SamplerState>, GenericHash<>> mSamplers;

	struct TexDestroyCommand
	{
		TextureId TexId;
		DX11TextureRef DevTex;
	};
	Vector<TexDestroyCommand> mDestroyCommands;
	Vector<class Texture*> mCreateCommands;
	std::mutex mCreateDestroyMtx;

	SmallMap<VertAttDrawInfo, ComPtr<ID3D11InputLayout>> mInputLayouts;

	DXGI_FORMAT GetFormatForType(TypeInfo const* type);

//	std::vector<ComPtr<ID3D11InputLayout>> mInputLayouts;

	std::unordered_map<MeshPart const*, DX11IndexedMesh> m_MeshData;

	SmallMap<Name, VertexAttributeMask> mSemanticsToMask;

	DX11Ctx* mCtx = nullptr;
private:

};
}
}
