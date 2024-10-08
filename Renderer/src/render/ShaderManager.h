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
	RefPtr<ShaderType const> GetCompiledShader(ShaderType::Permutation const& permutation) const
	{
		ShaderInstanceId instanceId{ ShaderType::sRegistryId, permutation.GetUniqueId() };
		if (auto it = mCompiledShaders.find(); it != mCompiledShaders.end())
		{
			return it->second;
		}

		ShaderCompileEnv env;
		permutation.ModifyCompileEnv(env);
		auto& shaderInfo = ShaderRegistry::Get().GetRegisteredShader(ShaderType::sRegistryId);
		auto deviceShader = mCompiler->CompileShader(shaderInfo.Name, env, ShaderType::Type);
		auto& shader = mCompiledShaders[instanceId];
		shader = MakeOwning<ShaderType>();
		shader->DeviceShader = std::move(deviceShader);

	}
protected:
	std::unordered_map<ShaderInstanceId, OwningPtr<Shader>> mCompiledShaders;

	IShaderCompiler* mCompiler = nullptr;
};
} // namespace rnd
