#pragma once
#include <core/RefCounted.h>

#include <unordered_map>
#include <core/Types.h>
#include <core/Hash.h>
#include "RndFwd.h"
#include "VertexAttributes.h"

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
	virtual void ModifyCompileEnv(ShaderCompileEnv& env) const {}

	virtual u64 GetUniqueId() const = 0;
};

class DynamicShaderPermutation : public IShaderPermutation
{
public:
	void SetEnv(String const& key, String const& val)
	{
		mEnvironment[key] = val;
	}

	void ModifyCompileEnv(ShaderCompileEnv& env) const;

	u64 GetUniqueId() const override final;

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
	
	bool operator==(ShaderInstanceId const& other) const
	{
		return ShaderId == other.ShaderId && PermuatationId == other.PermuatationId;
	}
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
		mRegisteredShaders.emplace_back(RegisteredShader(type, shaderFile));
		return NumCast<u32>(mRegisteredShaders.size() - 1);
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

class Shader : public RefCountedObject
{
public:
	using Permutation = DynamicShaderPermutation;
	IDeviceShader* GetDeviceShader() const
	{
		return DeviceShader.get();
	}

protected:
	OwningPtr<IDeviceShader> DeviceShader;
	friend ShaderManager;
};

template<EShaderType ShaderType>
class TShader : public Shader
{
public:
	constexpr static EShaderType Type = ShaderType;
};

class PixelShader : public TShader<EShaderType::Pixel>
{
};

class VertexShader : public TShader<EShaderType::Vertex>
{
public:
	VertexAttributeMask GetInputSignature() const
	{
		return InputSigInst;
	}
	VertexAttributeMask InputSigInst;
};

struct ShaderResources
{
	Vector<IDeviceTextureRef> SRVs;
};

#define DECLARE_SHADER(Type)\
public:\
	static const u32 sRegistryId;
//	Type(OwningPtr<IDeviceShader>&&)


#define DEFINE_SHADER(Type, ShaderFile)\
	u32 const Type::sRegistryId = ShaderRegistry::Get().RegisterShaderType<Type>(#Type, "" ShaderFile);

}

#define VS_INPUTS(args)\
public:\
	constexpr static VertexAttributeMask InputSignature = VertexAttributeMask(args);


START_HASH(rnd::ShaderInstanceId, id)
{
	return CombineHash(id.PermuatationId, id.ShaderId);
}
END_HASH(rnd::ShaderInstanceId)

