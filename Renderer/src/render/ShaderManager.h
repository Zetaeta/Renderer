#pragma once
#include <unordered_map>
#include "core/RefCounted.h"
#include "Shader.h"
#include "DeviceShader.h"

#define ALLOW_RECOMPILATION ZE_BUILD_EDITOR

namespace rnd
{
class ShaderManager
{
public:
	ShaderManager(IShaderCompiler* compiler)
		: mCompiler(compiler)
	{}

	~ShaderManager();

	template <typename ShaderType>
	ShaderType const* GetCompiledShader(ShaderType::Permutation const& permutation = {})
	{
		ShaderInstanceId instanceId{ ShaderType::sRegistryId, permutation.GetUniqueId() };
		if (auto it = mCompiledShaders.find(instanceId); it != mCompiledShaders.end())
		{
			return static_cast<ShaderType*>(it->second.Get());
		}

		ShaderCompileEnv env;
		permutation.ModifyCompileEnv(env);
		auto& shaderInfo = ShaderRegistry::Get().GetRegisteredShader(ShaderType::sRegistryId);
		OwningPtr<IDeviceShader> deviceShader;
		ShaderType* shader = new ShaderType;
		if constexpr (ShaderType::Type == EShaderType::Vertex)
		{
			deviceShader = mCompiler->CompileShader(instanceId, shaderInfo, env, ShaderType::Type, ShaderType::InputSignature);
			shader->InputSigInst = ShaderType::InputSignature;
		}
		else
		{
			deviceShader = mCompiler->CompileShader(instanceId, shaderInfo, env, ShaderType::Type);
		}
		shader->DeviceShader = std::move(deviceShader);
		mCompiledShaders[instanceId] = shader;
		auto& compileInfo = (mCompileInfo[instanceId] = {std::move(env), ShaderType::Type, {}});
		if constexpr (ShaderType::Type == EShaderType::Vertex)
		{
			compileInfo.InputSignature = ShaderType::InputSignature;
		}
		return shader;
	}

#if ALLOW_RECOMPILATION
	void RecompileAll(bool bForce);
#endif

protected:

	struct CompileInfo
	{
		ShaderCompileEnv Env;
		EShaderType Type;
		VertexAttributeMask InputSignature;
	};
	std::unordered_map<ShaderInstanceId, RefPtr<Shader>> mCompiledShaders;
#if ALLOW_RECOMPILATION
	std::unordered_map<ShaderInstanceId, CompileInfo> mCompileInfo;
#endif

	IShaderCompiler* mCompiler = nullptr;
};
} // namespace rnd
