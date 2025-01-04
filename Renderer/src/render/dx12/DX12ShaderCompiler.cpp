#include "DX12ShaderCompiler.h"
#include "core/Types.h"



OwningPtr<rnd::IDeviceShader> rnd::dx12::DX12LegacyShaderCompiler::CompileShader(ShaderInstanceId const& id, const ShaderDesc& desc, const ShaderCompileEnv& env, EShaderType ShaderType, VertexAttributeMask inputMask /*=*/, bool forceRecompile /*= false*/)
{
	auto bc = GetShaderBytecode(id, desc, env, ShaderType, forceRecompile);
	if (bc)
	{
		return MakeOwning<DX12Shader>(DX12Shader{bc});
	}
	return nullptr;
}
