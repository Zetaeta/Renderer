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
		ZE_ASSERT(shader->GetRefCount() <= 1);
		delete shader;
		shader = nullptr;
	}
	mCompiledShaders.clear();
}

}
