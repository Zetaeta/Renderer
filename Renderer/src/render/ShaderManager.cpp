#include "ShaderManager.h"

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
		delete shader;
		shader = nullptr;
	}
	mCompiledShaders.clear();
}

}
