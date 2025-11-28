#pragma once
#include <unordered_map>
#include "core/RefCounted.h"
#include "Shader.h"
#include "DeviceShader.h"
#include "shaders/ShaderReflection.h"
#include "shaders/ShaderCompiler.h"

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
	ShaderType const* GetCompiledShader(ShaderTypeId id,
		ShaderType::Permutation const& permutation, VertexAttributeMask inputSig, OwningPtr<IShaderReflector>* outReflector = nullptr)
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
		shader->mDebugName = shaderInfo.Name;
		OwningPtr<IShaderReflector> reflector;
		if constexpr (ShaderType::Type == EShaderType::Vertex)
		{
			deviceShader = mCompiler->CompileShader(instanceId, shaderInfo, env, ShaderType::Type, inputSig, &reflector);
			shader->InputSigInst = inputSig;
		}
		else
		{
			deviceShader = mCompiler->CompileShader(instanceId, shaderInfo, env, ShaderType::Type, {}, &reflector);
		}
		if (reflector)
		{
			shader->mRequirements = GetRequirements(reflector.get());
			shader->mParameters = reflector->GetParamsInfo();
		}
		if (outReflector)
		{
			*outReflector = std::move(reflector);
		}
		shader->DeviceShader = std::move(deviceShader);
		mCompiledShaders[instanceId] = shader;
		auto& compileInfo = (mCompileInfo[instanceId] = {std::move(env), ShaderType::Type, {}});
		return shader;
	}

	template <typename ShaderType>
	ShaderType const* GetCompiledShader(ShaderTypeId id = ShaderType::sRegistryId,
		ShaderType::Permutation const& permutation = {}, OwningPtr<IShaderReflector>* outReflector= nullptr)
	{
		VertexAttributeMask inputSig{};
		if constexpr (ShaderType::Type == EShaderType::Vertex)
		{
			inputSig = ShaderType::GetInputSignature(permutation);
		}
		return GetCompiledShader<ShaderType>(id, permutation, inputSig, outReflector);
	}

	template <typename ShaderType>
	ShaderType const* GetCompiledVertexShader(ShaderTypeId id,
		ShaderType::Permutation const& permutation, VertexAttributeMask inputSig, OwningPtr<IShaderReflector>* outReflector= nullptr)
	{
		return GetCompiledShader<ShaderType>(id, permutation, inputSig, outReflector);
	}

	template<typename ShaderType>
	ShaderType const* GetCompiledShader(ShaderType::Permutation const& permutation)
	{
		return GetCompiledShader<ShaderType>(ShaderType::sRegistryId, permutation);
	}

	ShaderRequirements GetRequirements(IShaderReflector* forShader);

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
