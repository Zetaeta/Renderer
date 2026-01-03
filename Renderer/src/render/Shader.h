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
#include "core/String.h"

#ifndef SHADER_PARAMETER_VALIDATION
	#define SHADER_PARAMETER_VALIDATION defined(_DEBUG)
#endif


namespace rnd
{

class IDeviceShader;

enum class EShaderParamObjectKind : u8
{
	Value,
	ConstantBuffer,
	SRV,
	UAV
};

inline EResourceType GetResourceDimension(IDeviceResource* resource)
{
	return resource ? resource->GetResourceType() : EResourceType::Unknown;
}

template<typename ObjRefType, EResourceType InDimension>
struct ShaderParamObjectBase
{
	ShaderParamObjectBase()
	{}

	constexpr static EResourceType Dimension = InDimension;

	ShaderParamObjectBase& operator=(ObjRefType const& obj)
	{
#if SHADER_PARAMETER_VALIDATION
		ZE_ENSURE(GetResourceDimension(obj) == Dimension);
#endif
		mObject = obj;
		return *this;
	}
private:
	ObjRefType mObject;
};

template<typename EltType = float4>
struct Texture2D : public ShaderParamObjectBase<ResourceView, EResourceType::Texture2D>
{
	using ShaderParamObjectBase::operator=;
	constexpr static auto Kind = EShaderParamObjectKind::SRV;
	using ElementType = EltType;
};


template<typename EltType = float4>
struct RWTexture2D : public ShaderParamObjectBase<UnorderedAccessView, EResourceType::Texture2D>
{
	using ShaderParamObjectBase::operator=;
	constexpr static auto Kind = EShaderParamObjectKind::UAV;
	using ElementType = EltType;
};

struct ShaderParamStructEntryMeta
{
	HashString Name;
	TypeInfo const* ValueType = nullptr;
	int Offset = 0;
	u16 Size = 0;
	EShaderParamObjectKind Kind = EShaderParamObjectKind::Value;

	ShaderParamStructEntryMeta(HashString name, size_t size, size_t offset)
		:Name(name), Size(NumCast<u16>(size)), Offset(static_cast<int>(offset))
	{
	}

	template<typename T>
	T const& Get(void const* paramStruct) const
	{
	
		return *reinterpret_cast<T const*>(static_cast<u8 const*>(paramStruct) + Offset);
	}

	template<typename T>
	requires std::is_pod_v<T>
	void SetType()
	{
		if constexpr (HasTypeInfo<T>)
		{
			ValueType = &GetTypeInfo<T>();
		}
		else
		{
			ValueType = nullptr;
		}
	}

	template<typename T>
	void SetType()
	{
		Kind = T::Kind;
		ValueType = &GetTypeInfo<typename T::ElementType>();
	}
};
struct ShaderParamStructMeta
{
	Vector<ShaderParamStructEntryMeta> Values;
	Vector<ShaderParamStructEntryMeta> SRVs;
	Vector<ShaderParamStructEntryMeta> UAVs;
	Vector<ShaderParamStructEntryMeta> CBVs;
};

template<typename TRange>
inline ShaderParamStructEntryMeta const* FindByName(TRange const& range, HashString name)
{
	for (auto const& entry : range)
	{
		if (entry.Name == name)
		{
			return &entry;
		}
	}

	return nullptr;
}

void RegisterShaderParamStructMeta(ShaderParamStructMeta (*createFunc)(), ShaderParamStructMeta& outMeta);

void RegisterAllShaderParamMeta();

#define SHADER_PARAMETER_STRUCT_START(name)\
struct name                             \
	{                                       \
	constexpr static int _sp_Start = __COUNTER__;\
	using Self = name;\
	template<int spIndex>\
	constexpr static void _sp_GetParamInfo(Vector<ShaderParamStructEntryMeta>& params);\
	template<>\
	constexpr static void _sp_GetParamInfo<_sp_Start>(Vector<ShaderParamStructEntryMeta>& params) {}

#define SHADER_PARAMETER_IMPL(Type, name, impl, ...)\
	Type name __VA_ARGS__;\
	constexpr static int _sp_Param##name = __COUNTER__;\
	template <>                                         \
	static void _sp_GetParamInfo<_sp_Param##name>(Vector<ShaderParamStructEntryMeta>& params)\
	{\
		_sp_GetParamInfo<_sp_Param##name - 1>(params);\
		impl\
	}\

#define SHADER_PARAMETER(Type, name, ...)\
	Type name __VA_ARGS__;\
	constexpr static int _sp_Param##name = __COUNTER__;\
	template <>                                         \
	static void _sp_GetParamInfo<_sp_Param##name>(Vector<ShaderParamStructEntryMeta>& params)\
	{\
		_sp_GetParamInfo<_sp_Param##name - 1>(params);\
		params.emplace_back(#name, sizeof(Type), offsetof(Self, name))\
			  .SetType<Type>();\
	}\

#define SHADER_PARAMETER_UAV(Type, name) SHADER_PARAMETER(Type, name)
#define SHADER_PARAMETER_SRV(Type, name) SHADER_PARAMETER(Type, name)

#define SHADER_PARAMETER_CBV(Type, name) SHADER_PARAMETER_IMPL(Type, name, {\
		params.emplace_back(#name, sizeof(Type), offsetof(Self, name)).Kind = EShaderParamObjectKind::ConstantBuffer;\
	})

#define SHADER_PARAMETER_STRUCT_END()\
	constexpr static int _sp_End = __COUNTER__;\
	constexpr static int _sp_Count = _sp_End - _sp_Start - 1;\
	static ShaderParamStructMeta _sp_MakeMetadata()\
	{\
		ShaderParamStructMeta result;\
		Vector<ShaderParamStructEntryMeta> AllEntries;\
		_sp_GetParamInfo<_sp_End - 1>(AllEntries);\
		for (u32 i=0; i<AllEntries.size(); ++i)\
		{\
			switch (AllEntries[i].Kind)\
			{\
			case EShaderParamObjectKind::Value:\
				result.Values.push_back(std::move(AllEntries[i]));\
				break;\
			case EShaderParamObjectKind::SRV:\
				result.SRVs.push_back(std::move(AllEntries[i]));\
				break;\
			case EShaderParamObjectKind::UAV:\
				result.UAVs.push_back(std::move(AllEntries[i]));\
				break;\
			case EShaderParamObjectKind::ConstantBuffer:\
				result.CBVs.push_back(std::move(AllEntries[i]));\
				break;\
			default:\
				ZE_ASSERT(false);\
				break;\
			}\
		}\
		return result;\
	}\
	static ShaderParamStructMeta const& GetMeta() { return Metadata; }\
private:\
	inline static ShaderParamStructMeta Metadata;\
	inline static bool _startupDummy = []{ RegisterShaderParamStructMeta(_sp_MakeMetadata, Metadata); return true; }();\
};
	//struct TypeInfoHelper                                         \
	//{                                                             \
	//	inline static ClassTypeInfoImpl<Self> const s_TypeInfo = _sp_MakeTypeInfo();\
	//};



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

struct ShaderVariableInfo
{
	HashString Name;
	int Size = -1;
};
struct ShaderCBInfo
{
	HashString Name;
	u32 Size = -1;
	Vector<HashString> VariableNames; 
	Vector<u16> VariableSizes;
	Vector<u16> VariableOffsets;
};


struct BindingDesc
{
	HashString Name;
	EResourceType Type = EResourceType::Unknown;
};
using UAVBindingDesc = BindingDesc;
using SRVBindingDesc = BindingDesc;

enum class ESystemValue : u32
{
	None = 0,
	Position = 1,
	Target = 1 << 2,
	Depth = 1 << 1
};

FLAG_ENUM(ESystemValue)

struct ShaderSignatureParam
{
	String SemanticName;
	u32 SemanticIdx = 0;
	ESystemValue SV = ESystemValue::None;
};

struct ShaderSignature
{
	Vector<ShaderSignatureParam> Params;
	ESystemValue SVMask = ESystemValue::None;
};

struct ShaderResourcesInfo
{
	Vector<HashString> Names;
	Vector<EResourceType> Types;
};

struct ShaderParamersInfo
{
	SmallVector<ShaderCBInfo, 4> ConstantBuffers;

	//ShaderResourcesInfo SRVs;
	//ShaderResourcesInfo UAV;
	SmallVector<SRVBindingDesc, 4> SRVs;
	SmallVector<UAVBindingDesc, 2> UAVs;

	ShaderSignature Inputs;
	ShaderSignature Outputs;
};

//struct EmptyShaderParameters
//{
//	SHADER_PARAMETER_STRUCT_START(EmptyShaderParameters)
//	SHADER_PARAMETER_STRUCT_END()
//};

class Shader : public RefCountedObject
{
public:
	using Permutation = DynamicShaderPermutation;
	IDeviceShader* GetDeviceShader() const
	{
		return DeviceShader.get();
	}

	ShaderRequirements const& GetRequirements() const
	{
		return mRequirements;
	}

	ShaderParamersInfo const& GetParamsInfo() const
	{
		return mParameters;
	}

	char const* GetDebugName() const { return mDebugName.c_str(); }

	DebugName mDebugName;
protected:
	ShaderRequirements mRequirements;
	ShaderParamersInfo mParameters;
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
