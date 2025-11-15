#pragma once
#include <core/RefCounted.h>

#include <unordered_map>
#include <core/Types.h>
#include <core/Hash.h>
#include "RndFwd.h"
#include "VertexAttributes.h"
#include "RenderEnums.h"
#include "core/ClassTypeInfo.h"
#include "DeviceResource.h"

#ifndef SHADER_PARAMETER_VALIDATION
	#define SHADER_PARAMETER_VALIDATION defined(_DEBUG)
#endif


namespace rnd
{

class IDeviceShader;

struct ShaderParamInfo
{
	PropertyInfo Property;
};

enum class EShaderParamObjectKind : u8
{
	ConstantBuffer,
	SRV,
	UAV
};

inline EResourceType GetResourceDimension(IDeviceResource* resource)
{
	return resource ? resource->GetResourceType() : EResourceType::Unknown;
}

template<typename ObjRefType, EResourceType InDimension>
class ShaderParamObjectBase
{
	ShaderParamObjectBase(EResourceType dimension)
	{}

	ObjRefType mObject;
	constexpr static EResourceType Dimension = InDimension;

	ShaderParamObjectBase& operator=(ObjRefType const& obj)
	{
#if SHADER_PARAMETER_VALIDATION
		ZE_ENSURE(GetResourceDimension(obj) == Dimension);
#endif
		mObject = obj;
		return *this;
	}
};

template<typename ElementType = float4>
struct Texture2D : public ShaderParamObjectBase<ResourceView, EResourceType::Texture2D>
{
};


template<typename ElementType = float4>
struct RWTexture2D : public ShaderParamObjectBase<UnorderedAccessView, EResourceType::Texture2D>
{
};

#define SHADER_PARAMETER_STRUCT_START(name)\
	constexpr static int _sp_Start = __COUNTER__;\
	using Self = name;\
	template<int spIndex>\
	constexpr static void _sp_GetParamInfo(Vector<ShaderParamInfo>& params);\
	template<>\
	constexpr static void _sp_GetParamInfo<_sp_Start>(Vector<ShaderParamInfo>& params) {}

#define SHADER_PARAMETER(Type, name, ...)\
	Type name __VA_ARGS__;\
	constexpr static int _sp_Param##name = __COUNTER__;\
	template <>                                         \
	constexpr static void _sp_GetParamInfo<_sp_Param##name>(Vector<ShaderParamInfo>& params)\
	{\
		_sp_GetParamInfo<_sp_Param##name - 1>(params);\
		params.push_back({ PropertyInfo(#name, GetTypeInfo<Type>(), offsetof(Self, name)) });\
	}

#define SHADER_PARAMETER_UAV()

#define SHADER_PARAMETER_STRUCT_END(name)\
	constexpr static int _sp_End = __COUNTER__;\
	constexpr static int _sp_Count = _sp_End - _sp_Start - 1;\
	inline static ClassTypeInfoImpl<Self> _sp_MakeTypeInfo()\
	{\
		Vector<ShaderParamInfo> params;\
		_sp_GetParamInfo<_sp_End - 1>(params);\
		ClassTypeInfo::Properties attrs;\
		for (const auto& param : params)\
		{                                                     \
			attrs.push_back(param.Property);\
		}\
		return ClassTypeInfoImpl<Self>{ #name, nullptr, EClassFlags::None, std::move(attrs) }; \
	}\
	struct TypeInfoHelper                                         \
	{                                                             \
		inline static ClassTypeInfoImpl<Self> const s_TypeInfo = _sp_MakeTypeInfo();\
	};



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
		return mRegisteredShaders[idx - 1];
	}

	static ShaderRegistry& Get();

	std::vector<ShaderDesc> mRegisteredShaders;


	template<typename T>
	u32 RegisterShaderType(char const* type, char const* shaderFile, char const* entryPoint)
	{
		mRegisteredShaders.emplace_back(ShaderDesc(type, shaderFile, entryPoint));
		return NumCast<u32>(mRegisteredShaders.size());
	}
};

class ShaderManager;

struct ShaderRequirements
{
	u32 NumUAVs = 0;
	u32 NumSRVs = 0;
	u32 NumCBs = 0;
};

struct EmptyShaderParameters
{
	SHADER_PARAMETER_STRUCT_START(EmptyShaderParameters)
	SHADER_PARAMETER_STRUCT_END(EmptyShaderParameters)
};

class Shader : public RefCountedObject
{
public:
	using Permutation = DynamicShaderPermutation;
	IDeviceShader* GetDeviceShader() const
	{
		return DeviceShader.get();
	}

	ShaderRequirements const& GetRequirements()
	{
		return mRequirements;
	}
protected:
	ShaderRequirements mRequirements;
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

#define VS_INPUTS(args)\
public:\
	constexpr static VertexAttributeMask GetInputSignature(const Permutation&) { return VertexAttributeMask(args); }

#define VS_INPUTS_START public:\
	static VertexAttributeMask GetInputSignature(const Permutation& perm) { u32 mask = 0;
#define VS_INPUT_STATIC(arg) mask |= arg;
#define VS_INPUT_SWITCH(boolSwitch, arg) if (perm.Get<boolSwitch>().Value) mask |= arg; 
#define VS_INPUTS_END return VertexAttributeMask(mask); }


}

START_HASH(rnd::ShaderInstanceId, id)
{
	return CombineHash(id.PermuatationId, id.ShaderId);
}
END_HASH(rnd::ShaderInstanceId)
