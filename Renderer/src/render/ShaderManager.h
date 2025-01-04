#pragma once
#include <unordered_map>
#include "core/RefCounted.h"
#include "Shader.h"
#include "DeviceShader.h"

#define ALLOW_RECOMPILATION ZE_BUILD_EDITOR

namespace rnd
{

class IShaderReflector;
class ShaderManager
{
public:
	ShaderManager(IShaderCompiler* compiler)
		: mCompiler(compiler)
	{}

	~ShaderManager();

	template <typename ShaderType>
	ShaderType const* GetCompiledShader(ShaderTypeId id = ShaderType::sRegistryId,
		ShaderType::Permutation const& permutation = {}, OwningPtr<IShaderReflector>* outReflector= nullptr)
	{
		ShaderInstanceId instanceId{ id, permutation.GetUniqueId() };
		if (auto it = mCompiledShaders.find(instanceId); it != mCompiledShaders.end())
		{
			return static_cast<ShaderType*>(it->second.Get());
		}

		ShaderCompileEnv env;
		permutation.ModifyCompileEnv(env);
		auto& shaderInfo = ShaderRegistry::Get().GetRegisteredShader(id);
		OwningPtr<IDeviceShader> deviceShader;
		ShaderType* shader = new ShaderType;
		if constexpr (ShaderType::Type == EShaderType::Vertex)
		{
			auto inputSig = ShaderType::GetInputSignature(permutation);
			deviceShader = mCompiler->CompileShader(instanceId, shaderInfo, env, ShaderType::Type, inputSig, outReflector);
			shader->InputSigInst = inputSig;
		}
		else
		{
			deviceShader = mCompiler->CompileShader(instanceId, shaderInfo, env, ShaderType::Type, {}, outReflector);
		}
		shader->DeviceShader = std::move(deviceShader);
		mCompiledShaders[instanceId] = shader;
		auto& compileInfo = (mCompileInfo[instanceId] = {std::move(env), ShaderType::Type, {}});
		if constexpr (ShaderType::Type == EShaderType::Vertex)
		{
			compileInfo.InputSignature = ShaderType::GetInputSignature(permutation);
		}
		return shader;
	}

	template<typename ShaderType>
	ShaderType const* GetCompiledShader(ShaderType::Permutation const& permutation)
	{
		return GetCompiledShader<ShaderType>(ShaderType::sRegistryId, permutation);
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
