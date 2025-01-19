#include "DX12ShaderCompiler.h"
#include "core/Types.h"



OwningPtr<rnd::IDeviceShader> rnd::dx12::DX12LegacyShaderCompiler::CompileShader(ShaderInstanceId const& id,
	const ShaderDesc& desc, const ShaderCompileEnv& env, EShaderType ShaderType, VertexAttributeMask inputMask,
		OwningPtr<IShaderReflector>* outReflector, bool forceRecompile)
{
	auto bc = GetShaderBytecode(id, desc, env, ShaderType, outReflector, forceRecompile);
	if (bc)
	{
		return MakeOwning<DX12Shader>(DX12Shader{bc});
	}
	return nullptr;
}

rnd::dx12::DX12LegacyShaderCompiler::DX12LegacyShaderCompiler()
{
	mSrcDir = "src\\shaders";
	mOutDir = "generated\\shaders";
	mTypeStrings[EShaderType::Pixel] = "ps_5_0";
	mTypeStrings[EShaderType::Vertex] = "vs_5_0";
	mTypeStrings[EShaderType::Compute] = "cs_5_0";
}
