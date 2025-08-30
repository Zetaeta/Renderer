#pragma once
#include "render/DeviceShader.h"
#include "SharedD3D12.h"
#include "render/dxcommon/DXShaderCompiler.h"

#include <dxcapi.h>

namespace rnd::dx12
{

struct DX12Shader : public IDeviceShader
{
	virtual ~DX12Shader() {}
	DX12Shader(ComPtr<ID3DBlob> bc)
	:Bytecode(bc){ }
	ComPtr<ID3DBlob> Bytecode;
};

class DX12LegacyShaderCompiler : public IShaderCompiler, public dx::FXCShaderCompiler
{
public:
	DX12LegacyShaderCompiler();

	OwningPtr<IDeviceShader> CompileShader(ShaderInstanceId const& id, const ShaderDesc& desc, const ShaderCompileEnv& env,
		EShaderType ShaderType, VertexAttributeMask inputMask, OwningPtr<IShaderReflector>* outReflector, bool forceRecompile) override;

};

class DX12ShaderCompiler : public IShaderCompiler, public DXShaderCompiler, public IDxcIncludeHandler
{
public:
	DX12ShaderCompiler();
	~DX12ShaderCompiler();

	OwningPtr<IDeviceShader> CompileShader(ShaderInstanceId const& id, const ShaderDesc& desc, const ShaderCompileEnv& env,
		EShaderType ShaderType, VertexAttributeMask inputMask, OwningPtr<IShaderReflector>* outReflector, bool forceRecompile) override;

	// IDxcIncludeHandler API
	virtual HRESULT LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource) override;
	virtual HRESULT QueryInterface(REFIID riid, void** ppvObject) override;
	virtual ULONG AddRef() override;
	virtual ULONG Release() override;

private:
	ComPtr<IDxcCompiler3> mCompiler;
	ComPtr<IDxcUtils> mDxcUtils;

	Vector<fs::path> mCurrentCompilePaths;
	u32 mRefCount = 0;
};
}
