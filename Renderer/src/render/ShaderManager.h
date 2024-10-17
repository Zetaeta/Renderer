#pragma once
#include <unordered_map>
#include "core/RefCounted.h"
#include "Shader.h"
#include "DeviceShader.h"

namespace rnd
{
class ShaderManager
{
public:
	ShaderManager(IShaderCompiler* compiler)
		: mCompiler(compiler)
	{}

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
			deviceShader = mCompiler->CompileShader(instanceId, shaderInfo.File, env, ShaderType::Type, ShaderType::InputSignature);
			shader->InputSigInst = ShaderType::InputSignature;
		}
		else
		{
			deviceShader = mCompiler->CompileShader(instanceId, shaderInfo.File, env, ShaderType::Type);
		}
		shader->DeviceShader = std::move(deviceShader);
		mCompiledShaders[instanceId] = shader;
		return shader;

	}
protected:
	std::unordered_map<ShaderInstanceId, RefPtr<Shader>> mCompiledShaders;

	IShaderCompiler* mCompiler = nullptr;
};
} // namespace rnd
