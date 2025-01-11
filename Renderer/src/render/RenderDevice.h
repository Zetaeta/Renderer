#pragma once
#include "core/BaseDefines.h"
#include "DeviceTexture.h"
#include "RndFwd.h"
#include "DeviceMesh.h"
#include "BasicMeshes.h"
#include "core/memory/CopyableMemory.h"
#include <span>
#include "RenderResourceManager.h"
#include "SamplerState.h"
#include "GPUSyncPoint.h"

struct CompoundMesh;

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

enum class ECBLifetime : u8
{
	Dynamic, // Exists for <= 1 frame. May be kept in upload heap.
	Static // exists for >1 frame
};

class ICBPool
{
public:
	template<typename T>
	PooledCBHandle AcquireConstantBuffer(ECBLifetime lifetime, T const& data)
	{
		return AcquireConstantBuffer(lifetime, sizeof(T), {reinterpret_cast<const byte*>(&data), sizeof(data)});
	}
	[[nodiscard]] virtual PooledCBHandle AcquireConstantBuffer(ECBLifetime lifetime, u32 size, std::span<const byte> initialData) = 0;
	virtual void	 ReleaseConstantBuffer(PooledCBHandle handle) = 0;

	virtual ~ICBPool() {}
};

class ScopedCBClaim
{
public:
	template<typename... Args>
	ScopedCBClaim(ICBPool* pool, Args... args)
	:mPool(pool)
	{
		mHandle = pool->AcquireConstantBuffer(ECBLifetime::Dynamic, std::forward<Args>(args)...);
	}
	~ScopedCBClaim()
	{
		mPool->ReleaseConstantBuffer(mHandle);
	}

	operator CBHandle()
	{
		return mHandle;
	}
private:
	PooledCBHandle mHandle;
	ICBPool* mPool;
};

struct VertexBufferData
{
	VertexAttributeDesc::Handle VertAtts;
	u32 NumVerts;
	u32 VertSize;
	void const* Data;
};

using BatchedUploadHandle = u32;

class IRenderDevice
{
protected:
	IRenderDevice(ShaderManager* shaderMgr) :ShaderMgr(shaderMgr), BasicMeshes(this) {}
public:
	virtual IDeviceTexture::Ref CreateTextureCube(DeviceTextureDesc const& desc, CubemapData const& initialData = CubemapData{}) = 0;
	virtual IDeviceTexture::Ref CreateTexture2D(DeviceTextureDesc const& desc, TextureData initialData = nullptr) = 0;
	virtual DeviceMeshRef		CreateDirectMesh(EPrimitiveTopology topology, VertexBufferData data, BatchedUploadHandle uploadHandle) = 0;
	virtual RefPtr<IDeviceIndexedMesh>		CreateIndexedMesh(EPrimitiveTopology topology, VertexBufferData vertexBuffer, Span<u16> indexBuffer, BatchedUploadHandle uploadHandle) = 0;

	virtual BatchedUploadHandle StartBatchedUpload() { return 0; }
	virtual OwningPtr<GPUSyncPoint> FinishBatchedUpload(BatchedUploadHandle handle) { return nullptr; }

//	virtual DeviceMeshRef CreateMeshData(CompoundMesh* compoundMesh, BatchedUploadHandle uploadHandle) = 0;

	virtual SamplerHandle GetSampler(SamplerDesc const& desc) = 0;

	virtual ~IRenderDevice() {}

//	bool CreateInputLayout(VertexAttributeDesc::Handle vaHandle, I)

	template<typename Range>
	DeviceMeshRef CreateDirectMesh(const Range& vertices)
	{
		using Vert = std::remove_cvref_t<decltype(vertices[0])>;
		return CreateDirectMesh(EPrimitiveTopology::TRIANGLES, {TVertexAttributes<Vert>::template Handle, NumCast<u32>(std::size(vertices)), sizeof(Vert), &vertices[0]}, {});
	}

	ShaderManager* const ShaderMgr;

	BasicMeshFactory BasicMeshes;
};

}
