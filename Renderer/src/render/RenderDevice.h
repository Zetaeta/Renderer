#pragma once
#include "core/BaseDefines.h"
#include "DeviceTexture.h"
#include "RndFwd.h"
#include "DeviceMesh.h"
#include "BasicMeshes.h"
#include "core/memory/CopyableMemory.h"
#include <span>
#include "RenderResourceManager.h"

namespace rnd { class ShaderManager; }

namespace rnd
{

struct CBHandle
{
	CopyableMemory<8> UserData;
};
struct PooledCBHandle : CBHandle
{
	u64 Id = 0;
};

template<typename T>
using Span = std::span<T>;

class ICBPool
{
public:
	virtual PooledCBHandle AcquireConstantBuffer(u32 size, std::span<const byte> initialData) = 0;
	virtual void	 ReleaseConstantBuffer(PooledCBHandle handle) = 0;

	virtual ~ICBPool() {}
};

class IRenderDevice
{
protected:
	IRenderDevice(ShaderManager* shaderMgr) :ShaderMgr(shaderMgr), BasicMeshes(this) {}
public:
	virtual IDeviceTexture::Ref CreateTextureCube(DeviceTextureDesc const& desc, CubemapData const& initialData = CubemapData{}) = 0;
	virtual IDeviceTexture::Ref CreateTexture2D(DeviceTextureDesc const& desc, TextureData initialData = nullptr) = 0;
	virtual DeviceMeshRef		CreateDirectMesh(VertexAttributeDesc::Handle vertAtts, u32 numVerts, u32 vertSize, void const* data) = 0;

	virtual ~IRenderDevice() {}

//	bool CreateInputLayout(VertexAttributeDesc::Handle vaHandle, I)

	template<typename Range>
	DeviceMeshRef CreateDirectMesh(const Range& vertices)
	{
		using Vert = std::remove_cvref_t<decltype(vertices[0])>;
		return CreateDirectMesh(TVertexAttributes<Vert>::template Handle, NumCast<u32>(std::size(vertices)), sizeof(Vert), &vertices[0]);
	}

	ShaderManager* const ShaderMgr;

	BasicMeshFactory BasicMeshes;
};

}
