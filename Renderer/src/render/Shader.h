#pragma once
#include <core/RefCounted.h>

#include <unordered_map>
#include <core/Types.h>
#include <core/Hash.h>

namespace rnd { class IDeviceShader; }

namespace rnd
{

class ShaderCompileEnv
{
public:
	void SetEnv(String const& key, String const& val)
	{
		Defines[key] = val;
	}

	std::unordered_map<String, String> Defines;
};

class IShaderPermutation
{
public:
	virtual void ModifyCompileEnv(ShaderCompileEnv& env) {}

	virtual u64 GetUniqueId() = 0;
};

class DynamicShaderPermutation : IShaderPermutation
{
public:
	void SetEnv(String const& key, String const& val)
	{
		mEnvironment[key] = val;
	}

	void ModifyCompileEnv(ShaderCompileEnv& env);

	u64 GetUniqueId() override final;

private:

	std::unordered_map<String, String> mEnvironment;
};


//template<typename ShaderType>
//class CompiledShader : RefCountedObject
//{
//	
//};

struct ShaderInstanceId
{
	u32 ShaderId;
	u64 PermuatationId;
};

class Shader;

class ShaderRegistry
{
public:
	struct RegisteredShader
	{
		String Name;
		String File;
	};

	const RegisteredShader& GetRegisteredShader(u32 idx) const
	{
		return mRegisteredShaders[idx];
	}

	static ShaderRegistry& Get();

	std::vector<RegisteredShader> mRegisteredShaders;


	template<typename T>
	u32 RegisterShaderType(char const* type, char const* shaderFile)
	{
		mRegisteredShaders.emplace_back({type, shaderFile});
		return mRegisteredShaders.size() - 1;
	}
};

enum class EShaderType : u8
{
	Vertex,
	Pixel,
	Geometry,
	Compute,
	Count
};

class ShaderManager;

class Shader : RefCountedObject
{
public:
	using Permutation = DynamicShaderPermutation;
protected:
	OwningPtr<IDeviceShader> DeviceShader;
	friend ShaderManager;
};

template<EShaderType ShaderType>
class TShader
{
	constexpr static EShaderType Type = ShaderType;
};

class PixelShader : TShader<EShaderType::Pixel>
{
};

class VertexShader : TShader<EShaderType::Vertex>
{
};

#define DECLARE_SHADER(Type)\
public:\
	extern static const u32 sRegistryId;
//	Type(OwningPtr<IDeviceShader>&&)


#define DEFINE_SHADER(Type, ShaderFile)\
	u32 Type::sRegistryId = ShaderRegistry::Get().RegisterShaderType<Type>(#Type, "" ShaderFile);

}

//START_HASH(rnd::ShaderInstanceId, id)
//{
//	return CombineHash(id.PermuatationId, id.ShaderId);
//}
//END_HASH(rnd::ShaderInstanceId)

