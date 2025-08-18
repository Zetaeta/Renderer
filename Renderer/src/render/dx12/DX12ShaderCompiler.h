#pragma once
#include "render/DeviceShader.h"
#include "SharedD3D12.h"
#include "render/dxcommon/DXShaderCompiler.h"

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
}
