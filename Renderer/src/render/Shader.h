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

struct ShaderDesc
{
	String Name;
	String File;
	String EntryPoint = "main";
};

class ShaderRegistry
{
public:

	const ShaderDesc& GetRegisteredShader(u32 idx) const
	{
		return mRegisteredShaders[idx];
	}

	static ShaderRegistry& Get();

	std::vector<ShaderDesc> mRegisteredShaders;


	template<typename T>
	u32 RegisterShaderType(char const* type, char const* shaderFile, char const* entryPoint)
	{
		mRegisteredShaders.emplace_back(ShaderDesc(type, shaderFile, entryPoint));
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

class ComputeShader : public TShader<EShaderType::Compute>
{
};

//struct ShaderResources
//{
//	Vector<ResourceView> SRVs;
//};

using ShaderTypeId = u32;

#define DECLARE_SHADER(Type)\
public:\
	static const ShaderTypeId sRegistryId;
//	Type(OwningPtr<IDeviceShader>&&)


#define DEFINE_SHADER(Type, ShaderFile, EntryPoint)\
	ShaderTypeId const Type::sRegistryId = ShaderRegistry::Get().RegisterShaderType<Type>(#Type, "" ShaderFile, "" EntryPoint);
}

#define VS_INPUTS(args)\
public:\
	constexpr static VertexAttributeMask GetInputSignature(const Permutation&) { return VertexAttributeMask(args); }

#define VS_INPUTS_START public:\
	static VertexAttributeMask GetInputSignature(const Permutation& perm) { u32 mask = 0;
#define VS_INPUT_STATIC(arg) mask |= arg;
#define VS_INPUT_SWITCH(boolSwitch, arg) if (perm.Get<boolSwitch>().Value) mask |= arg; 
#define VS_INPUTS_END return VertexAttributeMask(mask); }


START_HASH(rnd::ShaderInstanceId, id)
{
	return CombineHash(id.PermuatationId, id.ShaderId);
}
END_HASH(rnd::ShaderInstanceId)

