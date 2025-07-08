#include "ShaderManager.h"
#include "shaders/ShaderReflection.h"

namespace rnd
{

void ShaderManager::RecompileAll(bool bForce)
{
	for (auto& [id, shader] : mCompiledShaders)
	{
		auto& shaderInfo = ShaderRegistry::Get().GetRegisteredShader(id.ShaderId);
		CompileInfo const& compileInfo = mCompileInfo[id];
		shader->DeviceShader = mCompiler->CompileShader(id, shaderInfo, compileInfo.Env, compileInfo.Type, compileInfo.InputSignature, nullptr, bForce);
	}
}

ShaderManager::~ShaderManager()
{
	mCompileInfo.clear();
	for (auto& [id, shader] : mCompiledShaders)
	{
		Assertf(shader->GetRefCount() <= 1, "Shader %s is still referenced", ShaderRegistry::Get().GetRegisteredShader(id.ShaderId).Name.c_str());
		Delete(shader);
	}
	mCompiledShaders.clear();
}

ShaderRequirements ShaderManager::GetRequirements(IShaderReflector* forShader)
{
	ShaderRequirements result;
	SmallVector<IShaderReflector::UAVDesc, 4> uavs;
	SmallVector<IShaderReflector::SRVDesc, 4> srvs;
	forShader->GetBindingInfo(srvs, uavs);
	result.NumCBs = NumCast<u32>(forShader->GetConstantBuffers().size());
	result.NumSRVs = NumCast<u32>(srvs.size());
	result.NumUAVs = NumCast<u32>(uavs.size());
	return result;
}

}
